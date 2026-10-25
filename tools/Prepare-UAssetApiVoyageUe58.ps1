param(
    [string]$SourceRoot,

    [Parameter(Mandatory = $true)]
    [string]$OutputRoot
)

$ErrorActionPreference = 'Stop'
if ([string]::IsNullOrWhiteSpace($SourceRoot)) { $SourceRoot = Join-Path $PSScriptRoot '..\.tools\UAssetAPI' }

$source = (Resolve-Path -LiteralPath $SourceRoot).Path
$output = [IO.Path]::GetFullPath($OutputRoot)
if (Test-Path -LiteralPath $output) {
    throw "OutputRoot already exists: $output"
}

$sourceGitPath = $source.Replace('\', '/')
$sourceChanges = @(& git -c "safe.directory=$sourceGitPath" -C $source status --porcelain --untracked-files=no)
if ($LASTEXITCODE -ne 0 -or $sourceChanges.Count -ne 0) {
    throw 'The reviewed UAssetAPI source has tracked working-tree changes.'
}

[IO.Directory]::CreateDirectory($output) | Out-Null
$trackedFiles = @(& git -c "safe.directory=$sourceGitPath" -C $source ls-files -- UAssetAPI README.md NOTICE.md LICENSE)
if ($LASTEXITCODE -ne 0 -or $trackedFiles.Count -eq 0) {
    throw 'Could not enumerate the reviewed UAssetAPI source files.'
}
foreach ($relativePath in $trackedFiles) {
    $destination = Join-Path $output $relativePath
    [IO.Directory]::CreateDirectory((Split-Path -Parent $destination)) | Out-Null
    Copy-Item -LiteralPath (Join-Path $source $relativePath) -Destination $destination
}

$manifest = [ordered]@{
    kind = 'Voyage UE 5.8 UAssetAPI compatibility source'
    officialSupportThrough = 'UE 5.7'
    compatibilityChanges = @(
        'Serialize filtered FObjectImport.PackageName only for explicitly selected UE 5.8 assets'
        'Handle filtered cooked FField layout'
        'Preserve the requested engine version while loading dependency schemas'
        'Load complete Blueprint parent schema chains and retry incomplete dependency walks'
        'Preserve RigHierarchy and RigVM exports through explicit native-serialization passthrough'
        'Read and write the proven empty InstancedPropertyBag native representation'
        'Preserve numeric values for imported native enums absent from mappings'
    )
    generatedAtUtc = [DateTime]::UtcNow.ToString('o')
}
[IO.File]::WriteAllText(
    (Join-Path $output 'compatibility-manifest.json'),
    (($manifest | ConvertTo-Json -Depth 3) + [Environment]::NewLine))

Write-Host "Prepared Voyage-compatible UAssetAPI source: $output"
