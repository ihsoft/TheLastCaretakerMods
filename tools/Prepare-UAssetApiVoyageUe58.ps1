param(
    [string]$SourceRoot = (Join-Path $PSScriptRoot '..\.tools\UAssetAPI'),

    [Parameter(Mandatory = $true)]
    [string]$OutputRoot
)

$ErrorActionPreference = 'Stop'

$expectedSourceCommit = '2943aa117aa805a0e3203a8befa5a89e2daaa6e3'
$expectedSourceDescribe = 'v1.1.0-84-g2943aa1'
$upstreamBaseCommit = '3228c1e86261aa08131f7ec0ff1a395f5d0b2a84'
$criticalSources = [ordered]@{
    'UAssetAPI\FieldTypes\FField.cs' = '2805C719525B62613C3A11166CAE56A62D5925FC9EE4AB4C0B8F41DA37F8E074'
    'UAssetAPI\Import.cs' = '36B0C55E02B7FDF8162FEDD7727226959CA77A0143AF2BCC2E6A9FD52E181052'
    'UAssetAPI\UAsset.cs' = '971C9CEBC0B034401045BE354A7CE29AC1423399F2DCB7DD4A07CEE040883E26'
    'UAssetAPI\MainSerializer.cs' = '6CF3BE1FC6AA80513CB4323D01FDB6873FB0CCF20C69F23991407CDBDC0AB4BE'
    'UAssetAPI\Unversioned\Usmap.cs' = '3CFCCC2A1CFAA2AB93B27316E79AA0191D2401DDAEEAB5545E6EA2BF4F8DDC27'
    'UAssetAPI\PropertyTypes\Objects\EnumPropertyData.cs' = 'DF7E1FBA2E0B26271CBEC4B9C0A3C9E3D6DAE8E31DDDD51173795F8214E23A36'
    'UAssetAPI\ExportTypes\CustomSerializedExport.cs' = '63E2B474D4759DB690F12CEDD8A48435E454887E43B3D8E6B8DFC1F5407EB5EA'
    'UAssetAPI\PropertyTypes\Structs\Core\InstancedPropertyBagPropertyData.cs' = '66696F635CBEBF507933560A6945DC17F0F3402F54F3A1EDC3C9E75A28A9A450'
}
$source = (Resolve-Path -LiteralPath $SourceRoot).Path
$output = [IO.Path]::GetFullPath($OutputRoot)
if (Test-Path -LiteralPath $output) {
    throw "OutputRoot already exists: $output"
}

$sourceGitPath = $source.Replace('\', '/')
$actualCommit = (& git -c "safe.directory=$sourceGitPath" -C $source rev-parse HEAD).Trim()
if ($LASTEXITCODE -ne 0 -or $actualCommit -cne $expectedSourceCommit) {
    throw "The UAssetAPI source is not the reviewed fork commit $expectedSourceCommit."
}
$actualDescribe = (& git -c "safe.directory=$sourceGitPath" -C $source describe --tags --always).Trim()
if ($LASTEXITCODE -ne 0 -or $actualDescribe -cne $expectedSourceDescribe) {
    throw "Unexpected UAssetAPI source description: $actualDescribe"
}
$sourceChanges = @(& git -c "safe.directory=$sourceGitPath" -C $source status --porcelain --untracked-files=no)
if ($LASTEXITCODE -ne 0 -or $sourceChanges.Count -ne 0) {
    throw 'The reviewed UAssetAPI source has tracked working-tree changes.'
}

foreach ($entry in $criticalSources.GetEnumerator()) {
    $actualHash = (Get-FileHash -Algorithm SHA256 -LiteralPath (Join-Path $source $entry.Key)).Hash
    if ($actualHash -cne $entry.Value) {
        throw "Reviewed UAssetAPI source hash changed: $($entry.Key)"
    }
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
    sourceDescribe = $actualDescribe
    sourceCommit = $actualCommit
    upstreamBaseCommit = $upstreamBaseCommit
    officialSupportThrough = 'UE 5.7'
    compatibilityChanges = @(
        'Always serialize FObjectImport.PackageName for the applicable object version'
        'Handle filtered cooked FField layout'
        'Preserve the requested engine version while loading dependency schemas'
        'Load complete Blueprint parent schema chains and retry incomplete dependency walks'
        'Preserve RigHierarchy and RigVM exports through explicit native-serialization passthrough'
        'Read and write the proven empty InstancedPropertyBag native representation'
        'Preserve numeric values for imported native enums absent from mappings'
    )
    criticalSourceSha256 = $criticalSources
    generatedAtUtc = [DateTime]::UtcNow.ToString('o')
}
[IO.File]::WriteAllText(
    (Join-Path $output 'compatibility-manifest.json'),
    (($manifest | ConvertTo-Json -Depth 3) + [Environment]::NewLine))

Write-Host "Prepared Voyage-compatible UAssetAPI source: $output"
