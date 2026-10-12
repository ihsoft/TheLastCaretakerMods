param(
    [string]$SourceRoot = (Join-Path $PSScriptRoot '..\.tools\jmap'),

    [string]$CargoHome = 'R:\Codex\ToolCache\rust-retoc-master\cargo',

    [string]$RustupHome = 'R:\Codex\ToolCache\rust-retoc-master\rustup',

    [Parameter(Mandatory = $true)]
    [string]$OutputRoot
)

$ErrorActionPreference = 'Stop'

$expectedSourceCommit = 'fad7b923d31e098d958bd35867f3ed0bedc637a2'
$expectedSourceDescribe = 'v0.2.0-rc2-1-gfad7b92'
$upstreamBaseCommit = '3f189715f08a646a8c341bf80c2fe06e44177ac3'
$expectedLibSha256 = '12E04B342E5030E88570F4B4235425F7F2F7C4F55E405E6511EC7B3DD5985E0B'
$expectedMemSha256 = '992AD2BAD423D9C118C8E7BBFE1B0FF5C1D7720DA20C8174820CAE6DF585E0F4'
$expectedObjectsSha256 = 'D3E46A422BCEB9EA7CD400B239CF0673F2EB2A0E867924E2717E15C95CBC9DB7'

$source = (Resolve-Path -LiteralPath $SourceRoot).Path
$cargo = (Resolve-Path -LiteralPath (Join-Path $CargoHome 'bin\cargo.exe')).Path
$rustup = (Resolve-Path -LiteralPath $RustupHome).Path
$output = [IO.Path]::GetFullPath($OutputRoot)
if (Test-Path -LiteralPath $output) {
    throw "OutputRoot already exists: $output"
}

$sourceGitPath = $source.Replace('\', '/')
$actualCommit = (& git -c "safe.directory=$sourceGitPath" -C $source rev-parse HEAD).Trim()
if ($LASTEXITCODE -ne 0 -or $actualCommit -cne $expectedSourceCommit) {
    throw "The jmap source is not the reviewed fork commit $expectedSourceCommit."
}
$actualDescribe = (& git -c "safe.directory=$sourceGitPath" -C $source describe --tags --always).Trim()
if ($LASTEXITCODE -ne 0 -or $actualDescribe -cne $expectedSourceDescribe) {
    throw "Unexpected jmap source description: $actualDescribe"
}
$sourceChanges = @(& git -c "safe.directory=$sourceGitPath" -C $source status --porcelain --untracked-files=no)
if ($LASTEXITCODE -ne 0 -or $sourceChanges.Count -ne 0) {
    throw 'The reviewed jmap source has tracked working-tree changes.'
}

$criticalSources = [ordered]@{
    'jmap_dumper\src\lib.rs' = $expectedLibSha256
    'jmap_dumper\src\mem.rs' = $expectedMemSha256
    'jmap_dumper\src\objects.rs' = $expectedObjectsSha256
}
foreach ($entry in $criticalSources.GetEnumerator()) {
    $actualHash = (Get-FileHash -Algorithm SHA256 -LiteralPath (Join-Path $source $entry.Key)).Hash
    if ($actualHash -cne $entry.Value) {
        throw "Reviewed jmap source hash changed: $($entry.Key)"
    }
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
    sourceDescribe = $actualDescribe
    sourceCommit = $actualCommit
    upstreamBaseCommit = $upstreamBaseCommit
    fixesAfterRelease = @(
        'UE5.6+ UStruct.MinAlignment width'
        'Nullable enum, interface, and object Blueprint metadata'
        'Nullable soft-class and soft-object Blueprint metadata'
    )
    criticalSourceSha256 = $criticalSources
    executableSha256 = (Get-FileHash -Algorithm SHA256 -LiteralPath $executable).Hash
    generatedAtUtc = [DateTime]::UtcNow.ToString('o')
}
[IO.File]::WriteAllText(
    (Join-Path $output 'build-manifest.json'),
    (($manifest | ConvertTo-Json -Depth 4) + [Environment]::NewLine))

Write-Host "Built reviewed jmap_dumper: $executable"
