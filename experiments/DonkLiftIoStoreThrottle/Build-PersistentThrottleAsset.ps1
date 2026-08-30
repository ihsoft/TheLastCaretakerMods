# HAND-WRITTEN RESEARCH TOOL with bytecode/layout contracts derived from
# Steam build 23962331 (Unreal Engine 5.7.4),
# VoyageSteam-Win64-Shipping.exe SHA-256
# 6A9AE86E5CE5D7D1B6555F579091AAB1E0E67FF7A96276FA2570052F99102E8D.
# Re-extract the source asset and revalidate every offset/hash after a game update.

param(
    [Parameter(Mandatory = $true)]
    [string] $SourceJson,

    [Parameter(Mandatory = $true)]
    [string] $SourceUasset,

    [Parameter(Mandatory = $true)]
    [string] $UAssetGui,

    [Parameter(Mandatory = $true)]
    [string] $Mappings,

    [Parameter(Mandatory = $true)]
    [string] $OutputDirectory,

    [double] $RampSpeed = (1.0 / 3.0),

    # Unreal control-flow operands use the VM iCode index, not the file offset
    # shown by other Blueprint dumpers. Verified with UAssetAPI SerializeScript.
    [uint32] $InsertionIndex = 2385
)

$ErrorActionPreference = 'Stop'
$jsonDepth = 100
$utf8NoBom = [Text.UTF8Encoding]::new($false)
$uberGraphName = 'ExecuteUbergraph_BP_Forklift_Possesable'
$fInterpToConstantImport = -110
$multiplyDoubleImport = -117
$multiplyResultName = 'CallFunc_Multiply_DoubleDouble_ReturnValue_3'
$throttleDoubleName = 'CallFunc_Multiply_DoubleDouble_B_ImplicitCast'

function Copy-JsonNode([object] $Node) {
    return $Node | ConvertTo-Json -Depth $jsonDepth -Compress | ConvertFrom-Json -Depth $jsonDepth
}

function Write-JsonFile([object] $Value, [string] $Path) {
    $text = $Value | ConvertTo-Json -Depth $jsonDepth
    [IO.File]::WriteAllText($Path, $text, $utf8NoBom)
}

function Invoke-UAssetGui([string[]] $Arguments, [string] $ExpectedOutput) {
    $process = Start-Process -FilePath $UAssetGui -ArgumentList $Arguments -PassThru -Wait -NoNewWindow
    if ($process.ExitCode -ne 0 -or -not (Test-Path -LiteralPath $ExpectedOutput)) {
        throw "UAssetGUI failed to create $ExpectedOutput (exit $($process.ExitCode))"
    }
}

function Get-UberGraph([object] $Asset) {
    $matches = @($Asset.Exports | Where-Object ObjectName -eq $uberGraphName)
    if ($matches.Count -ne 1) {
        throw "Expected one $uberGraphName export, found $($matches.Count)"
    }
    return $matches[0]
}

function Update-JumpTargets([object] $Node, [uint32] $InsertionIndex, [uint32] $Delta) {
    if ($null -eq $Node) { return }

    if ($Node -is [System.Management.Automation.PSCustomObject]) {
        $typeName = [string] $Node.'$type'
        if ($typeName -match '\.EX_(Jump|JumpIfNot|Skip),') {
            if ([uint32] $Node.CodeOffset -gt $InsertionIndex) {
                $Node.CodeOffset = [uint32] $Node.CodeOffset + $Delta
            }
        }
        elseif ($typeName -match '\.EX_PushExecutionFlow,') {
            if ([uint32] $Node.PushingAddress -gt $InsertionIndex) {
                $Node.PushingAddress = [uint32] $Node.PushingAddress + $Delta
            }
        }

        foreach ($property in $Node.PSObject.Properties) {
            Update-JumpTargets $property.Value $InsertionIndex $Delta
        }
        return
    }

    if ($Node -is [System.Collections.IEnumerable] -and $Node -isnot [string]) {
        foreach ($item in $Node) {
            Update-JumpTargets $item $InsertionIndex $Delta
        }
    }
}

function Update-UberGraphEntrypoints(
    [object] $Asset,
    [int] $UberGraphPackageIndex,
    [uint32] $InsertionIndex,
    [uint32] $Delta
) {
    $updated = 0
    foreach ($export in $Asset.Exports) {
        if ($null -eq $export.ScriptBytecode) { continue }
        foreach ($statement in $export.ScriptBytecode) {
            if ($statement.'$type' -notmatch '\.EX_LocalFinalFunction,' -or
                [int] $statement.StackNode -ne $UberGraphPackageIndex) {
                continue
            }

            if ($statement.Parameters.Count -lt 1 -or
                $statement.Parameters[0].'$type' -notmatch '\.EX_IntConst,') {
                throw "Unexpected $uberGraphName caller in $($export.ObjectName)"
            }

            if ([uint32] $statement.Parameters[0].Value -gt $InsertionIndex) {
                $statement.Parameters[0].Value = [int64] $statement.Parameters[0].Value + $Delta
            }
            $updated++
        }
    }
    if ($updated -eq 0) {
        throw "No $uberGraphName entrypoints were updated"
    }
    return $updated
}

$sourcePath = (Resolve-Path -LiteralPath $SourceJson).Path
$sourceUassetPath = (Resolve-Path -LiteralPath $SourceUasset).Path
$uassetGuiPath = (Resolve-Path -LiteralPath $UAssetGui).Path
$mappingsPath = (Resolve-Path -LiteralPath $Mappings).Path
$outputPath = [IO.Path]::GetFullPath($OutputDirectory)
if (Test-Path -LiteralPath $outputPath) {
    throw "Output directory already exists: $outputPath"
}
New-Item -ItemType Directory -Path $outputPath -Force | Out-Null

$asset = Get-Content -LiteralPath $sourcePath -Raw | ConvertFrom-Json -Depth $jsonDepth
$uberGraph = Get-UberGraph $asset
$uberGraphPackageIndex = [array]::IndexOf([object[]] $asset.Exports, [object] $uberGraph) + 1
if ($uberGraphPackageIndex -le 0) {
    throw 'Failed to determine the UberGraph package index'
}

$targetPositions = @()
for ($index = 0; $index -lt $uberGraph.ScriptBytecode.Count; $index++) {
    $statement = $uberGraph.ScriptBytecode[$index]
    $call = $statement.Expression
    if ($call.'$type' -match '\.EX_CallMath,' -and
        [int] $call.StackNode -eq $fInterpToConstantImport -and
        $call.Parameters.Count -eq 4 -and
        $call.Parameters[0].Variable.New.Path[0] -eq 'Acceleration' -and
        $call.Parameters[1].Variable.New.Path[0] -eq $multiplyResultName) {
        $targetPositions += $index
    }
}
if ($targetPositions.Count -ne 1) {
    throw "Expected one throttle FInterpTo_Constant call, found $($targetPositions.Count)"
}

$targetPosition = $targetPositions[0]
$targetStatement = $uberGraph.ScriptBytecode[$targetPosition]
$targetCall = $targetStatement.Expression
$throttleExpression = [pscustomobject] [ordered] @{
    '$type' = 'UAssetAPI.Kismet.Bytecode.Expressions.EX_LocalVariable, UAssetAPI'
    Variable = [pscustomobject] [ordered] @{
        '$type' = 'UAssetAPI.Kismet.Bytecode.KismetPropertyPointer, UAssetAPI'
        New = [pscustomobject] [ordered] @{
            '$type' = 'UAssetAPI.UnrealTypes.FFieldPath, UAssetAPI'
            Path = @($throttleDoubleName)
            ResolvedOwner = $uberGraphPackageIndex
        }
    }
}
$squaredInputExpression = [pscustomobject] [ordered] @{
    '$type' = 'UAssetAPI.Kismet.Bytecode.Expressions.EX_CallMath, UAssetAPI'
    StackNode = $multiplyDoubleImport
    Parameters = @(
        (Copy-JsonNode $throttleExpression),
        (Copy-JsonNode $throttleExpression)
    )
}
$speedExpression = [pscustomobject] [ordered] @{
    '$type' = 'UAssetAPI.Kismet.Bytecode.Expressions.EX_CallMath, UAssetAPI'
    StackNode = $multiplyDoubleImport
    Parameters = @(
        $squaredInputExpression,
        [pscustomobject] [ordered] @{
            '$type' = 'UAssetAPI.Kismet.Bytecode.Expressions.EX_DoubleConst, UAssetAPI'
            Value = $RampSpeed
        }
    )
}

$targetCall.Parameters[3] = $speedExpression

$draftJson = Join-Path $outputPath 'draft.json'
$baselineAsset = Join-Path $outputPath 'baseline-roundtrip.uasset'
$draftAsset = Join-Path $outputPath 'draft.uasset'
$normalizedJson = Join-Path $outputPath 'normalized.json'
Invoke-UAssetGui @('fromjson', $sourcePath, $baselineAsset, $mappingsPath) $baselineAsset
Write-JsonFile $asset $draftJson
Invoke-UAssetGui @('fromjson', $draftJson, $draftAsset, $mappingsPath) $draftAsset
Invoke-UAssetGui @('tojson', $draftAsset, $normalizedJson, '5.7', $mappingsPath) $normalizedJson

$normalizedAsset = Get-Content -LiteralPath $normalizedJson -Raw | ConvertFrom-Json -Depth $jsonDepth
$normalizedUberGraph = Get-UberGraph $normalizedAsset
if ($normalizedUberGraph.ScriptBytecode.Count -ne $uberGraph.ScriptBytecode.Count) {
    throw 'The draft round-trip changed the UberGraph statement count'
}

$baselineUexp = [IO.Path]::ChangeExtension($baselineAsset, '.uexp')
$draftUexp = [IO.Path]::ChangeExtension($draftAsset, '.uexp')
$indexDelta = (Get-Item -LiteralPath $draftUexp).Length - (Get-Item -LiteralPath $baselineUexp).Length
if ($indexDelta -le 0) {
    throw "Expected a positive serialized bytecode delta, got $indexDelta"
}
$indexDelta = [uint32] $indexDelta

Update-JumpTargets $normalizedUberGraph.ScriptBytecode $insertionIndex $indexDelta
$entrypointCount = Update-UberGraphEntrypoints $normalizedAsset $uberGraphPackageIndex $insertionIndex $indexDelta

$finalJson = Join-Path $outputPath 'persistent-throttle.json'
$normalizedAssetPath = Join-Path $outputPath 'normalized-patched.uasset'
$finalAsset = Join-Path $outputPath 'BP_Forklift_Possesable.uasset'
$checkJson = Join-Path $outputPath 'persistent-throttle-check.json'
Write-JsonFile $normalizedAsset $finalJson
Invoke-UAssetGui @('fromjson', $finalJson, $normalizedAssetPath, $mappingsPath) $normalizedAssetPath

# UAssetGUI normalizes thousands of unrelated bytes in this cooked package's
# header. The same-size constant-only override proved that Voyage accepts the
# original retoc header, so preserve it and overlay only the size/offset bytes
# that differ between UAssetGUI's baseline and patched headers.
$sourceHeader = [IO.File]::ReadAllBytes($sourceUassetPath)
$baselineHeader = [IO.File]::ReadAllBytes($baselineAsset)
$patchedHeader = [IO.File]::ReadAllBytes($normalizedAssetPath)
if ($sourceHeader.Length -ne $baselineHeader.Length -or
    $sourceHeader.Length -ne $patchedHeader.Length) {
    throw 'Header lengths differ; refusing to overlay normalized export offsets'
}
$headerPatchCount = 0
for ($index = 0; $index -lt $baselineHeader.Length; $index++) {
    if ($baselineHeader[$index] -eq $patchedHeader[$index]) { continue }
    if ($sourceHeader[$index] -ne $baselineHeader[$index]) {
        throw "Original header differs at required patch offset $index"
    }
    $sourceHeader[$index] = $patchedHeader[$index]
    $headerPatchCount++
}
[IO.File]::WriteAllBytes($finalAsset, $sourceHeader)
Copy-Item -LiteralPath ([IO.Path]::ChangeExtension($normalizedAssetPath, '.uexp')) `
    -Destination ([IO.Path]::ChangeExtension($finalAsset, '.uexp'))

Invoke-UAssetGui @('tojson', $finalAsset, $checkJson, '5.7', $mappingsPath) $checkJson

$checkedAsset = Get-Content -LiteralPath $checkJson -Raw | ConvertFrom-Json -Depth $jsonDepth
$checkedUberGraph = Get-UberGraph $checkedAsset
$checkedTarget = $checkedUberGraph.ScriptBytecode[$targetPosition].Expression
if ($checkedTarget.StackNode -ne $fInterpToConstantImport -or
    $checkedTarget.Parameters[3].StackNode -ne $multiplyDoubleImport -or
    $checkedTarget.Parameters[3].Parameters[0].StackNode -ne $multiplyDoubleImport -or
    [double] $checkedTarget.Parameters[3].Parameters[1].Value -ne $RampSpeed) {
    throw 'Final round-trip did not preserve the persistent throttle expression'
}
if ((Get-Item -LiteralPath ([IO.Path]::ChangeExtension($finalAsset, '.uexp'))).Length -ne
    (Get-Item -LiteralPath $draftUexp).Length) {
    throw 'Updating control-flow offsets unexpectedly changed the serialized bytecode size'
}

Write-Host "Persistent throttle asset built successfully"
Write-Host 'Formula: FInterpTo_Constant(Current, Target, DeltaTime, ThrottleInput^2 * RampSpeed)'
Write-Host "Ramp speed: $RampSpeed"
Write-Host "Insertion index: $InsertionIndex"
Write-Host "Serialized bytecode delta: $indexDelta"
Write-Host "Updated UberGraph entrypoints: $entrypointCount"
Write-Host "Original-header bytes patched: $headerPatchCount"
Write-Host "Output: $finalAsset"
