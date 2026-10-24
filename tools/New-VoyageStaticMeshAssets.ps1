# PUBLIC TOOL: imports one or more GLB meshes through Unreal's native StaticMesh
# pipeline, builds optional Nanite and ray-tracing data, cooks only the requested
# /Game packages, and returns loose cooked files. It never packages or installs.

<#
.SYNOPSIS
Builds loose cooked Voyage StaticMesh assets from a strict JSON manifest.

.DESCRIPTION
Normal callers use the published UE 5.8 host. OutputRoot must be a new directory
below this repository's Tmp directory. Relative source paths are resolved from
the manifest directory. Only /Game destinations are supported in schema v1.

.EXAMPLE
.\tools\New-VoyageStaticMeshAssets.ps1 -ShowSchema

.EXAMPLE
.\tools\New-VoyageStaticMeshAssets.ps1 -ShowExample

.EXAMPLE
.\tools\New-VoyageStaticMeshAssets.ps1 `
  -Manifest .\Tmp\mesh-inputs\build.json `
  -OutputRoot .\Tmp\Example\static-mesh-assets
#>
[CmdletBinding(DefaultParameterSetName='Build')]
param(
    [Parameter(Mandatory=$true, ParameterSetName='Build')][string]$Manifest,
    [Parameter(Mandatory=$true, ParameterSetName='Build')][string]$OutputRoot,
    [Parameter(ParameterSetName='Build')][string]$EvidenceRoot,
    [Parameter(ParameterSetName='Build')][string]$EngineRoot = 'K:\Epic Games\UE_5.8\Engine',
    [Parameter(Mandatory=$true, ParameterSetName='Schema')]
    [Alias('PrintManifestSchema')][switch]$ShowSchema,
    [Parameter(Mandatory=$true, ParameterSetName='Example')][switch]$ShowExample
)

$ErrorActionPreference = 'Stop'
Set-StrictMode -Version Latest
$repo = (Resolve-Path -LiteralPath (Join-Path $PSScriptRoot '..')).Path
$tmp = [IO.Path]::GetFullPath((Join-Path $repo 'Tmp'))

function Write-Utf8Json([string]$Path, $Value, [int]$Depth = 20) {
    $parent = Split-Path -Parent $Path
    if ($parent) { $null = New-Item -ItemType Directory -Path $parent -Force }
    [IO.File]::WriteAllText($Path, ($Value | ConvertTo-Json -Depth $Depth),
        (New-Object Text.UTF8Encoding($false)))
}
function Get-Names($Value) { @($Value.PSObject.Properties | ForEach-Object Name) }
function Assert-Fields($Value, [string[]]$Allowed, [string]$At) {
    foreach ($name in @(Get-Names $Value)) {
        if ($Allowed -cnotcontains $name) { throw "Unknown field '$name' at $At." }
    }
}
function Require-Field($Value, [string]$Name, [string]$At) {
    $property = $Value.PSObject.Properties[$Name]
    if ($null -eq $property) { throw "Missing required field '$Name' at $At." }
    $property.Value
}
function Optional-Field($Value, [string]$Name, $Default) {
    $property = $Value.PSObject.Properties[$Name]
    if ($null -eq $property) { return $Default }
    $property.Value
}
function Require-String($Value, [string]$Name, [string]$At) {
    $result = Require-Field $Value $Name $At
    if ($result -isnot [string] -or [string]::IsNullOrWhiteSpace($result)) {
        throw "Field '$Name' at $At must be a non-empty string."
    }
    [string]$result
}
function Optional-String($Value, [string]$Name, [string]$At) {
    $property = $Value.PSObject.Properties[$Name]
    if ($null -eq $property) { return $null }
    if ($property.Value -isnot [string] -or [string]::IsNullOrWhiteSpace($property.Value)) {
        throw "Field '$Name' at $At must be a non-empty string when present."
    }
    [string]$property.Value
}
function As-Bool($Value, [string]$At) {
    if ($Value -isnot [bool]) { throw "$At must be a Boolean." }
    [bool]$Value
}
function As-Number($Value, [string]$At, [double]$Min, [double]$Max) {
    if ($Value -isnot [byte] -and $Value -isnot [int16] -and
        $Value -isnot [int32] -and $Value -isnot [int64] -and
        $Value -isnot [single] -and $Value -isnot [double] -and
        $Value -isnot [decimal]) { throw "$At must be numeric." }
    $number = [double]$Value
    if ([double]::IsNaN($number) -or [double]::IsInfinity($number) -or
        $number -lt $Min -or $number -gt $Max) {
        throw "$At must be between $Min and $Max."
    }
    $number
}
function As-Integer($Value, [string]$At) {
    $number = As-Number $Value $At ([int]::MinValue) ([int]::MaxValue)
    if ([math]::Truncate($number) -ne $number) { throw "$At must be an integer." }
    [int]$number
}
function Normalize-Precision($Value, [string]$At) {
    if ($Value -is [string]) {
        if ($Value -cne 'Auto') { throw "$At must be 'Auto' or an integer." }
        return -1
    }
    As-Integer $Value $At
}
function Normalize-Count($Value, [string]$At) {
    if ($null -eq $Value) { return $null }
    Assert-Fields $Value @('exact','min','max') $At
    $result = [ordered]@{}
    foreach ($name in @('exact','min','max')) {
        if ($null -ne $Value.PSObject.Properties[$name]) {
            $count = As-Integer $Value.$name "$At.$name"
            if ($count -lt 0) { throw "$At.$name must be non-negative." }
            $result[$name] = $count
        }
    }
    if ($result.Count -eq 0) { throw "$At needs exact, min, or max." }
    if ($result.Contains('exact') -and $result.Count -ne 1) { throw "$At exact cannot be combined with min/max." }
    if ($result.Contains('min') -and $result.Contains('max') -and $result.min -gt $result.max) {
        throw "$At min cannot exceed max."
    }
    $result
}
function Assert-UnderTmp([string]$Path, [string]$Label) {
    $full = [IO.Path]::GetFullPath($Path)
    $prefix = $tmp.TrimEnd('\') + '\'
    if (-not $full.StartsWith($prefix, [StringComparison]::OrdinalIgnoreCase)) {
        throw "$Label must stay below the repository Tmp directory: $full"
    }
    $full
}

if ($ShowExample) {
    [ordered]@{
        schemaVersion=1
        assets=@([ordered]@{
            id='sample'
            source='replace-with-input.glb'
            destination=[ordered]@{
                packagePath='/Game/Mods/Example/SM_Sample'
                objectName='SM_Sample'
            }
        })
    } | ConvertTo-Json -Depth 8
    return
}
if ($ShowSchema) {
    [ordered]@{
        kind='VoyageStaticMeshBuilder manifest field reference (not a JSON Schema document)'
        schemaVersion=[ordered]@{required=$true;type='integer';value=1}
        assets=[ordered]@{required=$true;type='non-empty array';entryFields=[ordered]@{
            id='required safe unique string'
            source='required .glb path; relative paths resolve from manifest directory'
            sourceMesh='optional exact imported StaticMesh name; required when discovery is ambiguous'
            destination='required object: packagePath safe /Game path only; objectName Unreal identifier; destinations unique'
            materialMode='optional enum DefaultEngine|ExternalReferences; default DefaultEngine'
            materialBindings='ExternalReferences only; array covering imported slots exactly; each has slot, objectPath, classPath'
            import='optional object; booleans generateLightmapUVs=false, removeDegenerates=false, recomputeNormals=false, recomputeTangents=true, useFullPrecisionUVs=false, useHighPrecisionTangentBasis=false'
            nanite='optional object; enabled=true, explicitTangents=false, positionPrecision=Auto|integer, normalPrecision=Auto|integer, keepPercentTriangles=1, trimRelativeError=0, generateFallback=Enabled|PlatformDefault, fallbackTarget=Auto|PercentTriangles|RelativeError, fallbackPercentTriangles=1, fallbackRelativeError=1'
            rayTracing='optional object; Boolean support=true'
            assertions='optional object; sourceTriangles, naniteInputTriangles, fallbackTriangles each accepts exact or min/max non-negative integer bounds'
        }}
        behavior=@(
            'Unknown fields fail before Unreal launches.',
            'Embedded GLB shader fidelity is unsupported and explicitly reported; materialMode controls replacement/reference behavior.',
            'Outputs are loose cooked assets only; this tool never packages or installs.'
        )
    } | ConvertTo-Json -Depth 12
    return
}

$runId = [DateTime]::UtcNow.ToString('yyyyMMdd-HHmmss') + '-' + [Guid]::NewGuid().ToString('N').Substring(0,8)
if ([string]::IsNullOrWhiteSpace($EvidenceRoot)) {
    $EvidenceRoot = Join-Path $tmp "tools\VoyageStaticMeshBuilder\build-$runId"
}
$evidence = Assert-UnderTmp $EvidenceRoot 'EvidenceRoot'
$output = Assert-UnderTmp $OutputRoot 'OutputRoot'
if (Test-Path -LiteralPath $evidence) { throw "EvidenceRoot already exists: $evidence" }
if (Test-Path -LiteralPath $output) { throw "OutputRoot already exists: $output" }
$evidencePrefix = $evidence.TrimEnd('\') + '\'
$outputPrefix = $output.TrimEnd('\') + '\'
if ($output.StartsWith($evidencePrefix, [StringComparison]::OrdinalIgnoreCase) -or
    $evidence.StartsWith($outputPrefix, [StringComparison]::OrdinalIgnoreCase)) {
    throw 'EvidenceRoot and OutputRoot must not contain one another.'
}
$null = New-Item -ItemType Directory -Path $evidence
$failurePath = Join-Path $evidence 'failure.json'

try {
    $manifestPath = (Resolve-Path -LiteralPath $Manifest).Path
    $manifestDirectory = Split-Path -Parent $manifestPath
    try { $request = Get-Content -LiteralPath $manifestPath -Raw | ConvertFrom-Json }
    catch { throw "Manifest is not valid JSON: $($_.Exception.Message)" }
    Assert-Fields $request @('schemaVersion','assets') '$'
    if ((As-Integer (Require-Field $request 'schemaVersion' '$') '$.schemaVersion') -ne 1) {
        throw 'Only schemaVersion 1 is supported.'
    }
    $assetValues = @(Require-Field $request 'assets' '$')
    if ($assetValues.Count -eq 0) { throw '$.assets must contain at least one entry.' }
    $ids = @{}; $objectPaths = @{}; $sources = @(); $assets = @()
    for ($index=0; $index -lt $assetValues.Count; $index++) {
        $asset = $assetValues[$index]; $at = "$.assets[$index]"
        Assert-Fields $asset @('id','source','sourceMesh','destination','materialMode','materialBindings','import','nanite','rayTracing','assertions') $at
        $id = Require-String $asset 'id' $at
        if ($id -cnotmatch '^[A-Za-z0-9][A-Za-z0-9._-]*$' -or $ids.ContainsKey($id)) { throw "$at.id is invalid or duplicated: $id" }
        $ids[$id] = $true
        $sourceText = Require-String $asset 'source' $at
        $sourceCandidate = if ([IO.Path]::IsPathRooted($sourceText)) { $sourceText } else { Join-Path $manifestDirectory $sourceText }
        $sourcePath = (Resolve-Path -LiteralPath $sourceCandidate).Path
        if ([IO.Path]::GetExtension($sourcePath) -cne '.glb') { throw "$at.source must resolve to a .glb file." }
        $sourceMesh = Optional-String $asset 'sourceMesh' $at
        $destination = Require-Field $asset 'destination' $at
        Assert-Fields $destination @('packagePath','objectName') "$at.destination"
        $packagePath = Require-String $destination 'packagePath' "$at.destination"
        $objectName = Require-String $destination 'objectName' "$at.destination"
        if ($packagePath -cnotmatch '^/Game/[A-Za-z0-9_/-]+$' -or $packagePath.Contains('//') -or
            $packagePath.EndsWith('/') -or $packagePath.Contains('/../') -or $packagePath.Contains('/./')) {
            throw "$at.destination.packagePath must be a safe schema-v1 /Game package path."
        }
        if ($objectName -cnotmatch '^[A-Za-z_][A-Za-z0-9_]*$') { throw "$at.destination.objectName is invalid." }
        $objectPath = "$packagePath.$objectName"
        if ($objectPaths.ContainsKey($objectPath)) { throw "Duplicate destination objectPath: $objectPath" }
        $objectPaths[$objectPath] = $true

        $materialMode = [string](Optional-Field $asset 'materialMode' 'DefaultEngine')
        if ($materialMode -cnotin @('DefaultEngine','ExternalReferences')) { throw "$at.materialMode is unsupported." }
        $bindings = @()
        [object[]]$bindingValue = @(Optional-Field $asset 'materialBindings' @())
        if ($materialMode -ceq 'DefaultEngine' -and $bindingValue.Count -ne 0) { throw "$at.materialBindings is only valid with ExternalReferences." }
        $bindingSlots = @{}
        foreach ($binding in $bindingValue) {
            Assert-Fields $binding @('slot','objectPath','classPath') "$at.materialBindings[]"
            $slot = Require-String $binding 'slot' "$at.materialBindings[]"
            if ($bindingSlots.ContainsKey($slot)) { throw "Duplicate material slot binding at ${at}: $slot" }
            $bindingSlots[$slot] = $true
            $materialObjectPath = Require-String $binding 'objectPath' "$at.materialBindings[]"
            $materialClassPath = Require-String $binding 'classPath' "$at.materialBindings[]"
            if ($materialObjectPath -cnotmatch '^/(Game|Engine)/[A-Za-z0-9_/-]+\.[A-Za-z_][A-Za-z0-9_]*$' -or
                $materialObjectPath.Contains('//') -or $materialObjectPath.Contains('/../') -or
                $materialObjectPath.Contains('/./')) {
                throw "$at.materialBindings[].objectPath must be a safe /Game or /Engine object path."
            }
            if ($materialClassPath -cnotmatch '^/Script/[A-Za-z0-9_]+\.[A-Za-z_][A-Za-z0-9_]*$') {
                throw "$at.materialBindings[].classPath must be a /Script module.class path."
            }
            $bindings += [ordered]@{slot=$slot;objectPath=$materialObjectPath;classPath=$materialClassPath}
        }
        if ($materialMode -ceq 'ExternalReferences' -and $bindings.Count -eq 0) { throw "$at ExternalReferences requires materialBindings." }

        $importIn = Optional-Field $asset 'import' ([pscustomobject]@{})
        Assert-Fields $importIn @('generateLightmapUVs','removeDegenerates','recomputeNormals','recomputeTangents','useFullPrecisionUVs','useHighPrecisionTangentBasis') "$at.import"
        $import = [ordered]@{}
        foreach ($pair in @(@('generateLightmapUVs',$false),@('removeDegenerates',$false),@('recomputeNormals',$false),@('recomputeTangents',$true),@('useFullPrecisionUVs',$false),@('useHighPrecisionTangentBasis',$false))) {
            $import[$pair[0]] = As-Bool (Optional-Field $importIn $pair[0] $pair[1]) "$at.import.$($pair[0])"
        }
        $naniteIn = Optional-Field $asset 'nanite' ([pscustomobject]@{})
        Assert-Fields $naniteIn @('enabled','explicitTangents','positionPrecision','normalPrecision','keepPercentTriangles','trimRelativeError','generateFallback','fallbackTarget','fallbackPercentTriangles','fallbackRelativeError') "$at.nanite"
        $generateFallback = [string](Optional-Field $naniteIn 'generateFallback' 'Enabled')
        $fallbackTarget = [string](Optional-Field $naniteIn 'fallbackTarget' 'Auto')
        if ($generateFallback -cnotin @('Enabled','PlatformDefault')) { throw "$at.nanite.generateFallback is unsupported." }
        if ($fallbackTarget -cnotin @('Auto','PercentTriangles','RelativeError')) { throw "$at.nanite.fallbackTarget is unsupported." }
        $nanite = [ordered]@{
            enabled=As-Bool (Optional-Field $naniteIn 'enabled' $true) "$at.nanite.enabled"
            explicitTangents=As-Bool (Optional-Field $naniteIn 'explicitTangents' $false) "$at.nanite.explicitTangents"
            positionPrecision=Normalize-Precision (Optional-Field $naniteIn 'positionPrecision' 'Auto') "$at.nanite.positionPrecision"
            normalPrecision=Normalize-Precision (Optional-Field $naniteIn 'normalPrecision' 'Auto') "$at.nanite.normalPrecision"
            keepPercentTriangles=As-Number (Optional-Field $naniteIn 'keepPercentTriangles' 1.0) "$at.nanite.keepPercentTriangles" 0 1
            trimRelativeError=As-Number (Optional-Field $naniteIn 'trimRelativeError' 0.0) "$at.nanite.trimRelativeError" 0 ([double]::MaxValue)
            generateFallback=$generateFallback; fallbackTarget=$fallbackTarget
            fallbackPercentTriangles=As-Number (Optional-Field $naniteIn 'fallbackPercentTriangles' 1.0) "$at.nanite.fallbackPercentTriangles" 0 1
            fallbackRelativeError=As-Number (Optional-Field $naniteIn 'fallbackRelativeError' 1.0) "$at.nanite.fallbackRelativeError" 0 ([double]::MaxValue)
        }
        $rayIn = Optional-Field $asset 'rayTracing' ([pscustomobject]@{})
        Assert-Fields $rayIn @('support') "$at.rayTracing"
        $ray = [ordered]@{support=As-Bool (Optional-Field $rayIn 'support' $true) "$at.rayTracing.support"}
        $assertions = [ordered]@{}
        $assertIn = Optional-Field $asset 'assertions' ([pscustomobject]@{})
        Assert-Fields $assertIn @('sourceTriangles','naniteInputTriangles','fallbackTriangles') "$at.assertions"
        foreach ($name in @('sourceTriangles','naniteInputTriangles','fallbackTriangles')) {
            if ($null -ne $assertIn.PSObject.Properties[$name]) { $assertions[$name] = Normalize-Count $assertIn.$name "$at.assertions.$name" }
        }
        $assets += [ordered]@{id=$id;sourcePath=$sourcePath;sourceMesh=$sourceMesh;packagePath=$packagePath;objectName=$objectName;materialMode=$materialMode;materialBindings=$bindings;import=$import;nanite=$nanite;rayTracing=$ray;assertions=$assertions}
        $sources += [ordered]@{path=$sourcePath;length=(Get-Item -LiteralPath $sourcePath).Length;sha256=(Get-FileHash -LiteralPath $sourcePath -Algorithm SHA256).Hash}
    }

    foreach ($asset in $assets) {
        foreach ($binding in $asset.materialBindings) {
            if ($objectPaths.ContainsKey([string]$binding.objectPath)) {
                throw "External material reference collides with a requested StaticMesh destination: $($binding.objectPath)"
            }
        }
    }

    $normalized = [ordered]@{schemaVersion=1;assets=$assets}
    $normalizedPath = Join-Path $evidence 'normalized-manifest.json'
    Write-Utf8Json $normalizedPath $normalized
    $builder = & (Join-Path $PSScriptRoot 'VoyageStaticMeshBuilder\Get-VoyageStaticMeshBuilder.ps1') -EngineRoot $EngineRoot
    $executionHost = Join-Path $evidence 'execution-host'
    Copy-Item -LiteralPath (Split-Path -Parent $builder.ProjectPath) -Destination $executionHost -Recurse
    $project = Join-Path $executionHost 'VoyageStaticMeshBuilder.uproject'
    $editor = [string]$builder.Engine.editorPath
    $summaryPath = Join-Path $evidence 'editor-summary.json'
    $generateLog = Join-Path $evidence 'generate.log'
    $generateArgs = @(('"{0}"' -f $project),'-run=BuildVoyageStaticMeshes','-unattended','-nop4','-nosplash','-nullrhi','-ddc=NoZenLocalFallback',('-LocalDataCachePath="{0}"' -f (Join-Path $evidence 'ddc')),('-BuildManifest="{0}"' -f $normalizedPath),('-OutputSummary="{0}"' -f $summaryPath),('-abslog="{0}"' -f $generateLog))
    $process = Start-Process -FilePath $editor -ArgumentList $generateArgs -Wait -PassThru -WindowStyle Hidden
    if (Test-Path -LiteralPath $summaryPath) {
        $editorSummary = Get-Content -LiteralPath $summaryPath -Raw | ConvertFrom-Json
        if ($editorSummary.status -cne 'passed') {
            $discovered = @($editorSummary.discoveredMeshes) -join ', '
            throw "Native import/build failed: $($editorSummary.errorCode): $($editorSummary.message). Discovered meshes: [$discovered]. Summary: $summaryPath"
        }
    } else {
        throw "Native import/build produced no summary ($($process.ExitCode)). Log: $generateLog"
    }
    if ($process.ExitCode -ne 0) { throw "Native import/build returned $($process.ExitCode) after a passed summary. Log: $generateLog" }

    $cookLog = Join-Path $evidence 'cook.log'
    $packages = @($assets | ForEach-Object packagePath)
    $cookArgs = @(('"{0}"' -f $project),'-run=cook','-targetplatform=Windows','-unversioned','-SkipZenStore',('-Package={0}' -f ($packages -join '+')),'-CookSinglePackageNoRefs','-unattended','-nop4','-nosplash','-nullrhi','-ddc=NoZenLocalFallback',('-LocalDataCachePath="{0}"' -f (Join-Path $evidence 'ddc')),('-abslog="{0}"' -f $cookLog))
    $process = Start-Process -FilePath $editor -ArgumentList $cookArgs -Wait -PassThru -WindowStyle Hidden
    if ($process.ExitCode -ne 0) { throw "Native cook failed ($($process.ExitCode)). Log: $cookLog" }

    $staged = Join-Path $evidence 'staged'
    $null = New-Item -ItemType Directory -Path (Join-Path $staged 'Content') -Force
    $cookedContent = Join-Path $executionHost 'Saved\Cooked\Windows\VoyageStaticMeshBuilder\Content'
    $fileRecords = @()
    foreach ($asset in $assets) {
        $relative = $asset.packagePath.Substring('/Game/'.Length)
        $sourceBase = Join-Path $cookedContent $relative
        $destBase = Join-Path (Join-Path $staged 'Content') $relative
        $null = New-Item -ItemType Directory -Path (Split-Path -Parent $destBase) -Force
        foreach ($ext in @('.uasset','.uexp')) {
            if (-not (Test-Path -LiteralPath ($sourceBase + $ext) -PathType Leaf)) { throw "Cook did not produce required companion: $sourceBase$ext" }
        }
        foreach ($ext in @('.uasset','.uexp','.ubulk','.uptnl','.m.ubulk')) {
            $from = $sourceBase + $ext
            if (Test-Path -LiteralPath $from -PathType Leaf) { Copy-Item -LiteralPath $from -Destination ($destBase + $ext) }
        }
    }
    foreach ($file in @(Get-ChildItem -LiteralPath (Join-Path $staged 'Content') -File -Recurse | Sort-Object FullName)) {
        $fileRecords += [ordered]@{path=$file.FullName.Substring($staged.Length+1).Replace('\','/');length=$file.Length;sha256=(Get-FileHash -LiteralPath $file.FullName -Algorithm SHA256).Hash}
    }
    foreach ($source in $sources) {
        if ((Get-Item -LiteralPath $source.path).Length -ne $source.length -or (Get-FileHash -LiteralPath $source.path -Algorithm SHA256).Hash -cne $source.sha256) { throw "Input changed during build: $($source.path)" }
    }
    $result = [ordered]@{schemaVersion=1;status='passed';createdAtUtc=[DateTime]::UtcNow.ToString('o');builder=[ordered]@{manifestPath=$builder.ManifestPath;inputFingerprint=$builder.InputFingerprint;engine=$builder.Engine};request=[ordered]@{manifestPath=$manifestPath;manifestSha256=(Get-FileHash -LiteralPath $manifestPath -Algorithm SHA256).Hash;sources=$sources};normalizedRequest=$normalized;assets=$editorSummary.assets;files=$fileRecords;limitations=@('Schema v1 supports /Game destinations only.','Embedded GLB shader fidelity is not preserved; materialMode explicitly selects default engine materials or external references.','Loose cooked assets are not a package, container, installation, or game-runtime proof.')}
    Write-Utf8Json (Join-Path $staged 'build-result.json') $result
    Write-Utf8Json (Join-Path $staged 'output-manifest.json') ([ordered]@{schemaVersion=1;kind='Voyage loose cooked StaticMesh assets';files=$fileRecords;assetObjectPaths=@($assets | ForEach-Object { $_.packagePath + '.' + $_.objectName })})
    Move-Item -LiteralPath $staged -Destination $output
    Remove-Item -LiteralPath $evidence -Recurse -Force
    [pscustomobject]@{status='passed';outputRoot=$output;buildResultPath=(Join-Path $output 'build-result.json');outputManifestPath=(Join-Path $output 'output-manifest.json');assetCount=$assets.Count} | ConvertTo-Json -Compress
} catch {
    $failure = [ordered]@{schemaVersion=1;status='failed';message=$_.Exception.Message;evidenceRoot=$evidence;manifest=if ($Manifest) {[IO.Path]::GetFullPath($Manifest)} else {$null};createdAtUtc=[DateTime]::UtcNow.ToString('o')}
    Write-Utf8Json $failurePath $failure
    Write-Error "StaticMesh build failed. Machine-readable failure: $failurePath`n$($_.Exception.Message)"
    exit 1
}
