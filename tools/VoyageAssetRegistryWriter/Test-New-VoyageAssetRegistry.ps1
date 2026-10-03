[CmdletBinding()]
param([string]$EngineRoot = 'K:\Epic Games\UE_5.8\Engine')

$ErrorActionPreference = 'Stop'
Set-StrictMode -Version Latest
$repo = (Resolve-Path -LiteralPath (Join-Path $PSScriptRoot '..\..')).Path
$root = Join-Path $repo ('artifacts\tests\voyage-asset-registry-' +
    [DateTime]::UtcNow.ToString('yyyyMMdd-HHmmss-fff'))
$null = New-Item -ItemType Directory -Path $root
$utf8 = New-Object Text.UTF8Encoding($false)
$converter = Join-Path $PSScriptRoot 'ConvertTo-VoyageAssetRegistryMetadata.ps1'
$publicTool = Join-Path (Split-Path -Parent $PSScriptRoot) `
    'New-VoyageAssetRegistry.ps1'
function Copy-Json($Value) {
    $Value | ConvertTo-Json -Depth 100 | ConvertFrom-Json
}

function Write-Json([string]$Path, $Value) {
    [IO.File]::WriteAllText($Path, ($Value | ConvertTo-Json -Depth 100), $utf8)
}

function New-SyntheticAssetJson([string]$Package, [string]$Object,
    [string]$Class, [string]$PrimaryType, [bool]$AuxiliaryExport) {
    $data = @(
        [pscustomobject]@{
            Name='Type'; StructType='PrimaryAssetType'
            Value=@([pscustomobject]@{Name='Name';Value=$PrimaryType})
        }
    )
    if ($PrimaryType -ceq 'Item') {
        $data += @(
            [pscustomobject]@{Name='bAllowFabricationOnTop';Value=$false},
            [pscustomobject]@{Name='bCanGainLootExp';Value=$true},
            [pscustomobject]@{Name='bIsDestructible';Value=$false},
            [pscustomobject]@{Name='bIsDismantlable';Value=$false},
            [pscustomobject]@{Name='bIsRecyclable';Value=$true},
            [pscustomobject]@{Name='bIsRepairable';Value=$false},
            [pscustomobject]@{Name='Category';Value='Ammo'},
            [pscustomobject]@{Name='CraftFilter';Value=3},
            [pscustomobject]@{Name='Quality';Value='Common'},
            [pscustomobject]@{Name='Tag';Value='None'},
            [pscustomobject]@{Name='Weight';Value=1.5}
        )
    }
    $exports = @([pscustomobject]@{
        '$type'='UAssetAPI.ExportTypes.NormalExport, UAssetAPI'
        ObjectName=$Object; ClassIndex=-1; OuterIndex=0; Data=$data
    })
    if ($AuxiliaryExport) {
        $exports += [pscustomobject]@{
            '$type'='UAssetAPI.ExportTypes.NormalExport, UAssetAPI'
            ObjectName='AuxiliaryExport'; ClassIndex=0; OuterIndex=0; Data=@()
        }
    }
    [pscustomobject]@{
        FolderName=$Package
        PackageFlags='PKG_Cooked, PKG_UnversionedProperties, PKG_FilterEditorOnly'
        Imports=@(
            [pscustomobject]@{
                ObjectName=$Class; ClassName='Class'
                ClassPackage='/Script/CoreUObject'; OuterIndex=-2
            },
            [pscustomobject]@{
                ObjectName='/Script/Voyage'; ClassName='Package'
                ClassPackage='/Script/CoreUObject'; OuterIndex=0
            }
        )
        Exports=$exports
    }
}

$fixtureRoot = Join-Path $root 'fixtures'
$null = New-Item -ItemType Directory -Path $fixtureRoot
$ammo = New-SyntheticAssetJson '/Game/Fixture/DA_TestAmmo' 'DA_TestAmmo' `
    'VoyageItemAmmo' 'Item' $false
$gun = New-SyntheticAssetJson '/Game/Fixture/DA_TestItem' 'DA_TestItem' `
    'VoyageItem' 'Item' $true
$skill = New-SyntheticAssetJson '/Game/Fixture/DA_TestSkill' 'DA_TestSkill' `
    'VoyageSkill' 'Skill' $false
$sourcePaths = @(
    (Join-Path $fixtureRoot 'ammo.json'),
    (Join-Path $fixtureRoot 'item.json'),
    (Join-Path $fixtureRoot 'skill.json')
)
Write-Json $sourcePaths[0] $ammo
Write-Json $sourcePaths[1] $gun
Write-Json $sourcePaths[2] $skill
$sourceHashes = @($sourcePaths | ForEach-Object {
    [pscustomobject]@{path=$_;sha256=(Get-FileHash -LiteralPath $_ -Algorithm SHA256).Hash}
})

function Invoke-ConverterCase([string]$Name, [string[]]$Inputs, [bool]$ShouldPass) {
    $directory = Join-Path $root $Name
    $null = New-Item -ItemType Directory -Path $directory
    $output = Join-Path $directory 'metadata.json'
    $failure = $null
    try { $result = & $converter -AssetJsonPath $Inputs -OutputPath $output }
    catch { $failure = $_ }
    if ($ShouldPass -and $failure) { throw "$Name unexpectedly failed: $failure" }
    if (-not $ShouldPass -and -not $failure) { throw "$Name unexpectedly passed." }
    if ($ShouldPass) {
        [pscustomobject]@{result=$result;metadata=(Get-Content $output -Raw | ConvertFrom-Json);path=$output}
    }
}

$baseline = Invoke-ConverterCase 'baseline' $sourcePaths $true
if ($baseline.result.assetCount -ne 3 -or @($baseline.metadata.assets).Count -ne 3) {
    throw 'Baseline converter count differs.'
}
$explicitAmmo = Copy-Json $ammo
($explicitAmmo.Exports[0].Data | Where-Object Name -ceq 'CraftFilter').Value = 0
($explicitAmmo.Exports[0].Data | Where-Object Name -ceq 'bCanGainLootExp').Value = $false
($explicitAmmo.Exports[0].Data | Where-Object Name -ceq 'Weight').Value = 7.25
($explicitAmmo.Exports[0].Data | Where-Object Name -ceq 'Quality').Value = 'Rare'
$explicitPath = Join-Path $root 'explicit.json'
Write-Json $explicitPath $explicitAmmo
$explicit = Invoke-ConverterCase 'explicit-values' @($explicitPath) $true
$explicitTags = $explicit.metadata.assets[0].tags
if ($explicitTags.bCanGainLootExp -cne 'False' -or
    $explicitTags.CraftFilter -cne '0' -or $explicitTags.Weight -cne '7.25' -or
    $explicitTags.Quality -cne 'EVoyageItemQuality::Rare') {
    throw 'Explicit false/zero/Weight/Quality values did not win over defaults.'
}

$renamedAmmo = Copy-Json $ammo
$renamedAmmo.FolderName = '/Game/Fixture/DA_RenamedAmmo'
$renamedAmmo.Exports[0].ObjectName = 'DA_RenamedAmmo'
$renamedPath = Join-Path $root 'renamed.json'
Write-Json $renamedPath $renamedAmmo
$renamed = Invoke-ConverterCase 'renamed-single' @($renamedPath) $true
if ($renamed.result.assetCount -ne 1 -or
    $renamed.metadata.assets[0].objectPath -cne
        '/Game/Fixture/DA_RenamedAmmo.DA_RenamedAmmo') {
    throw 'Renamed single-asset fixture was not derived from JSON identity.'
}
$two = Invoke-ConverterCase 'two-assets' @($renamedPath,$sourcePaths[2]) $true
if ($two.result.assetCount -ne 2) { throw 'Variable asset count is not supported.' }
$auxiliary = Invoke-ConverterCase 'auxiliary-exports' @($sourcePaths[1]) $true
if ($auxiliary.result.assetCount -ne 1) { throw 'Auxiliary exports blocked the gun item.' }

$wrongClass = Copy-Json $ammo
$classIndex = -([int]$wrongClass.Exports[0].ClassIndex) - 1
$wrongClass.Imports[$classIndex].ObjectName = 'UnsupportedVoyageItem'
$wrongClassPath = Join-Path $root 'wrong-class.json'
Write-Json $wrongClassPath $wrongClass
$null = Invoke-ConverterCase 'wrong-class' @($wrongClassPath) $false

$invalidType = Copy-Json $skill
$typeProperty = $invalidType.Exports[0].Data | Where-Object Name -ceq 'Type'
($typeProperty.Value | Where-Object Name -ceq 'Name').Value = 'Item'
$invalidTypePath = Join-Path $root 'invalid-type.json'
Write-Json $invalidTypePath $invalidType
$null = Invoke-ConverterCase 'invalid-type' @($invalidTypePath) $false
$null = Invoke-ConverterCase 'duplicate-object' @($sourcePaths[0],$sourcePaths[0]) $false

$duplicateId = Copy-Json $ammo
$duplicateId.FolderName = '/Game/Fixture/DuplicatePrimaryId'
$duplicateIdPath = Join-Path $root 'duplicate-id.json'
Write-Json $duplicateIdPath $duplicateId
$null = Invoke-ConverterCase 'duplicate-primary-id' `
    @($sourcePaths[0],$duplicateIdPath) $false

$nested = Copy-Json $ammo
$nested.Exports[0].OuterIndex = 1
$nestedPath = Join-Path $root 'nested.json'
Write-Json $nestedPath $nested
$null = Invoke-ConverterCase 'nested-primary' @($nestedPath) $false

$publicOutput = Join-Path $root 'public\AssetRegistry.bin'
$public = & $publicTool -AssetJsonPath $sourcePaths -OutputPath $publicOutput `
    -EngineRoot $EngineRoot -EvidenceRoot (Join-Path $root 'public-evidence')
if ($public.status -cne 'passed' -or $public.assetCount -ne 3 -or
    $public.packageCount -ne 3 -or -not [bool]$public.reopenVerified) {
    throw 'Public tool did not write and reopen the baseline registry.'
}
$outputHash = (Get-FileHash -LiteralPath $publicOutput -Algorithm SHA256).Hash
$manifestHash = (Get-FileHash -LiteralPath ($publicOutput + '.manifest.json') `
    -Algorithm SHA256).Hash
try {
    $null = & $publicTool -AssetJsonPath @($wrongClassPath) `
        -OutputPath $publicOutput -EngineRoot $EngineRoot `
        -EvidenceRoot (Join-Path $root 'public-invalid-evidence')
    throw 'Bad public input unexpectedly passed.'
} catch {
    if ($_.Exception.Message -ceq 'Bad public input unexpectedly passed.') { throw }
}
if ((Get-FileHash -LiteralPath $publicOutput -Algorithm SHA256).Hash -cne $outputHash -or
    (Get-FileHash -LiteralPath ($publicOutput + '.manifest.json') `
        -Algorithm SHA256).Hash -cne $manifestHash) {
    throw 'Bad input changed the previously published output/manifest pair.'
}

$caseVariant = $sourcePaths[0].ToUpperInvariant()
try {
    $null = & $publicTool -AssetJsonPath @($sourcePaths[0]) `
        -OutputPath $caseVariant -EngineRoot $EngineRoot `
        -EvidenceRoot (Join-Path $root 'collision-evidence')
    throw 'Case-insensitive input/output collision unexpectedly passed.'
} catch {
    if ($_.Exception.Message -ceq
        'Case-insensitive input/output collision unexpectedly passed.') { throw }
}

$writer = & (Join-Path $PSScriptRoot 'Get-VoyageAssetRegistryWriter.ps1') `
    -EngineRoot $EngineRoot
$directRoot = Join-Path $root 'direct-writer'
$hostRoot = Join-Path $directRoot 'host'
$null = New-Item -ItemType Directory -Path $directRoot
Copy-Item -LiteralPath (Split-Path -Parent $writer.ProjectPath) `
    -Destination $hostRoot -Recurse
$directMetadata = Copy-Json $baseline.metadata
$first = Copy-Json $directMetadata.assets[0]
$second = Copy-Json $first
$first.assetClassPath = '/Script/Synthetic.OtherClass'
$second.assetClassPath = '/Script/Synthetic.OtherClass'
$second.assetName = 'DA_TestAlternateId'
$second.objectPath = $second.packageName + '.' + $second.assetName
$second.tags.PrimaryAssetName = 'AlternatePrimaryName'
$first.chunkIds = @(3,7)
$second.chunkIds = @(3,7)
$directMetadata.assets = @($first,$second)
$directMetadataPath = Join-Path $directRoot 'metadata.json'
Write-Json $directMetadataPath $directMetadata
$directOutput = Join-Path $directRoot 'AssetRegistry.bin'
$directSummary = Join-Path $directRoot 'summary.json'
$editor = $writer.Engine.editorPath
$directArguments = @(
    (Join-Path $hostRoot 'VoyageAssetRegistryWriter.uproject'),
    '-run=WriteVoyageAssetRegistry',('-RegistryMetadata=' + $directMetadataPath),
    ('-OutputRegistry=' + $directOutput),('-OutputSummary=' + $directSummary),
    '-unattended','-nop4','-nosplash','-nullrhi',
    ('-abslog=' + (Join-Path $directRoot 'writer.log'))
)
& $editor @directArguments *> (Join-Path $directRoot 'console.log')
if ($LASTEXITCODE -ne 0) { throw 'Generic direct writer fixture failed.' }
$direct = Get-Content -LiteralPath $directSummary -Raw | ConvertFrom-Json
if ($direct.assetCount -ne 2 -or $direct.packageCount -ne 1 -or
    -not [bool]$direct.reopenVerified) {
    throw 'Generic writer still assumes packageCount equals assetCount.'
}

$malformedRoot = Join-Path $root 'malformed-writer'
$null = New-Item -ItemType Directory -Path $malformedRoot
$malformed = Join-Path $malformedRoot 'metadata.json'
[IO.File]::WriteAllText($malformed, '{}', $utf8)
$malformedArguments = @(
    (Join-Path $hostRoot 'VoyageAssetRegistryWriter.uproject'),
    '-run=WriteVoyageAssetRegistry',('-RegistryMetadata=' + $malformed),
    ('-OutputRegistry=' + (Join-Path $malformedRoot 'AssetRegistry.bin')),
    ('-OutputSummary=' + (Join-Path $malformedRoot 'summary.json')),
    '-unattended','-nop4','-nosplash','-nullrhi',
    ('-abslog=' + (Join-Path $malformedRoot 'writer.log'))
)
& $editor @malformedArguments *> (Join-Path $malformedRoot 'console.log')
if ($LASTEXITCODE -eq 0 -or
    (Test-Path -LiteralPath (Join-Path $malformedRoot 'AssetRegistry.bin'))) {
    throw 'Malformed native metadata did not fail closed.'
}

foreach ($source in $sourceHashes) {
    if ((Get-FileHash -LiteralPath $source.path -Algorithm SHA256).Hash -cne
        $source.sha256) {
        throw "Test mutated source input: $($source.path)"
    }
}
[pscustomobject]@{
    status = 'passed'
    converterCases = 10
    nativeCases = 4
    outputRoot = $root
    baselineRegistrySha256 = $outputHash
    writerInputFingerprint = $writer.InputFingerprint
}
