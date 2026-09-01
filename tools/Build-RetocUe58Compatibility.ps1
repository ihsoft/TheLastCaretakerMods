param(
    [string]$SourceRoot = 'R:\Codex\ToolCache\rust-retoc-master\source',

    [string]$CargoHome = 'R:\Codex\ToolCache\rust-retoc-master\cargo',

    [string]$RustupHome = 'R:\Codex\ToolCache\rust-retoc-master\rustup',

    [Parameter(Mandatory = $true)]
    [string]$OutputRoot
)

$ErrorActionPreference = 'Stop'

$expectedLegacyAssetSha256 = '25E9859096C656CE36D35DF87599302EF0CE0847881FD5D64EDEBBE096D9BAAE'
$source = (Resolve-Path -LiteralPath $SourceRoot).Path
$cargo = (Resolve-Path -LiteralPath (Join-Path $CargoHome 'bin\cargo.exe')).Path
$rustup = (Resolve-Path -LiteralPath $RustupHome).Path
$output = [IO.Path]::GetFullPath($OutputRoot)
if (Test-Path -LiteralPath $output) {
    throw "OutputRoot already exists: $output"
}
[IO.Directory]::CreateDirectory($output) | Out-Null

$sourceLegacyAsset = Join-Path $source 'retoc\src\legacy_asset.rs'
$actualLegacyAssetSha256 = (Get-FileHash -Algorithm SHA256 -LiteralPath $sourceLegacyAsset).Hash
if ($actualLegacyAssetSha256 -cne $expectedLegacyAssetSha256) {
    throw 'The retoc legacy_asset.rs source does not match the reviewed 0.1.5 input.'
}

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

$legacyAsset = Join-Path $output 'retoc\src\legacy_asset.rs'
$text = [IO.File]::ReadAllText($legacyAsset).Replace("`r`n", "`n")
$oldReadBlock = @'
        // Used to support imports that live in their own packages for One File Per Actor in UE5
        // Such imports cannot exist in cooked data, and as such, we should never encounter them
        if !summary.is_filter_editor_only() {
            let _package_name: FMinimalName = s.de()?;
        }
'@
$newReadBlock = @'
        // FObjectImport::PackageName is serialized for every supported package
        // version. Filtered cooked packages write ObjectName as a placeholder
        // when PackageName is None; filtering changes the value, not the layout.
        let _package_name: FMinimalName = s.de()?;
'@
$oldWriteBlock = @'
        // We should never be serializing uncooked packages, might be worth to assert here instead of writing an empty name
        if !summary.is_filter_editor_only() {
            let package_name: FMinimalName = FMinimalName::default();
            s.ser(&package_name)?;
        }
'@
$newWriteBlock = @'
        // Match UE's filtered-cook placeholder: PackageName is still present
        // and uses ObjectName when no external package name is available.
        s.ser(&self.object_name)?;
'@
foreach ($oldBlock in @($oldReadBlock, $oldWriteBlock)) {
    if (-not $text.Contains($oldBlock)) {
        throw 'Reviewed retoc compatibility block was not found exactly once.'
    }
    if ($text.IndexOf($oldBlock) -ne $text.LastIndexOf($oldBlock)) {
        throw 'Reviewed retoc compatibility block occurs more than once.'
    }
}
$text = $text.Replace($oldReadBlock, $newReadBlock).Replace($oldWriteBlock, $newWriteBlock)
[IO.File]::WriteAllText($legacyAsset, $text)

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
    upstreamVersion = '0.1.5'
    upstreamLegacyAssetSha256 = $actualLegacyAssetSha256
    patchedLegacyAssetSha256 = (Get-FileHash -Algorithm SHA256 -LiteralPath $legacyAsset).Hash
    executableSha256 = (Get-FileHash -Algorithm SHA256 -LiteralPath $executable).Hash
    generatedAtUtc = [DateTime]::UtcNow.ToString('o')
}
[IO.File]::WriteAllText(
    (Join-Path $output 'compatibility-manifest.json'),
    (($manifest | ConvertTo-Json -Depth 3) + [Environment]::NewLine))

Write-Host "Built UE 5.8-compatible retoc: $executable"
