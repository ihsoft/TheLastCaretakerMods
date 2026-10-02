[CmdletBinding()]
param(
    [string]$OutputRoot = '',
    [string]$ContractPath = '',
    [string]$AmmoJsonPath = '',
    [string]$GunJsonPath = '',
    [string]$SkillJsonPath = ''
)

$ErrorActionPreference = 'Stop'
Set-StrictMode -Version Latest
$mod = Split-Path -Parent $PSScriptRoot
$repo = [IO.Path]::GetFullPath((Join-Path $mod '../..'))
if (-not $OutputRoot) {
    $OutputRoot = Join-Path $repo ('artifacts/tests/railgun-registry-metadata-' +
        [DateTime]::UtcNow.ToString('yyyyMMdd-HHmmss-fff'))
}
if (-not $ContractPath) {
    $ContractPath = Join-Path $mod 'Assets/data-assets-contract.json'
}
if (-not $AmmoJsonPath) {
    $AmmoJsonPath = Join-Path $mod 'Assets/Fabricator/railgun-ammo-item.json'
}
if (-not $GunJsonPath) {
    $GunJsonPath = Join-Path $mod 'Assets/Fabricator/railgun-item.json'
}
if (-not $SkillJsonPath) {
    $SkillJsonPath = Join-Path $mod 'Assets/Skill/railgun-skill.json'
}
$root = [IO.Path]::GetFullPath($OutputRoot)
$null = New-Item -ItemType Directory -Path $root
$generator = Join-Path $PSScriptRoot 'New-RailgunRegistryMetadata.ps1'
$utf8 = [Text.UTF8Encoding]::new($false)

function Write-Json([string]$Path, $Value) {
    [IO.File]::WriteAllText($Path, ($Value | ConvertTo-Json -Depth 100), $utf8)
}

function Copy-Json($Value) {
    $Value | ConvertTo-Json -Depth 100 | ConvertFrom-Json
}

function Invoke-Case([string]$Name, $Contract, $Ammo, $Gun, $Skill,
    [bool]$ShouldPass) {
    $directory = Join-Path $root $Name
    $null = New-Item -ItemType Directory -Path $directory
    $contractFile = Join-Path $directory 'contract.json'
    $ammoFile = Join-Path $directory 'ammo.json'
    $gunFile = Join-Path $directory 'gun.json'
    $skillFile = Join-Path $directory 'skill.json'
    $outputFile = Join-Path $directory 'metadata.json'
    Write-Json $contractFile $Contract
    Write-Json $ammoFile $Ammo
    Write-Json $gunFile $Gun
    Write-Json $skillFile $Skill
    $failure = $null
    try {
        $null = & $generator -ContractPath $contractFile `
            -AssetJsonPath @($ammoFile,$gunFile,$skillFile) -OutputPath $outputFile
    } catch {
        $failure = $_
    }
    if ($ShouldPass -and $null -ne $failure) {
        throw "Case $Name unexpectedly failed: $failure"
    }
    if (-not $ShouldPass -and $null -eq $failure) {
        throw "Case $Name unexpectedly passed."
    }
    if ($ShouldPass) {
        return Get-Content -LiteralPath $outputFile -Raw | ConvertFrom-Json
    }
}

$contract = Get-Content -LiteralPath $ContractPath -Raw | ConvertFrom-Json
$ammo = Get-Content -LiteralPath $AmmoJsonPath -Raw | ConvertFrom-Json
$gun = Get-Content -LiteralPath $GunJsonPath -Raw | ConvertFrom-Json
$skill = Get-Content -LiteralPath $SkillJsonPath -Raw | ConvertFrom-Json
$baseline = Invoke-Case 'baseline' (Copy-Json $contract) (Copy-Json $ammo) `
    (Copy-Json $gun) (Copy-Json $skill) $true
if (@($baseline.assets).Count -ne 3 -or
    $baseline.assets[0].tags.bCanGainLootExp -cne 'True' -or
    $baseline.assets[0].tags.bIsDestructible -cne 'False') {
    throw 'Baseline fallback tags are invalid.'
}

$mutatedAmmo = Copy-Json $ammo
($mutatedAmmo.Exports[0].Data | Where-Object { $_.Name -ceq 'Weight' }).Value = 7.25
($mutatedAmmo.Exports[0].Data | Where-Object { $_.Name -ceq 'Quality' }).Value = 'Rare'
$mutated = Invoke-Case 'weight-quality' (Copy-Json $contract) $mutatedAmmo `
    (Copy-Json $gun) (Copy-Json $skill) $true
if ($mutated.assets[0].tags.Weight -cne '7.25' -or
    $mutated.assets[0].tags.Quality -cne 'EVoyageItemQuality::Rare') {
    throw 'Serialized Weight/Quality mutations did not reach registry tags.'
}

$explicitAmmo = Copy-Json $ammo
($explicitAmmo.Exports[0].Data | Where-Object { $_.Name -ceq 'CraftFilter' }).Value = 0
$explicitAmmo.Exports[0].Data += [pscustomobject]@{
    '$type' = 'UAssetAPI.PropertyTypes.Objects.BoolPropertyData, UAssetAPI'
    Name = 'bCanGainLootExp'
    Value = $false
}
$explicit = Invoke-Case 'explicit-false-zero' (Copy-Json $contract) $explicitAmmo `
    (Copy-Json $gun) (Copy-Json $skill) $true
if ($explicit.assets[0].tags.bCanGainLootExp -cne 'False' -or
    $explicit.assets[0].tags.CanGainLootExp -cne 'False' -or
    $explicit.assets[0].tags.CraftFilter -cne '0') {
    throw 'Explicit false/zero was replaced by a fallback.'
}

$wrongClassAmmo = Copy-Json $ammo
$wrongClassAmmo.Imports[0].ObjectName = 'VoyageItem'
$null = Invoke-Case 'wrong-class' (Copy-Json $contract) $wrongClassAmmo `
    (Copy-Json $gun) (Copy-Json $skill) $false

$ambiguousAmmo = Copy-Json $ammo
$ambiguousAmmo.Exports += Copy-Json $ambiguousAmmo.Exports[0]
$null = Invoke-Case 'ambiguous-export' (Copy-Json $contract) $ambiguousAmmo `
    (Copy-Json $gun) (Copy-Json $skill) $false

$invalidSkill = Copy-Json $skill
$type = $invalidSkill.Exports[0].Data | Where-Object { $_.Name -ceq 'Type' }
($type.Value | Where-Object { $_.Name -ceq 'Name' }).Value = 'Item'
$null = Invoke-Case 'invalid-type' (Copy-Json $contract) (Copy-Json $ammo) `
    (Copy-Json $gun) $invalidSkill $false

[pscustomobject]@{
    status = 'passed'
    passedCases = 6
    outputRoot = $root
}
