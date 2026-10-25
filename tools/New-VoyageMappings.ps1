[CmdletBinding()]
param(
    [string]$GameRoot = 'P:\SteamLibrary\steamapps\common\Voyage',

    [string]$DumperPath,

    [string]$OutputRoot,

    [ValidateRange(1, 4096)]
    [int]$Concurrency = 128,

    [ValidateRange(1, 3600)]
    [int]$ReflectionTimeoutSeconds = 180,

    [ValidateRange(0, 3600)]
    [int]$MinimumProcessAgeSeconds = 60,

    [ValidateRange(1, 20)]
    [int]$StableSampleCount = 3,

    [ValidateRange(1, 7200)]
    [int]$DumpTimeoutSeconds = 600,

    [ValidateRange(1, 60)]
    [int]$RetryIntervalSeconds = 3,

    [switch]$InstallForUAssetGUI
)

$ErrorActionPreference = 'Stop'
Set-StrictMode -Version Latest

if ([string]::IsNullOrWhiteSpace($DumperPath)) {
    $DumperPath = Join-Path $PSScriptRoot '..\.tools\bin\jmap_dumper.exe'
}
if ([string]::IsNullOrWhiteSpace($OutputRoot)) {
    $OutputRoot = Join-Path $PSScriptRoot '..\Tmp\mappings'
}

$gameProcessName = 'VoyageSteam-Win64-Shipping'
$engineVersion = '5.8'
$fingerprintScript = Join-Path $PSScriptRoot 'Get-VoyageBuildFingerprint.ps1'
$findObjectArrayScript = Join-Path $PSScriptRoot 'Find-VoyageUObjectArray.ps1'
$testMappingsScript = Join-Path $PSScriptRoot 'Test-VoyageMappings.ps1'

function Get-SingleVoyageProcess {
    $matches = @(Get-Process -Name $gameProcessName -ErrorAction SilentlyContinue)
    if ($matches.Count -gt 1) {
        throw "Expected at most one $gameProcessName process, found $($matches.Count)."
    }
    if ($matches.Count -eq 1) {
        return $matches[0]
    }
    return $null
}

function Get-Dumper {
    $executablePath = [IO.Path]::GetFullPath($DumperPath)
    if (-not (Test-Path -LiteralPath $executablePath -PathType Leaf)) {
        throw "The canonical jmap binary is absent. Run tools\Publish-JmapBinary.ps1: $executablePath"
    }
    (Resolve-Path -LiteralPath $executablePath).Path
}

function Get-RequiredVoyageProcess {
    $process = Get-SingleVoyageProcess
    if (-not $process) {
        throw 'Voyage must already be running before mappings are generated.'
    }
    $process
}

function Wait-ForObjectArray {
    param(
        [Parameter(Mandatory = $true)]
        [Diagnostics.Process]$Process
    )

    $deadline = [DateTime]::UtcNow.AddSeconds($ReflectionTimeoutSeconds)
    $lastFailure = $null
    $lastSignature = $null
    $stableSamples = 0
    do {
        $processAge = ([DateTime]::UtcNow - $Process.StartTime.ToUniversalTime()).TotalSeconds
        if ($processAge -lt $MinimumProcessAgeSeconds) {
            $remainingAge = [Math]::Ceiling($MinimumProcessAgeSeconds - $processAge)
            Write-Host "Voyage is still warming up; waiting at least $remainingAge more seconds..."
            Start-Sleep -Seconds ([Math]::Min($RetryIntervalSeconds, $remainingAge))
            continue
        }
        try {
            $results = @(& $findObjectArrayScript -TargetProcessId $Process.Id)
            if ($results.Count -ne 1) {
                throw "The GUObjectArray scanner returned $($results.Count) results."
            }
            $candidate = $results[0]
            $signature = @(
                [string]$candidate.guObjectArray,
                [string]$candidate.objects,
                [string]$candidate.numElements,
                [string]$candidate.numChunks,
                [string]$candidate.firstChunk,
                [string]$candidate.firstObject
            ) -join '|'
            if ($signature -ceq $lastSignature) {
                $stableSamples++
            }
            else {
                $lastSignature = $signature
                $stableSamples = 1
            }
            if ($stableSamples -ge $StableSampleCount) {
                return $candidate
            }
            Write-Host "Reflection candidate is stable for $stableSamples/$StableSampleCount samples..."
        }
        catch {
            $lastFailure = $_.Exception.Message
            $lastSignature = $null
            $stableSamples = 0
            if ($lastFailure -notmatch 'found 0|Could not read the complete \.data section' -or
                $Process.HasExited) {
                throw
            }
            Write-Host "Reflection data is not ready; retrying in $RetryIntervalSeconds seconds..."
        }
        if ([DateTime]::UtcNow -lt $deadline) {
            Start-Sleep -Seconds $RetryIntervalSeconds
        }
    } while ([DateTime]::UtcNow -lt $deadline)

    throw "Reflection data did not become ready within $ReflectionTimeoutSeconds seconds. Last error: $lastFailure"
}

function Invoke-JmapDumper {
    param(
        [Parameter(Mandatory = $true)]
        [string]$ExecutablePath,

        [Parameter(Mandatory = $true)]
        [int]$TargetProcessId,

        [Parameter(Mandatory = $true)]
        [string]$GuObjectArray,

        [Parameter(Mandatory = $true)]
        [string]$MappingsPath,

        [Parameter(Mandatory = $true)]
        [string]$StdoutPath,

        [Parameter(Mandatory = $true)]
        [string]$StderrPath
    )

    $startInfo = [Diagnostics.ProcessStartInfo]::new()
    $startInfo.FileName = $ExecutablePath
    $startInfo.UseShellExecute = $false
    $startInfo.CreateNoWindow = $true
    $startInfo.RedirectStandardOutput = $true
    $startInfo.RedirectStandardError = $true
    $startInfo.EnvironmentVariables['JMAP_CONCURRENCY'] = [string]$Concurrency
    $dumperArguments = @(
        '--pid', [string]$TargetProcessId,
        '--guobject-array', $GuObjectArray,
        '--engine-version', $engineVersion,
        '--all',
        $MappingsPath
    )
    # ProcessStartInfo.ArgumentList is unavailable in Windows PowerShell 5.1.
    # Escape backslashes before quotes and the closing quote using Win32 argv rules.
    $quotedArguments = foreach ($argument in $dumperArguments) {
        '"' + ([regex]::Replace(
            [regex]::Replace($argument, '(\\*)"', '$1$1\"'),
            '(\\+)$', '$1$1')) + '"'
    }
    $startInfo.Arguments = $quotedArguments -join ' '

    $dumperProcess = [Diagnostics.Process]::new()
    $dumperProcess.StartInfo = $startInfo
    if (-not $dumperProcess.Start()) {
        throw "Could not start jmap dumper: $ExecutablePath"
    }
    $stdoutTask = $dumperProcess.StandardOutput.ReadToEndAsync()
    $stderrTask = $dumperProcess.StandardError.ReadToEndAsync()
    $completed = $dumperProcess.WaitForExit($DumpTimeoutSeconds * 1000)
    if (-not $completed) {
        try {
            $dumperProcess.Kill()
            $dumperProcess.WaitForExit()
        }
        catch {
            Write-Warning "Could not stop timed-out jmap process: $($_.Exception.Message)"
        }
    }

    $stdout = $stdoutTask.GetAwaiter().GetResult()
    $stderr = $stderrTask.GetAwaiter().GetResult()
    [IO.File]::WriteAllText($StdoutPath, $stdout)
    [IO.File]::WriteAllText($StderrPath, $stderr)

    if (-not $completed) {
        throw "jmap exceeded the $DumpTimeoutSeconds-second timeout. Logs: $StdoutPath, $StderrPath"
    }
    if ($dumperProcess.ExitCode -ne 0) {
        throw "jmap exited with code $($dumperProcess.ExitCode). Logs: $StdoutPath, $StderrPath"
    }
}

$gameProcess = Get-RequiredVoyageProcess
$fingerprintText = (& $fingerprintScript -GameRoot $GameRoot) -join [Environment]::NewLine
$fingerprint = $fingerprintText | ConvertFrom-Json
$steamBuildId = [string]$fingerprint.steam.buildId
if ($steamBuildId -notmatch '^\d+$') {
    throw 'The installed game has no valid Steam build ID.'
}

$resolvedGameRoot = (Resolve-Path -LiteralPath $GameRoot).Path
$gameExecutable = Join-Path $resolvedGameRoot 'Voyage\Binaries\Win64\VoyageSteam-Win64-Shipping.exe'
$dumper = Get-Dumper
$liveExecutable = $gameProcess.MainModule.FileName
if ([IO.Path]::GetFullPath($liveExecutable) -ine [IO.Path]::GetFullPath($gameExecutable)) {
    throw 'The running Voyage process is not from the selected game installation.'
}

Write-Host "Waiting for Voyage reflection data in PID $($gameProcess.Id)..."
$candidate = Wait-ForObjectArray -Process $gameProcess

$timestamp = [DateTime]::UtcNow.ToString('yyyyMMddTHHmmssZ')
$runName = "steam-$steamBuildId-jmap-$timestamp"
$runRoot = Join-Path ([IO.Path]::GetFullPath($OutputRoot)) $runName
if (Test-Path -LiteralPath $runRoot) {
    throw "Mappings output already exists: $runRoot"
}
[IO.Directory]::CreateDirectory($runRoot) | Out-Null

$fingerprintPath = Join-Path $runRoot 'game-fingerprint.json'
$mappingsPath = Join-Path $runRoot 'Mappings.usmap'
$manifestPath = Join-Path $runRoot 'mapping-manifest.json'
$scanPath = Join-Path $runRoot 'reflection-scan.json'
$stdoutPath = Join-Path $runRoot 'jmap.stdout.log'
$stderrPath = Join-Path $runRoot 'jmap.stderr.log'
[IO.File]::WriteAllText(
    $fingerprintPath,
    (($fingerprint | ConvertTo-Json -Depth 6) + [Environment]::NewLine))
$scanEvidence = [ordered]@{
    kind = 'Voyage reflection readiness evidence'
    processId = $gameProcess.Id
    processStartedAtUtc = $gameProcess.StartTime.ToUniversalTime().ToString('o')
    minimumProcessAgeSeconds = $MinimumProcessAgeSeconds
    stableSampleCount = $StableSampleCount
    candidate = $candidate
    capturedAtUtc = [DateTime]::UtcNow.ToString('o')
}
[IO.File]::WriteAllText(
    $scanPath,
    (($scanEvidence | ConvertTo-Json -Depth 5) + [Environment]::NewLine))

Write-Host "Dumping complete UE $engineVersion mappings to $mappingsPath"
Invoke-JmapDumper `
    -ExecutablePath $dumper `
    -TargetProcessId $gameProcess.Id `
    -GuObjectArray ([string]$candidate.guObjectArray) `
    -MappingsPath $mappingsPath `
    -StdoutPath $stdoutPath `
    -StderrPath $stderrPath

if (-not (Test-Path -LiteralPath $mappingsPath -PathType Leaf)) {
    throw "jmap completed without producing mappings: $mappingsPath"
}
$mappingItem = Get-Item -LiteralPath $mappingsPath
$mappingCopyDigest = (Get-FileHash -Algorithm SHA256 -LiteralPath $mappingsPath).Hash
$mappingManifest = [ordered]@{
    kind = 'Voyage reflection mappings'
    steamBuildId = $steamBuildId
    engineVersion = $engineVersion
    dumper = 'ihsoft/jmap Voyage UE 5.8 compatibility fork'
    resolver = 'Find-VoyageUObjectArray.ps1 structural .data scan plus explicit GUObjectArray'
    liveProcessId = $gameProcess.Id
    imageBase = [string]$candidate.imageBase
    guObjectArray = [string]$candidate.guObjectArray
    observedObjectCount = [int]$candidate.numElements
    observedChunkCount = [int]$candidate.numChunks
    observedMaximumChunkCount = [int]$candidate.maxChunks
    reflectionScan = (Split-Path -Leaf $scanPath)
    minimumProcessAgeSeconds = $MinimumProcessAgeSeconds
    stableSampleCount = $StableSampleCount
    concurrency = $Concurrency
    mappingFile = $mappingItem.Name
    mappingLength = $mappingItem.Length
    stdoutLog = (Split-Path -Leaf $stdoutPath)
    stderrLog = (Split-Path -Leaf $stderrPath)
    generatedAtUtc = [DateTime]::UtcNow.ToString('o')
}
[IO.File]::WriteAllText(
    $manifestPath,
    (($mappingManifest | ConvertTo-Json -Depth 5) + [Environment]::NewLine))

$validation = & $testMappingsScript `
    -MappingsPath $mappingsPath `
    -ManifestPath $manifestPath `
    -ExpectedSteamBuildId $steamBuildId

$postDumpFingerprint = (& $fingerprintScript -GameRoot $GameRoot | ConvertFrom-Json)
if ([string]$postDumpFingerprint.steam.buildId -cne $steamBuildId) {
    throw 'The installed Steam build changed while mappings were generated.'
}

$uassetGuiPath = $null
if ($InstallForUAssetGUI) {
    if ([string]::IsNullOrWhiteSpace($env:LOCALAPPDATA)) {
        throw 'LOCALAPPDATA is unavailable; cannot install mappings for UAssetGUI.'
    }
    $uassetGuiDirectory = Join-Path $env:LOCALAPPDATA 'UAssetGUI\Mappings'
    [IO.Directory]::CreateDirectory($uassetGuiDirectory) | Out-Null
    $uassetGuiPath = Join-Path $uassetGuiDirectory "Voyage-$steamBuildId.usmap"
    if (Test-Path -LiteralPath $uassetGuiPath -PathType Leaf) {
        $existingHash = (Get-FileHash -Algorithm SHA256 -LiteralPath $uassetGuiPath).Hash
        if ($existingHash -cne $mappingCopyDigest) {
            $backupPath = Join-Path $uassetGuiDirectory "Voyage-$steamBuildId.backup-$timestamp.usmap"
            Copy-Item -LiteralPath $uassetGuiPath -Destination $backupPath
            Write-Host "Preserved previous UAssetGUI mapping: $backupPath"
        }
    }
    Copy-Item -LiteralPath $mappingsPath -Destination $uassetGuiPath -Force
    if ((Get-FileHash -Algorithm SHA256 -LiteralPath $uassetGuiPath).Hash -cne $mappingCopyDigest) {
        throw "Installed UAssetGUI mapping failed hash verification: $uassetGuiPath"
    }
}

[pscustomobject]@{
    result = 'success'
    gameProcessId = $gameProcess.Id
    mappingsPath = (Resolve-Path -LiteralPath $mappingsPath).Path
    manifestPath = (Resolve-Path -LiteralPath $manifestPath).Path
    length = [long]$validation.length
    steamBuildId = $steamBuildId
    uassetGuiPath = $uassetGuiPath
}
