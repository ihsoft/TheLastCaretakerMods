param(
    [string]$SourceRoot,

    [string]$OutputPath
)

$ErrorActionPreference = 'Stop'
if ([string]::IsNullOrWhiteSpace($SourceRoot)) { $SourceRoot = Join-Path $PSScriptRoot '..\.tools\retoc' }
if ([string]::IsNullOrWhiteSpace($OutputPath)) { $OutputPath = Join-Path $PSScriptRoot '..\.tools\bin\retoc.exe' }
Set-StrictMode -Version Latest

$source = (Resolve-Path -LiteralPath $SourceRoot).Path
$sourceGitPath = $source.Replace('\', '/')
$destination = [IO.Path]::GetFullPath($OutputPath)
$destinationDirectory = [IO.Path]::GetDirectoryName($destination)
$manifestPath = [IO.Path]::ChangeExtension($destination, '.manifest.json')

$sourceChanges = @(& git -c "safe.directory=$sourceGitPath" -C $source status --porcelain --untracked-files=no)
if ($LASTEXITCODE -ne 0 -or $sourceChanges.Count -ne 0) {
    throw 'Refusing to publish retoc from tracked source changes.'
}

$repositoryRoot = (Resolve-Path -LiteralPath (Join-Path $PSScriptRoot '..')).Path
$buildParent = Join-Path $repositoryRoot 'artifacts\tools'
[IO.Directory]::CreateDirectory($buildParent) | Out-Null
$buildDirectory = Join-Path $buildParent ('retoc-publish-' + [Guid]::NewGuid().ToString('N'))
$candidate = $destination + '.new'
$candidateManifest = $manifestPath + '.new'

try {
    & (Join-Path $PSScriptRoot 'Build-RetocUe58Compatibility.ps1') `
        -SourceRoot $source `
        -OutputRoot $buildDirectory
    if ($LASTEXITCODE -ne 0) {
        throw "retoc compatibility build failed with exit code $LASTEXITCODE."
    }

    $builtExecutable = Join-Path $buildDirectory 'target\release\retoc.exe'
    $buildManifestPath = Join-Path $buildDirectory 'compatibility-manifest.json'
    if (-not (Test-Path -LiteralPath $builtExecutable -PathType Leaf) -or
        -not (Test-Path -LiteralPath $buildManifestPath -PathType Leaf)) {
        throw 'retoc build did not produce the expected executable and manifest.'
    }

    $versionText = ((& $builtExecutable --version) -join ' ').Trim()
    if ($LASTEXITCODE -ne 0 -or [string]::IsNullOrWhiteSpace($versionText)) {
        throw 'Published retoc candidate failed its version smoke test.'
    }

    [IO.Directory]::CreateDirectory($destinationDirectory) | Out-Null
    Copy-Item -LiteralPath $builtExecutable -Destination $candidate -Force
    $candidateHash = (Get-FileHash -Algorithm SHA256 -LiteralPath $candidate).Hash
    $buildManifest = Get-Content -Raw -LiteralPath $buildManifestPath | ConvertFrom-Json
    if ([string]$buildManifest.kind -cne 'retoc UE 5.8 FObjectImport compatibility build') {
        throw 'retoc candidate does not match its compatibility build manifest.'
    }

    $stableManifest = [ordered]@{
        kind = 'Voyage canonical retoc binary'
        versionOutput = $versionText
        executableFile = [IO.Path]::GetFileName($destination)
        executableLength = (Get-Item -LiteralPath $candidate).Length
        publishedAtUtc = [DateTime]::UtcNow.ToString('o')
    }
    [IO.File]::WriteAllText(
        $candidateManifest,
        (($stableManifest | ConvertTo-Json -Depth 4) + [Environment]::NewLine))

    Move-Item -LiteralPath $candidate -Destination $destination -Force
    Move-Item -LiteralPath $candidateManifest -Destination $manifestPath -Force
    $installedHash = (Get-FileHash -Algorithm SHA256 -LiteralPath $destination).Hash
    if ($installedHash -cne $candidateHash) {
        throw 'Canonical retoc binary failed post-install hash verification.'
    }

    [pscustomobject]@{
        Path = $destination
        Length = (Get-Item -LiteralPath $destination).Length
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
            throw "Refusing to clean retoc output outside artifacts: $resolvedBuild"
        }
        Remove-Item -LiteralPath $resolvedBuild -Recurse -Force
    }
}
