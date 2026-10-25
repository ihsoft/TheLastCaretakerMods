# HAND-WRITTEN STRUCTURAL VALIDATOR: inspects the packaged canonical forklift
# against the current stock package through the public asset JSON interface.
# This does not establish mount precedence or runtime behavior.

param(
    [Parameter(Mandatory = $true)]
    [string]$Container,

    [Parameter(Mandatory = $true)]
    [string]$GameRoot,

    [Parameter(Mandatory = $true)]
    [string]$OutputRoot
)

$ErrorActionPreference = 'Stop'
Set-StrictMode -Version Latest

$modRoot = (Resolve-Path -LiteralPath $PSScriptRoot).Path
$repoRoot = (Resolve-Path -LiteralPath (Join-Path $modRoot '..\..')).Path
$assetJsonTool = Join-Path $repoRoot 'tools\Get-VoyageAssetJson.ps1'
$containerPath = (Resolve-Path -LiteralPath $Container).Path
$gameRootPath = (Resolve-Path -LiteralPath $GameRoot).Path
$output = [IO.Path]::GetFullPath($OutputRoot)
$tmpOwnerRoot = [IO.Path]::GetFullPath((Join-Path $repoRoot 'Tmp\DonkLiftKeyboardControl'))
$requiredPrefix = $tmpOwnerRoot.TrimEnd([IO.Path]::DirectorySeparatorChar) +
    [IO.Path]::DirectorySeparatorChar
if (-not $output.StartsWith($requiredPrefix, [StringComparison]::OrdinalIgnoreCase)) {
    throw "Schema validation output must stay below Tmp/DonkLiftKeyboardControl: $output"
}
if (Test-Path -LiteralPath $output) {
    throw "Schema validation output already exists: $output"
}
New-Item -ItemType Directory -Path $output | Out-Null

function Get-ModAssetJson {
    param(
        [Parameter(Mandatory = $true)]
        [string]$Query,

        [Parameter(Mandatory = $true)]
        [string]$InspectionRoot
    )

    $resultText = & $assetJsonTool `
        -Query $Query `
        -GameRoot $gameRootPath `
        -Source Mod `
        -ModContainer $containerPath `
        -ModInspectionRoot $InspectionRoot `
        -AsJson
    $result = $resultText | ConvertFrom-Json
    if (-not $result.jsonPath -or -not (Test-Path -LiteralPath $result.jsonPath -PathType Leaf)) {
        throw "Asset JSON query did not return a readable path for $Query."
    }
    return $result
}

function Get-GameAssetJson {
    param([Parameter(Mandatory = $true)][string]$Query)

    $resultText = & $assetJsonTool `
        -Query $Query `
        -GameRoot $gameRootPath `
        -Source Game `
        -AsJson
    $result = $resultText | ConvertFrom-Json
    if (-not $result.jsonPath -or -not (Test-Path -LiteralPath $result.jsonPath -PathType Leaf)) {
        throw "Stock asset JSON query did not return a readable path for $Query."
    }
    return $result
}

function Get-OptionalArray {
    param(
        [Parameter(Mandatory = $true)][object]$Object,
        [Parameter(Mandatory = $true)][string]$Name
    )

    if ($Object.PSObject.Properties.Name -contains $Name) {
        return @($Object.$Name)
    }
    return @()
}

function Get-PropertyContract {
    param([Parameter(Mandatory = $true)][object[]]$Properties)

    return @(
        $Properties | ForEach-Object {
            [ordered]@{
                name = [string]$_.Name
                type = [string]$_.Type
                elementSize = [int]$_.ElementSize
                propertyFlags = [string]$_.PropertyFlags
            }
        }
    )
}

$candidateResult = Get-ModAssetJson `
    -Query 'Voyage/Content/Blueprints/Vehicles/BP_Forklift_Possesable.uasset' `
    -InspectionRoot (Join-Path $output 'candidate')
$stockResult = Get-GameAssetJson `
    -Query 'Voyage/Content/Blueprints/Vehicles/BP_Forklift_Possesable.uasset'

$candidateExports = @(Get-Content -LiteralPath $candidateResult.jsonPath -Raw | ConvertFrom-Json)
$stockExports = @(Get-Content -LiteralPath $stockResult.jsonPath -Raw | ConvertFrom-Json)
$candidateClasses = @($candidateExports | Where-Object { [string]$_.Type -ceq 'BlueprintGeneratedClass' })
$stockClasses = @($stockExports | Where-Object { [string]$_.Type -ceq 'BlueprintGeneratedClass' })
if ($candidateClasses.Count -ne 1 -or $stockClasses.Count -ne 1) {
    throw 'Expected exactly one BlueprintGeneratedClass in candidate and stock forklift packages.'
}

$candidateProperties = @(Get-OptionalArray -Object $candidateClasses[0] -Name 'ChildProperties')
$stockProperties = @(Get-OptionalArray -Object $stockClasses[0] -Name 'ChildProperties')
$candidatePropertyContract = @(Get-PropertyContract -Properties $candidateProperties)
$stockPropertyContract = @(Get-PropertyContract -Properties $stockProperties)
$propertyDifference = @(Compare-Object `
    ($stockPropertyContract | ConvertTo-Json -Depth 4 -Compress) `
    ($candidatePropertyContract | ConvertTo-Json -Depth 4 -Compress))
if ($propertyDifference.Count -ne 0) {
    throw 'Candidate forklift ordered own-property contract differs from stock.'
}
if ([string]$candidateClasses[0].SuperStruct.ObjectName -cne
        [string]$stockClasses[0].SuperStruct.ObjectName -or
    [string]$candidateClasses[0].SuperStruct.ObjectPath -cne
        [string]$stockClasses[0].SuperStruct.ObjectPath) {
    throw 'Candidate forklift native parent differs from stock.'
}

$candidateFunctions = @($candidateExports | Where-Object { [string]$_.Type -ceq 'Function' })
$functionNames = @($candidateFunctions.Name | Sort-Object)
$functionDifference = @(Compare-Object @('GetProvidedActionsBP', 'ReceiveBeginPlay') $functionNames)
if ($candidateFunctions.Count -ne 2 -or $functionDifference.Count -ne 0) {
    throw "Candidate forklift must contain exactly GetProvidedActionsBP and ReceiveBeginPlay; found: $($functionNames -join ', ')."
}
$ubergraphFunctions = @($candidateFunctions | Where-Object { [string]$_.Name -like 'ExecuteUbergraph_*' })
if ($ubergraphFunctions.Count -ne 0) {
    throw 'Candidate forklift must not contain an ExecuteUbergraph function.'
}
foreach ($function in $candidateFunctions) {
    if ($function.PSObject.Properties.Name -contains 'EventGraphFunction' -and $null -ne $function.EventGraphFunction) {
        throw "Candidate function $($function.Name) has a non-null EventGraphFunction."
    }
}
if (@($candidateProperties | Where-Object { [string]$_.Name -ceq 'UberGraphFrame' }).Count -ne 0) {
    throw 'Candidate forklift contains UberGraphFrame.'
}
if ($candidateExports.Count -ne $stockExports.Count + 2) {
    throw "Candidate must append exactly two exports to stock; stock=$($stockExports.Count), candidate=$($candidateExports.Count)."
}
$candidateOriginalExports = @($candidateExports | Where-Object { [string]$_.Type -cne 'Function' -and [string]$_.Type -cne 'BlueprintGeneratedClass' })
$stockOriginalExports = @($stockExports | Where-Object { [string]$_.Type -cne 'BlueprintGeneratedClass' })
$originalDifference = @(Compare-Object `
    ($stockOriginalExports | ConvertTo-Json -Depth 100 -Compress) `
    ($candidateOriginalExports | ConvertTo-Json -Depth 100 -Compress))
if ($originalDifference.Count -ne 0) {
    throw 'A non-class stock export changed in the canonical candidate.'
}

$report = [ordered]@{
    scope = 'Packaged cooked Blueprint structure only; no runtime or mount-precedence validation'
    container = $containerPath
    candidate = [ordered]@{
        jsonPath = [string]$candidateResult.jsonPath
        nativeParentObjectPath = [string]$candidateClasses[0].SuperStruct.ObjectPath
        ownPropertyCount = $candidateProperties.Count
        originalExportCount = $stockExports.Count
        totalExportCount = $candidateExports.Count
        functionNames = $functionNames
        executeUbergraphCount = $ubergraphFunctions.Count
        nonClassStockExportsUnchanged = $true
    }
    stockControl = [ordered]@{
        jsonPath = [string]$stockResult.jsonPath
        ownPropertyCount = $stockProperties.Count
    }
}
$reportPath = Join-Path $output 'forklift-schema-validation.json'
[IO.File]::WriteAllText(
    $reportPath,
    (($report | ConvertTo-Json -Depth 6) + [Environment]::NewLine))

[pscustomobject]@{
    status = 'pass'
    reportPath = $reportPath
    candidateJsonPath = [string]$candidateResult.jsonPath
    stockJsonPath = [string]$stockResult.jsonPath
}
