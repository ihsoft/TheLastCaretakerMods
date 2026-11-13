# Install an existing candidate and apply the owned INI migration without rebuilding.
[CmdletBinding()]
param(
    [Parameter(Mandatory)][string]$ReleaseManifest,
    [string]$GameRoot = 'P:/SteamLibrary/steamapps/common/Voyage',
    [switch]$AllowDirtySource
)
$ErrorActionPreference = 'Stop'
Set-StrictMode -Version Latest
$repo = (Resolve-Path -LiteralPath (Join-Path $PSScriptRoot '../..')).Path
Import-Module (Join-Path $repo 'tools/VoyageModPipeline.psm1')
$manifestPath = (Resolve-Path -LiteralPath $ReleaseManifest).Path
$record = Get-Content -LiteralPath $manifestPath -Raw | ConvertFrom-Json
if ($record.mod -cne 'Railgun') { throw 'Expected a Railgun release manifest' }
$plan = & (Join-Path $repo 'tools/Install-VoyageRelease.ps1') -ReleaseManifest $manifestPath -GameRoot $GameRoot -AllowDirtySource:$AllowDirtySource -ValidateOnly
if ($plan.paksDirectory -ne [IO.Path]::GetFullPath((Join-Path $GameRoot 'Voyage/Content/Paks'))) { throw 'Expected the Railgun Paks destination' }
if (Test-Path -LiteralPath (Join-Path $plan.paksDirectory 'Railgun.autoload')) { throw 'Legacy Railgun.autoload requires its reviewed recoverable migration before installation.' }
if (@(Get-Process -ErrorAction Stop | Where-Object { $_.ProcessName -in @('VoyageSteam-Win64-Shipping','Voyage') }).Count) { throw 'Voyage must be closed before installation.' }
$settingsTarget = Join-Path $plan.paksDirectory 'Railgun.ini'
if (Test-Path -LiteralPath $settingsTarget) {
    $item = Get-Item -LiteralPath $settingsTarget -Force
    if ($item.PSIsContainer -or ($item.Attributes -band [IO.FileAttributes]::ReparsePoint)) { throw 'Unsafe Railgun INI target' }
}
$preparation = New-VoyageModRun -ModRoot $PSScriptRoot -Kind install
$payload = Join-Path $preparation 'template'
[IO.Directory]::CreateDirectory($payload) | Out-Null
$templatePath = Join-Path $payload 'Railgun.ini'
$archive = [IO.Path]::GetFullPath((Join-Path (Split-Path -Parent $manifestPath) $record.archive.path))
Add-Type -AssemblyName System.IO.Compression.FileSystem
$zip = [IO.Compression.ZipFile]::OpenRead($archive)
try {
    $entries = @($zip.Entries | Where-Object FullName -CEQ 'Voyage/Content/Paks/Railgun.ini')
    if ($entries.Count -ne 1) { throw 'Candidate must contain exactly one Railgun.ini at its runtime path' }
    [IO.Compression.ZipFileExtensions]::ExtractToFile($entries[0], $templatePath)
} finally { $zip.Dispose() }
function Update-RailgunSettings([string]$TemplatePath, [string]$SettingsPath) {
    $tool = Join-Path $repo 'tools/Merge-VoyageModIni.ps1'
    $stage = $SettingsPath + '.railgun-' + [guid]::NewGuid().ToString('N') + '.tmp'
    $merged = $SettingsPath + '.railgun-' + [guid]::NewGuid().ToString('N') + '.tmp'
    $backup = $SettingsPath + '.railgun-' + [guid]::NewGuid().ToString('N') + '.bak'
    $source = $SettingsPath
    $migratedEnergy = $false
    try {
        $lines = [IO.File]::ReadAllLines($SettingsPath)
        $hasNewEnergy = @($lines | Where-Object { $_ -match '^\s*FullChargeEnergyKWh\s*=' }).Count -gt 0
        $defaultLine = @([IO.File]::ReadAllLines($TemplatePath) |
            Where-Object { $_ -match '^\s*FullChargeEnergyKWh\s*=' })
        $defaultEnergy = 0.0
        if ($defaultLine.Count -ne 1 -or
            -not [double]::TryParse(($defaultLine[0] -split '=', 2)[1].Trim(),
                [Globalization.NumberStyles]::Float,
                [Globalization.CultureInfo]::InvariantCulture, [ref]$defaultEnergy)) {
            throw 'Settings template has no valid FullChargeEnergyKWh default.'
        }
        $rewritten = New-Object 'System.Collections.Generic.List[string]'
        foreach ($line in $lines) {
            if ($line -match '^\s*FullChargeEnergyKJ\s*=\s*(.*?)\s*$') {
                $migratedEnergy = $true
                if (-not $hasNewEnergy) {
                    $legacy = 0.0
                    if (-not [double]::TryParse($matches[1].Trim(),
                        [Globalization.NumberStyles]::Float,
                        [Globalization.CultureInfo]::InvariantCulture, [ref]$legacy)) {
                        $legacy = 500.0
                    }
                    $converted = if ([Math]::Abs($legacy - 500.0) -lt 0.000001) {
                        $defaultEnergy
                    } else { $legacy / 1000.0 }
                    $value = $converted.ToString('0.###############',
                        [Globalization.CultureInfo]::InvariantCulture)
                    $rewritten.Add('FullChargeEnergyKWh=' + $value)
                    $hasNewEnergy = $true
                }
                continue
            }
            $rewritten.Add($line)
        }
        if ($migratedEnergy) {
            [IO.File]::WriteAllLines($stage, $rewritten,
                (New-Object System.Text.UTF8Encoding($false)))
            $source = $stage
        }
        $result = & $tool -TemplatePath $TemplatePath -CurrentPath $source `
            -OutputPath $merged -RetiredKeys @('OfflineDischargeKW','OpticsMousePercent') `
            -DefaultsComment '# Defaults added by a newer Railgun build.'
        if ($result.Changed -or $migratedEnergy) {
            if (@(Get-Process -Name 'VoyageSteam-Win64-Shipping','Voyage' `
                    -ErrorAction SilentlyContinue).Count -gt 0) {
                throw 'Game started; Railgun settings update refused.'
            }
            [IO.File]::Replace($merged, $SettingsPath, $backup, $true)
        }
        $added = @($result.AddedKeys)
        if ($migratedEnergy -and $added -cnotcontains 'FullChargeEnergyKWh') {
            $added = @('FullChargeEnergyKWh') + $added
        }
        return [pscustomobject]@{
            AddedKeys=$added; RemovedKeys=@($result.RemovedKeys)
        }
    } finally {
        foreach ($path in @($stage,$merged,$backup)) {
            if (Test-Path -LiteralPath $path) { Remove-Item -LiteralPath $path -Force }
        }
    }
}
$installation = & (Join-Path $repo 'tools/Install-VoyageRelease.ps1') `
    -ReleaseManifest $manifestPath -GameRoot $GameRoot -AllowDirtySource:$AllowDirtySource `
    -KeepRecoveryEvidence
$settingsPath = Join-Path $installation.paksDirectory 'Railgun.ini'
$settingsTemplatePath = $templatePath
$settingsEvidenceDirectory = Split-Path -Parent `
    $installation.installManifestPath
$settingsRecordPath = Join-Path $settingsEvidenceDirectory `
    'settings-installation.json'
$settingsBackupDirectory = Join-Path $settingsEvidenceDirectory `
    'settings-previous'
$settingsBackupPath = Join-Path $settingsBackupDirectory 'Railgun.ini'
$settingsExisted = Test-Path -LiteralPath $settingsPath -PathType Leaf
$settingsPreviousHash = if ($settingsExisted) {
    (Get-FileHash -LiteralPath $settingsPath -Algorithm SHA256).Hash
} else { $null }
$settingsBackupVerified = $false
try {
    if ($settingsExisted) {
        $null = New-Item -ItemType Directory -Path `
            $settingsBackupDirectory
        Copy-Item -LiteralPath $settingsPath `
            -Destination $settingsBackupPath
        if ((Get-FileHash -LiteralPath $settingsBackupPath `
                -Algorithm SHA256).Hash -cne $settingsPreviousHash) {
            throw 'Railgun settings backup hash mismatch.'
        }
        $settingsBackupVerified = $true
    }
    if (@(Get-Process -Name 'VoyageSteam-Win64-Shipping','Voyage' `
            -ErrorAction SilentlyContinue).Count -gt 0) {
        throw 'Game started after container installation; settings mutation refused.'
    }
    $settingsProcessCheckedAtUtc = [DateTime]::UtcNow.ToString('o')
    $addedKeys = @()
    $removedKeys = @()
    if (-not $settingsExisted) {
        $temporarySettingsPath = $settingsPath + '.railgun-' +
            [guid]::NewGuid().ToString('N') + '.tmp'
        try {
            Copy-Item -LiteralPath $settingsTemplatePath `
                -Destination $temporarySettingsPath
            [IO.File]::Move($temporarySettingsPath, $settingsPath)
        } finally {
            if (Test-Path -LiteralPath $temporarySettingsPath) {
                Remove-Item -LiteralPath $temporarySettingsPath -Force
            }
        }
        if ((Get-FileHash -LiteralPath $settingsPath `
                -Algorithm SHA256).Hash -cne
            (Get-FileHash -LiteralPath $settingsTemplatePath `
                -Algorithm SHA256).Hash) {
            throw 'Created Railgun settings readback hash mismatch.'
        }
    } else {
        $settingsUpdate = Update-RailgunSettings $settingsTemplatePath `
            $settingsPath
        $removedKeys = @($settingsUpdate.RemovedKeys)
        $addedKeys = @($settingsUpdate.AddedKeys)
    }
    if (@([IO.File]::ReadAllLines($settingsPath) | Where-Object {
            $_ -match '^\s*(?:OfflineDischargeKW|OpticsMousePercent)\s*='
        }).Count -ne 0) {
        throw 'A deprecated Railgun setting remains installed.'
    }
    $settingsInstallation = [pscustomobject]@{
        schemaVersion=1
        status='installed'
        path=$settingsPath
        created=(-not $settingsExisted)
        previousExisted=$settingsExisted
        previousSha256=$settingsPreviousHash
        previousBackupPath=$(if ($settingsExisted) {
            $settingsBackupPath
        } else { $null })
        addedKeys=$addedKeys
        removedKeys=$removedKeys
        processCheckedAtUtc=$settingsProcessCheckedAtUtc
        installedAtUtc=[DateTime]::UtcNow.ToString('o')
        releaseInstallManifestPath=$installation.installManifestPath
    }
    $settingsInstallation | ConvertTo-Json -Depth 5 |
        Set-Content -LiteralPath $settingsRecordPath -Encoding UTF8
    $settingsInstallation | Add-Member -NotePropertyName recordPath `
        -NotePropertyValue $settingsRecordPath
} catch {
    $settingsFailure = $_
    if (@(Get-Process -Name 'VoyageSteam-Win64-Shipping','Voyage' `
            -ErrorAction SilentlyContinue).Count -gt 0) {
        throw ('Railgun settings failed and the game is now running; ' +
            'installed containers were not restored. Actual error: ' +
            $settingsFailure.Exception.Message)
    }
    $settingsRollbackError = $null
    try {
        if ($settingsExisted -and $settingsBackupVerified) {
            Copy-Item -LiteralPath $settingsBackupPath `
                -Destination $settingsPath -Force
            if ((Get-FileHash -LiteralPath $settingsPath `
                    -Algorithm SHA256).Hash -cne $settingsPreviousHash) {
                throw 'Railgun settings rollback hash mismatch.'
            }
        } elseif (-not $settingsExisted -and
            (Test-Path -LiteralPath $settingsPath)) {
            Remove-Item -LiteralPath $settingsPath -Force
        }
    } catch {
        $settingsRollbackError = $_.Exception.Message
    }
    $releaseRollbackError = $null
    try {
        $null = & (Join-Path $repo `
            'tools/Restore-VoyageReleaseInstallation.ps1') `
            -InstallManifest $installation.installManifestPath
    } catch {
        $releaseRollbackError = $_.Exception.Message
    }
    if ($null -ne $settingsRollbackError -or
        $null -ne $releaseRollbackError) {
        throw ('Railgun settings/install recovery failed. Settings=' +
            [string]$settingsRollbackError + '; release=' +
            [string]$releaseRollbackError + '; original=' +
            $settingsFailure.Exception.Message)
    }
    throw ('Railgun settings failed; prior settings and release were ' +
        'restored. Actual error: ' + $settingsFailure.Exception.Message)
}
$restoreValidation = & (Join-Path $repo `
    'tools/Restore-VoyageReleaseInstallation.ps1') `
    -InstallManifest $installation.installManifestPath -ValidateOnly
if (-not [bool]$restoreValidation.validated) {
    throw 'Installed Railgun release did not pass restore validation.'
}
$temporaryInstallRoot = [IO.Path]::GetFullPath(
    (Join-Path $repo 'Tmp/Railgun/installations'))
$installationOperationRoot = [IO.Path]::GetFullPath(
    (Split-Path -Parent $installation.installManifestPath))
Remove-VoyageModScratch -ModRoot $PSScriptRoot -RunRoot $temporaryInstallRoot -Path $installationOperationRoot
$installation.installManifestPath = $null
$settingsInstallation = [pscustomobject]@{
    status = 'installed'
    path = $settingsPath
}
Write-Host 'Railgun installed successfully; container files were read back and settings are present.'
Remove-VoyageModScratch -ModRoot $PSScriptRoot -RunRoot $preparation -Path $payload
[pscustomobject]@{status='installed';releaseManifestPath=$manifestPath;settingsPath=$settingsPath;installation=$installation;settingsInstallation=$settingsInstallation;restoreValidation=$restoreValidation}
