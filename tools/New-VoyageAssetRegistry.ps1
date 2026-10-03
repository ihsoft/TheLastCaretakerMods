[CmdletBinding()]
param(
    [Parameter(Mandatory=$true)][string[]]$AssetJsonPath,
    [Parameter(Mandatory=$true)][string]$OutputPath,
    [string]$EngineRoot = 'K:\Epic Games\UE_5.8\Engine',
    [string]$EvidenceRoot = ''
)

$ErrorActionPreference = 'Stop'
Set-StrictMode -Version Latest
$repo = (Resolve-Path -LiteralPath (Join-Path $PSScriptRoot '..')).Path
$AssetJsonPath = @($AssetJsonPath | ForEach-Object {
    @(([string]$_).Split(';') | ForEach-Object { $_.Trim() } |
        Where-Object { $_ })
})
if ($AssetJsonPath.Count -lt 1) { throw 'At least one asset JSON path is required.' }
$writer = & (Join-Path $PSScriptRoot `
    'VoyageAssetRegistryWriter\Get-VoyageAssetRegistryWriter.ps1') `
    -EngineRoot $EngineRoot
$editor = $writer.Engine.editorPath
$output = [IO.Path]::GetFullPath($OutputPath)
if (-not $EvidenceRoot) {
    $EvidenceRoot = Join-Path $repo ('artifacts\tools\asset-registry-' +
        [DateTime]::UtcNow.ToString('yyyyMMdd-HHmmss-fff'))
}
$evidence = [IO.Path]::GetFullPath($EvidenceRoot)
$manifestPath = $output + '.manifest.json'
$sourceRoot = [IO.Path]::GetFullPath((Join-Path $PSScriptRoot 'VoyageAssetRegistryWriter'))
$publishedRoot = [IO.Path]::GetFullPath((Join-Path $repo `
    '.tools\bin\VoyageAssetRegistryWriter'))
$inputSnapshot = @($AssetJsonPath | ForEach-Object {
    $path = [IO.Path]::GetFullPath($_)
    if (-not (Test-Path -LiteralPath $path -PathType Leaf)) {
        throw "Asset JSON input is missing: $path"
    }
    [pscustomobject]@{
        path = $path
        length = (Get-Item -LiteralPath $path).Length
        sha256 = (Get-FileHash -LiteralPath $path -Algorithm SHA256).Hash
    }
})
foreach ($protected in @($inputSnapshot | ForEach-Object path)) {
    if ($output.Equals($protected, [StringComparison]::OrdinalIgnoreCase) -or
        $manifestPath.Equals($protected, [StringComparison]::OrdinalIgnoreCase)) {
        throw 'Output or manifest path collides with an asset JSON input.'
    }
}
foreach ($target in @($output,$manifestPath,$evidence)) {
    if ($target.StartsWith($sourceRoot + [IO.Path]::DirectorySeparatorChar,
            [StringComparison]::OrdinalIgnoreCase) -or
        $target.StartsWith($publishedRoot + [IO.Path]::DirectorySeparatorChar,
            [StringComparison]::OrdinalIgnoreCase)) {
        throw 'Output, manifest and evidence paths must not mutate writer source/publication.'
    }
}
if ($output -ceq $evidence -or $manifestPath -ceq $evidence -or
    $output.StartsWith($evidence + [IO.Path]::DirectorySeparatorChar,
        [StringComparison]::OrdinalIgnoreCase) -or
    $manifestPath.StartsWith($evidence + [IO.Path]::DirectorySeparatorChar,
        [StringComparison]::OrdinalIgnoreCase)) {
    throw 'Output and manifest must be outside EvidenceRoot.'
}
foreach ($input in $inputSnapshot) {
    if ($input.path.StartsWith($evidence + [IO.Path]::DirectorySeparatorChar,
            [StringComparison]::OrdinalIgnoreCase)) {
        throw 'EvidenceRoot contains an asset JSON input.'
    }
}
if (Test-Path -LiteralPath $evidence) {
    throw 'EvidenceRoot already exists; use a fresh evidence identity.'
}
$null = New-Item -ItemType Directory -Path $evidence
$metadata = Join-Path $evidence 'registry-metadata.json'
$candidate = Join-Path $evidence 'AssetRegistry.bin'
$writerSummaryPath = Join-Path $evidence 'writer-summary.json'
$log = Join-Path $evidence 'writer.log'
$executionHost = Join-Path $evidence 'writer-host'
Copy-Item -LiteralPath $publishedRoot -Destination $executionHost -Recurse
$executionProject = Join-Path $executionHost 'VoyageAssetRegistryWriter.uproject'

$metadataResult = & (Join-Path $PSScriptRoot `
    'VoyageAssetRegistryWriter\ConvertTo-VoyageAssetRegistryMetadata.ps1') `
    -AssetJsonPath $AssetJsonPath -OutputPath $metadata
$arguments = @(
    $executionProject,
    '-run=WriteVoyageAssetRegistry',
    ('-RegistryMetadata=' + $metadata),
    ('-OutputRegistry=' + $candidate),
    ('-OutputSummary=' + $writerSummaryPath),
    '-unattended','-nop4','-nosplash','-nullrhi',('-abslog=' + $log)
)
$savedPreference = $ErrorActionPreference
try {
    $ErrorActionPreference = 'Continue'
    & $editor @arguments *> (Join-Path $evidence 'writer-console.log')
    $writerExit = $LASTEXITCODE
} finally { $ErrorActionPreference = $savedPreference }
if ($writerExit -ne 0 -or -not (Test-Path -LiteralPath $candidate -PathType Leaf) -or
    -not (Test-Path -LiteralPath $writerSummaryPath -PathType Leaf)) {
    throw "AssetRegistry writer failed ($writerExit). Log: $log"
}
foreach ($input in $inputSnapshot) {
    if (-not (Test-Path -LiteralPath $input.path -PathType Leaf) -or
        (Get-Item -LiteralPath $input.path).Length -ne $input.length -or
        (Get-FileHash -LiteralPath $input.path -Algorithm SHA256).Hash -cne $input.sha256) {
        throw "Asset JSON input changed during generation: $($input.path)"
    }
}
$writerSummary = Get-Content -LiteralPath $writerSummaryPath -Raw | ConvertFrom-Json
if (-not [bool]$writerSummary.reopenVerified -or
    [int]$writerSummary.assetCount -ne [int]$metadataResult.assetCount) {
    throw 'AssetRegistry writer did not verify the expected records.'
}

$candidateHash = (Get-FileHash -LiteralPath $candidate -Algorithm SHA256).Hash
$manifest = [ordered]@{
    schemaVersion = 1
    kind = 'Voyage generated AssetRegistry'
    createdAtUtc = [DateTime]::UtcNow.ToString('o')
    outputPath = $output
    length = (Get-Item -LiteralPath $candidate).Length
    sha256 = $candidateHash
    formatVersion = [int]$writerSummary.formatVersion
    filterEditorOnly = [bool]$writerSummary.filterEditorOnly
    assetCount = [int]$writerSummary.assetCount
    packageCount = [int]$writerSummary.packageCount
    reopenVerified = [bool]$writerSummary.reopenVerified
    primaryAssetIds = @($writerSummary.assets | ForEach-Object {
        [string]$_.primaryAssetId
    })
    inputFiles = $inputSnapshot
    writerManifest = $writer.ManifestPath
    writerInputFingerprint = $writer.InputFingerprint
    voyagePolicyPath = Join-Path $PSScriptRoot `
        'VoyageAssetRegistryWriter\voyage-policy.json'
    voyagePolicySha256 = (Get-FileHash -LiteralPath (Join-Path $PSScriptRoot `
        'VoyageAssetRegistryWriter\voyage-policy.json') -Algorithm SHA256).Hash
    evidenceRoot = $evidence
    writerLog = $log
    metadataPath = $metadata
    writerSummaryPath = $writerSummaryPath
}
$candidateManifest = Join-Path $evidence 'output-manifest.json'
[IO.File]::WriteAllText($candidateManifest, ($manifest | ConvertTo-Json -Depth 8),
    (New-Object Text.UTF8Encoding($false)))
$outputParent = Split-Path -Parent $output
if (-not (Test-Path -LiteralPath $outputParent -PathType Container)) {
    $null = New-Item -ItemType Directory -Path $outputParent -Force
}
$suffix = [Guid]::NewGuid().ToString('N')
$stagedOutput = $output + '.new-' + $suffix
$stagedManifest = $manifestPath + '.new-' + $suffix
$previousOutput = Join-Path $evidence 'previous-AssetRegistry.bin'
$previousManifest = Join-Path $evidence 'previous-output-manifest.json'
$hadOutput = Test-Path -LiteralPath $output -PathType Leaf
$hadManifest = Test-Path -LiteralPath $manifestPath -PathType Leaf
if ($hadOutput) { Copy-Item -LiteralPath $output -Destination $previousOutput }
if ($hadManifest) { Copy-Item -LiteralPath $manifestPath -Destination $previousManifest }
Copy-Item -LiteralPath $candidate -Destination $stagedOutput
Copy-Item -LiteralPath $candidateManifest -Destination $stagedManifest
try {
    Move-Item -LiteralPath $stagedOutput -Destination $output -Force
    Move-Item -LiteralPath $stagedManifest -Destination $manifestPath -Force
} catch {
    if ($hadOutput) { Copy-Item -LiteralPath $previousOutput -Destination $output -Force }
    elseif (Test-Path -LiteralPath $output -PathType Leaf) {
        Remove-Item -LiteralPath $output -Force
    }
    if ($hadManifest) {
        Copy-Item -LiteralPath $previousManifest -Destination $manifestPath -Force
    } elseif (Test-Path -LiteralPath $manifestPath -PathType Leaf) {
        Remove-Item -LiteralPath $manifestPath -Force
    }
    throw
} finally {
    foreach ($path in @($stagedOutput,$stagedManifest)) {
        if (Test-Path -LiteralPath $path -PathType Leaf) {
            Remove-Item -LiteralPath $path -Force
        }
    }
}
[pscustomobject]@{
    status = 'passed'
    outputPath = $output
    manifestPath = $manifestPath
    sha256 = $manifest.sha256
    assetCount = $manifest.assetCount
    packageCount = $manifest.packageCount
    primaryAssetIds = $manifest.primaryAssetIds
    reopenVerified = $manifest.reopenVerified
    evidenceRoot = $evidence
    writerLog = $log
}
