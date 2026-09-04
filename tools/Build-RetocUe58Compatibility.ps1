param(
    [string]$SourceRoot,

    [string]$CargoHome = 'R:\Codex\ToolCache\rust-retoc-master\cargo',

    [string]$RustupHome = 'R:\Codex\ToolCache\rust-retoc-master\rustup',

    [Parameter(Mandatory = $true)]
    [string]$OutputRoot
)

$ErrorActionPreference = 'Stop'
if ([string]::IsNullOrWhiteSpace($SourceRoot)) { $SourceRoot = Join-Path $PSScriptRoot '..\.tools\retoc' }

$expectedSourceCommit = '49b772135ddb967dc56795d311bd88fe81929f63'
$expectedSourceDescriptions = @('v0.1.5-rc2-1-g49b7721', 'v0.1.5-rc3')
$upstreamBaseCommit = '885a8dae740cb1ce1e41ff2e74f67f9f0c118237'
$upstreamLegacyAssetSha256 = '25E9859096C656CE36D35DF87599302EF0CE0847881FD5D64EDEBBE096D9BAAE'
$expectedLegacyAssetSha256 = 'AC4D4C7EE4C55914BD428D4C76232334570833F5E81BBF2F9972FBDF848D71EC'
$source = (Resolve-Path -LiteralPath $SourceRoot).Path
$cargo = (Resolve-Path -LiteralPath (Join-Path $CargoHome 'bin\cargo.exe')).Path
$rustup = (Resolve-Path -LiteralPath $RustupHome).Path
$output = [IO.Path]::GetFullPath($OutputRoot)
if (Test-Path -LiteralPath $output) {
    throw "OutputRoot already exists: $output"
}

$sourceGitPath = $source.Replace('\', '/')
$actualSourceCommit = (& git -c "safe.directory=$sourceGitPath" -C $source rev-parse HEAD).Trim()
if ($LASTEXITCODE -ne 0 -or $actualSourceCommit -cne $expectedSourceCommit) {
    throw "The retoc source is not the reviewed fork commit $expectedSourceCommit."
}
$actualSourceDescribe = (& git -c "safe.directory=$sourceGitPath" -C $source describe --tags --always).Trim()
if ($LASTEXITCODE -ne 0 -or $expectedSourceDescriptions -cnotcontains $actualSourceDescribe) {
    throw "Unexpected retoc source description: $actualSourceDescribe"
}
$sourceChanges = @(& git -c "safe.directory=$sourceGitPath" -C $source status --porcelain --untracked-files=no)
if ($LASTEXITCODE -ne 0 -or $sourceChanges.Count -ne 0) {
    throw 'The reviewed retoc source has tracked working-tree changes.'
}

$sourceLegacyAsset = Join-Path $source 'retoc\src\legacy_asset.rs'
$actualLegacyAssetSha256 = (Get-FileHash -Algorithm SHA256 -LiteralPath $sourceLegacyAsset).Hash
if ($actualLegacyAssetSha256 -cne $expectedLegacyAssetSha256) {
    throw 'The retoc legacy_asset.rs source does not match the reviewed fork commit.'
}

[IO.Directory]::CreateDirectory($output) | Out-Null

Copy-Item -LiteralPath `
    (Join-Path $source 'Cargo.toml'), `
    (Join-Path $source 'Cargo.lock'), `
    (Join-Path $source 'rustfmt.toml') `
    -Destination $output
Copy-Item -LiteralPath `
    (Join-Path $source 'retoc'), `
    (Join-Path $source 'retoc_cli'), `
    (Join-Path $source 'load_logger') `
    -Destination $output `
    -Recurse

$previousCargoHome = $env:CARGO_HOME
$previousRustupHome = $env:RUSTUP_HOME
try {
    $env:CARGO_HOME = (Resolve-Path -LiteralPath $CargoHome).Path
    $env:RUSTUP_HOME = $rustup
    Push-Location $output
    try {
        & $cargo build --release --package retoc_cli
        if ($LASTEXITCODE -ne 0) {
            throw "retoc compatibility build failed with exit code $LASTEXITCODE"
        }
    }
    finally {
        Pop-Location
    }
}
finally {
    $env:CARGO_HOME = $previousCargoHome
    $env:RUSTUP_HOME = $previousRustupHome
}

$executable = Join-Path $output 'target\release\retoc.exe'
if (-not (Test-Path -LiteralPath $executable -PathType Leaf)) {
    throw "Expected retoc executable was not produced: $executable"
}
$manifest = [ordered]@{
    kind = 'retoc UE 5.8 FObjectImport compatibility build'
    upstreamTag = 'v0.1.5'
    sourceDescribe = $actualSourceDescribe
    sourceCommit = $actualSourceCommit
    upstreamBaseCommit = $upstreamBaseCommit
    upstreamLegacyAssetSha256 = $upstreamLegacyAssetSha256
    patchedLegacyAssetSha256 = $actualLegacyAssetSha256
    executableSha256 = (Get-FileHash -Algorithm SHA256 -LiteralPath $executable).Hash
    generatedAtUtc = [DateTime]::UtcNow.ToString('o')
}
[IO.File]::WriteAllText(
    (Join-Path $output 'compatibility-manifest.json'),
    (($manifest | ConvertTo-Json -Depth 3) + [Environment]::NewLine))

Write-Host "Built UE 5.8-compatible retoc: $executable"
