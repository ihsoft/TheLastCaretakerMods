param(
    [string]$SourceRoot = (Join-Path $PSScriptRoot '..\.tools\jmap'),

    [string]$OutputPath = (Join-Path $PSScriptRoot '..\.tools\bin\jmap_dumper.exe')
)

$ErrorActionPreference = 'Stop'
Set-StrictMode -Version Latest

$expectedSourceCommit = '4f88d8af758712839529f9eeeb02b82c9469e271'
$source = (Resolve-Path -LiteralPath $SourceRoot).Path
$sourceGitPath = $source.Replace('\', '/')
$destination = [IO.Path]::GetFullPath($OutputPath)
$destinationDirectory = [IO.Path]::GetDirectoryName($destination)
$manifestPath = [IO.Path]::ChangeExtension($destination, '.manifest.json')

$actualCommit = (& git -c "safe.directory=$sourceGitPath" -C $source rev-parse HEAD).Trim()
if ($LASTEXITCODE -ne 0 -or $actualCommit -cne $expectedSourceCommit) {
    throw "jmap must be at the validated commit $expectedSourceCommit."
}
$sourceChanges = @(& git -c "safe.directory=$sourceGitPath" -C $source status --porcelain --untracked-files=no)
if ($LASTEXITCODE -ne 0 -or $sourceChanges.Count -ne 0) {
    throw 'Refusing to publish jmap from tracked source changes.'
}

if ((Test-Path -LiteralPath $destination -PathType Leaf) -and
    (Test-Path -LiteralPath $manifestPath -PathType Leaf)) {
    try {
        $existingManifest = Get-Content -Raw -LiteralPath $manifestPath | ConvertFrom-Json
        $existingHash = (Get-FileHash -Algorithm SHA256 -LiteralPath $destination).Hash
        if ([string]$existingManifest.kind -ceq 'Voyage canonical jmap mappings dumper' -and
            [string]$existingManifest.sourceCommit -ceq $actualCommit -and
            [string]$existingManifest.executableSha256 -ceq $existingHash) {
            [pscustomobject]@{
                Path = (Resolve-Path -LiteralPath $destination).Path
                Length = (Get-Item -LiteralPath $destination).Length
                Sha256 = $existingHash
                SourceCommit = $actualCommit
                Rebuilt = $false
            }
            return
        }
    }
    catch {
        Write-Warning "Existing jmap publish manifest is invalid; rebuilding. $($_.Exception.Message)"
    }
}

$repositoryRoot = (Resolve-Path -LiteralPath (Join-Path $PSScriptRoot '..')).Path
$buildParent = Join-Path $repositoryRoot 'artifacts\tools'
[IO.Directory]::CreateDirectory($buildParent) | Out-Null
$buildDirectory = Join-Path $buildParent ('jmap-publish-' + [Guid]::NewGuid().ToString('N'))
$candidate = $destination + '.new'
$candidateManifest = $manifestPath + '.new'

try {
    & (Join-Path $PSScriptRoot 'Build-JmapVoyageMappingsDumper.ps1') `
        -SourceRoot $source `
        -OutputRoot $buildDirectory
    if ($LASTEXITCODE -ne 0) {
        throw "jmap mappings dumper build failed with exit code $LASTEXITCODE."
    }

    $builtExecutable = Join-Path $buildDirectory 'target\release\jmap_dumper.exe'
    $buildManifestPath = Join-Path $buildDirectory 'build-manifest.json'
    if (-not (Test-Path -LiteralPath $builtExecutable -PathType Leaf) -or
        -not (Test-Path -LiteralPath $buildManifestPath -PathType Leaf)) {
        throw 'jmap build did not produce the expected executable and manifest.'
    }

    $versionText = ((& $builtExecutable --version) -join ' ').Trim()
    if ($LASTEXITCODE -ne 0 -or [string]::IsNullOrWhiteSpace($versionText)) {
        throw 'Published jmap candidate failed its version smoke test.'
    }

    [IO.Directory]::CreateDirectory($destinationDirectory) | Out-Null
    Copy-Item -LiteralPath $builtExecutable -Destination $candidate -Force
    $candidateHash = (Get-FileHash -Algorithm SHA256 -LiteralPath $candidate).Hash
    $buildManifest = Get-Content -Raw -LiteralPath $buildManifestPath | ConvertFrom-Json
    if ([string]$buildManifest.kind -cne 'Voyage jmap mappings dumper build' -or
        [string]$buildManifest.sourceCommit -cne $actualCommit -or
        [string]$buildManifest.executableSha256 -cne $candidateHash) {
        throw 'jmap candidate does not match its build manifest.'
    }

    $stableManifest = [ordered]@{
        kind = 'Voyage canonical jmap mappings dumper'
        sourceCommit = $actualCommit
        sourceDescribe = [string]$buildManifest.sourceDescribe
        upstreamBaseCommit = [string]$buildManifest.upstreamBaseCommit
        fixesAfterRelease = @($buildManifest.fixesAfterRelease)
        versionOutput = $versionText
        executableFile = [IO.Path]::GetFileName($destination)
        executableLength = (Get-Item -LiteralPath $candidate).Length
        executableSha256 = $candidateHash
        publishedAtUtc = [DateTime]::UtcNow.ToString('o')
    }
    [IO.File]::WriteAllText(
        $candidateManifest,
        (($stableManifest | ConvertTo-Json -Depth 4) + [Environment]::NewLine))

    Move-Item -LiteralPath $candidate -Destination $destination -Force
    Move-Item -LiteralPath $candidateManifest -Destination $manifestPath -Force
    $installedHash = (Get-FileHash -Algorithm SHA256 -LiteralPath $destination).Hash
    if ($installedHash -cne $candidateHash) {
        throw 'Canonical jmap binary failed post-install hash verification.'
    }

    [pscustomobject]@{
        Path = $destination
        Length = (Get-Item -LiteralPath $destination).Length
        Sha256 = $installedHash
        SourceCommit = $actualCommit
        Rebuilt = $true
    }
}
finally {
    foreach ($temporaryFile in @($candidate, $candidateManifest)) {
        if (Test-Path -LiteralPath $temporaryFile -PathType Leaf) {
            Remove-Item -LiteralPath $temporaryFile -Force
        }
    }
    if (Test-Path -LiteralPath $buildDirectory -PathType Container) {
        $resolvedBuild = (Resolve-Path -LiteralPath $buildDirectory).Path
        $allowedParent = [IO.Path]::GetFullPath($buildParent) + [IO.Path]::DirectorySeparatorChar
        if (-not $resolvedBuild.StartsWith($allowedParent, [StringComparison]::OrdinalIgnoreCase)) {
            throw "Refusing to clean jmap output outside artifacts: $resolvedBuild"
        }
        Remove-Item -LiteralPath $resolvedBuild -Recurse -Force
    }
}
