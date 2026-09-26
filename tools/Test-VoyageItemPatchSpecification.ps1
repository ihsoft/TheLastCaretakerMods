# Validates one owner-supplied extended item patch without embedding item identities.
[CmdletBinding()]
param(
    [Parameter(Mandatory = $true)]
    [string]$InputAsset,

    [Parameter(Mandatory = $true)]
    [string]$Specification,

    [string]$Mappings,

    [ValidateSet('UE5_7', 'UE5_8')]
    [string]$EngineVersion = 'UE5_8',

    [string]$GameRoot = 'P:\SteamLibrary\steamapps\common\Voyage',

    [string]$EvidenceRoot
)

$ErrorActionPreference = 'Stop'
Set-StrictMode -Version Latest

$repositoryRoot = (Resolve-Path -LiteralPath (Join-Path $PSScriptRoot '..')).Path
$inputPath = (Resolve-Path -LiteralPath $InputAsset).Path
$specificationPath = (Resolve-Path -LiteralPath $Specification).Path
if ([string]::IsNullOrWhiteSpace($EvidenceRoot)) {
    $EvidenceRoot = Join-Path $repositoryRoot 'artifacts\tests'
}
$testRoot = Join-Path ([IO.Path]::GetFullPath($EvidenceRoot)) (
    'item-patch-specification-' + [Guid]::NewGuid().ToString('N'))
[IO.Directory]::CreateDirectory($testRoot) | Out-Null
$checks = [Collections.Generic.List[string]]::new()
$binary = & (Join-Path $PSScriptRoot 'Get-VoyageAssetPatcherBinary.ps1')
if ([string]::IsNullOrWhiteSpace($Mappings)) {
    if ($EngineVersion -cne 'UE5_8') {
        throw 'Legacy UE5_7 tests require one explicit provenance-checked -Mappings file.'
    }
    $mapping = & (Join-Path $PSScriptRoot 'Get-VoyageMappings.ps1') -GameRoot $GameRoot
    $mappingsPath = [string]$mapping.mappingsPath
}
else {
    $mappingsPath = (Resolve-Path -LiteralPath $Mappings).Path
}

function Assert-ItemPatchTest {
    param(
        [Parameter(Mandatory = $true)]
        [bool]$Condition,

        [Parameter(Mandatory = $true)]
        [string]$Message
    )

    if (-not $Condition) { throw $Message }
}

function Copy-SpecificationObject {
    param([Parameter(Mandatory = $true)]$Value)

    return (($Value | ConvertTo-Json -Depth 30) | ConvertFrom-Json)
}

function Write-SpecificationObject {
    param(
        [Parameter(Mandatory = $true)]$Value,
        [Parameter(Mandatory = $true)][string]$Path
    )

    [IO.File]::WriteAllText(
        $Path,
        (($Value | ConvertTo-Json -Depth 30) + [Environment]::NewLine),
        [Text.UTF8Encoding]::new($false))
}

function Assert-RejectedSpecification {
    param(
        [Parameter(Mandatory = $true)][string]$Name,
        [Parameter(Mandatory = $true)]$Value,
        [Parameter(Mandatory = $true)][string]$Pattern
    )

    $caseRoot = Join-Path $testRoot $Name
    [IO.Directory]::CreateDirectory($caseRoot) | Out-Null
    $caseSpecification = Join-Path $caseRoot 'specification.json'
    $caseOutput = Join-Path $caseRoot 'output.uasset'
    $caseLog = Join-Path $caseRoot 'patcher.log'
    Write-SpecificationObject -Value $Value -Path $caseSpecification
    $savedPreference = $ErrorActionPreference
    try {
        $ErrorActionPreference = 'Continue'
        & $binary.Path patch-item-data-asset $inputPath $mappingsPath $caseOutput `
            $EngineVersion $caseSpecification *> $caseLog
        $exitCode = $LASTEXITCODE
    }
    finally {
        $ErrorActionPreference = $savedPreference
    }
    $log = if (Test-Path -LiteralPath $caseLog) {
        [IO.File]::ReadAllText($caseLog)
    }
    else { '' }
    Assert-ItemPatchTest ($exitCode -ne 0) "Rejected case '$Name' unexpectedly succeeded."
    Assert-ItemPatchTest ($log -match $Pattern) `
        "Rejected case '$Name' did not report the expected diagnostic. Log: $caseLog"
    Assert-ItemPatchTest (-not (Test-Path -LiteralPath $caseOutput)) `
        "Rejected case '$Name' wrote an output asset."
    $checks.Add($Name)
}

$sourceSpecification = Get-Content -LiteralPath $specificationPath -Raw | ConvertFrom-Json
$softMutations = @($sourceSpecification.patch.softObjectReferences)
$mapClears = @($sourceSpecification.patch.clearNameObjectMaps)
$replacementRecipe = @($sourceSpecification.patch.components)
Assert-ItemPatchTest ($softMutations.Count -gt 0) `
    'The test specification must exercise softObjectReferences.'
Assert-ItemPatchTest ($mapClears.Count -gt 0) `
    'The test specification must exercise clearNameObjectMaps.'
Assert-ItemPatchTest ($replacementRecipe.Count -gt 0) `
    'The test specification must exercise complete recipe replacement.'

$positiveOutput = Join-Path $testRoot 'positive\output.uasset'
$positive = & (Join-Path $PSScriptRoot 'Invoke-VoyageAssetPatcher.ps1') `
    -Operation patch-item-data-asset -InputAsset $inputPath `
    -OutputAsset $positiveOutput -Mappings $mappingsPath `
    -EngineVersion $EngineVersion -Specification $specificationPath `
    -EvidenceRoot (Join-Path $testRoot 'tool-runs')
Assert-ItemPatchTest ($positive.status -ceq 'completed' -and
    (Test-Path -LiteralPath $positiveOutput -PathType Leaf)) `
    "Positive item patch failed. Log: $($positive.logPath)"
$checks.Add('extended-item-patch-positive')

$unknownField = Copy-SpecificationObject $sourceSpecification
$unknownField | Add-Member -NotePropertyName unsupportedTestField -NotePropertyValue $true
Assert-RejectedSpecification -Name 'unknown-field-rejected' -Value $unknownField `
    -Pattern 'unmapped|could not be mapped'

$wrongSoftType = Copy-SpecificationObject $sourceSpecification
$wrongSoftType.patch.softObjectReferences[0].property = 'Name'
Assert-RejectedSpecification -Name 'soft-object-type-rejected' -Value $wrongSoftType `
    -Pattern 'not a SoftObjectPropertyData'

$wrongMapContent = Copy-SpecificationObject $sourceSpecification
$wrongMapContent.patch.clearNameObjectMaps[0].expectedEntries[0].objectName += '_Mismatch'
Assert-RejectedSpecification -Name 'map-content-rejected' -Value $wrongMapContent `
    -Pattern 'does not match'

$duplicateTarget = Copy-SpecificationObject $sourceSpecification
$duplicateTarget.patch.softObjectReferences = @(
    $duplicateTarget.patch.softObjectReferences[0],
    (Copy-SpecificationObject $duplicateTarget.patch.softObjectReferences[0]))
Assert-RejectedSpecification -Name 'duplicate-target-rejected' -Value $duplicateTarget `
    -Pattern 'targeted more than once'

$conflictingRecipe = Copy-SpecificationObject $sourceSpecification
$firstMaterial = Copy-SpecificationObject $conflictingRecipe.expected.components[0].material
$conflictingRecipe.patch.componentReplacements = @(
    [pscustomobject][ordered]@{
        from = Copy-SpecificationObject $firstMaterial
        to = Copy-SpecificationObject $firstMaterial
    })
Assert-RejectedSpecification -Name 'recipe-conflict-rejected' -Value $conflictingRecipe `
    -Pattern 'mutually exclusive'

[pscustomobject][ordered]@{
    status = 'passed'
    checks = @($checks)
    checkCount = $checks.Count
    inputSha256 = (Get-FileHash -LiteralPath $inputPath -Algorithm SHA256).Hash
    specificationSha256 = (Get-FileHash -LiteralPath $specificationPath -Algorithm SHA256).Hash
    patcherBinarySha256 = $binary.Sha256
    mappingsSha256 = (Get-FileHash -LiteralPath $mappingsPath -Algorithm SHA256).Hash
    evidencePath = $testRoot
}
