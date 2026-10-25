param(
    [string]$SourceRoot,

    [string]$CargoHome = 'R:\Codex\ToolCache\rust-retoc-master\cargo',

    [string]$RustupHome = 'R:\Codex\ToolCache\rust-retoc-master\rustup',

    [Parameter(Mandatory = $true)]
    [string]$OutputRoot
)

$ErrorActionPreference = 'Stop'
if ([string]::IsNullOrWhiteSpace($SourceRoot)) { $SourceRoot = Join-Path $PSScriptRoot '..\.tools\retoc' }

$source = (Resolve-Path -LiteralPath $SourceRoot).Path
$cargo = (Resolve-Path -LiteralPath (Join-Path $CargoHome 'bin\cargo.exe')).Path
$rustup = (Resolve-Path -LiteralPath $RustupHome).Path
$output = [IO.Path]::GetFullPath($OutputRoot)
if (Test-Path -LiteralPath $output) {
    throw "OutputRoot already exists: $output"
}

$sourceGitPath = $source.Replace('\', '/')
$sourceChanges = @(& git -c "safe.directory=$sourceGitPath" -C $source status --porcelain --untracked-files=no)
if ($LASTEXITCODE -ne 0 -or $sourceChanges.Count -ne 0) {
    throw 'The reviewed retoc source has tracked working-tree changes.'
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
    executable = $executable
    generatedAtUtc = [DateTime]::UtcNow.ToString('o')
}
[IO.File]::WriteAllText(
    (Join-Path $output 'compatibility-manifest.json'),
    (($manifest | ConvertTo-Json -Depth 3) + [Environment]::NewLine))

Write-Host "Built UE 5.8-compatible retoc: $executable"
