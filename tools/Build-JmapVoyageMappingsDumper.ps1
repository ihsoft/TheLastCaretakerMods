param(
    [string]$SourceRoot = (Join-Path $PSScriptRoot '..\.tools\jmap'),

    [string]$CargoHome = 'R:\Codex\ToolCache\rust-retoc-master\cargo',

    [string]$RustupHome = 'R:\Codex\ToolCache\rust-retoc-master\rustup',

    [Parameter(Mandatory = $true)]
    [string]$OutputRoot
)

$ErrorActionPreference = 'Stop'

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
    throw 'The reviewed jmap source has tracked working-tree changes.'
}

[IO.Directory]::CreateDirectory($output) | Out-Null
Get-ChildItem -LiteralPath $source -Force | Where-Object {
    $_.Name -notin @('.git', 'target')
} | ForEach-Object {
    Copy-Item -LiteralPath $_.FullName -Destination $output -Recurse
}

$previousCargoHome = $env:CARGO_HOME
$previousRustupHome = $env:RUSTUP_HOME
try {
    $env:CARGO_HOME = (Resolve-Path -LiteralPath $CargoHome).Path
    $env:RUSTUP_HOME = $rustup
    Push-Location $output
    try {
        & $cargo build --release --package jmap_dumper
        if ($LASTEXITCODE -ne 0) {
            throw "jmap_dumper build failed with exit code $LASTEXITCODE"
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

$executable = Join-Path $output 'target\release\jmap_dumper.exe'
if (-not (Test-Path -LiteralPath $executable -PathType Leaf)) {
    throw "Expected jmap_dumper executable was not produced: $executable"
}
$manifest = [ordered]@{
    kind = 'Voyage jmap mappings dumper build'
    fixesAfterRelease = @(
        'UE5.6+ UStruct.MinAlignment width'
        'Nullable enum, interface, and object Blueprint metadata'
        'Nullable soft-class and soft-object Blueprint metadata'
    )
    executable = $executable
    generatedAtUtc = [DateTime]::UtcNow.ToString('o')
}
[IO.File]::WriteAllText(
    (Join-Path $output 'build-manifest.json'),
    (($manifest | ConvertTo-Json -Depth 4) + [Environment]::NewLine))

Write-Host "Built reviewed jmap_dumper: $executable"
