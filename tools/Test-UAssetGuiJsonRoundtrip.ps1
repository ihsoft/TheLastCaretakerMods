# Development regression. Inputs are read-only; outputs remain under artifacts.
[CmdletBinding()]
param(
    [Parameter(Mandatory = $true)][string]$CandidateGui,
    [Parameter(Mandatory = $true)][string]$InputAsset,
    [Parameter(Mandatory = $true)][string]$Mappings,
    [ValidateSet('5.7', '5.8')][string]$EngineVersion = '5.8',
    [switch]$TestCanPlace
)
$ErrorActionPreference = 'Stop'
Set-StrictMode -Version Latest
$repo = (Resolve-Path -LiteralPath (Join-Path $PSScriptRoot '..')).Path
$gui = (Resolve-Path -LiteralPath $CandidateGui).Path
$inputPath = (Resolve-Path -LiteralPath $InputAsset).Path
$mappingPath = (Resolve-Path -LiteralPath $Mappings).Path
$run = Join-Path $repo ('artifacts\tests\gui-json-' + [Guid]::NewGuid().ToString('N'))
[IO.Directory]::CreateDirectory($run) | Out-Null
$bounded = Join-Path $PSScriptRoot 'Invoke-VoyageBoundedTool.ps1'
function Invoke-Gui([string[]]$GuiArguments) {
    & $bounded -Executable $gui -Arguments (@('--portable') + $GuiArguments) -MemoryLimitMB 2048 -TimeoutSeconds 45 | Out-Null
}
function PayloadHashes([string]$Path) {
    @($Path, [IO.Path]::ChangeExtension($Path, '.uexp')) | ForEach-Object {
        if (Test-Path -LiteralPath $_) { (Get-FileHash -LiteralPath $_ -Algorithm SHA256).Hash }
    }
}
function Assert-Parsed($Asset) {
    if (@($Asset.Exports | Where-Object { $_.'$type' -like '*RawExport*' }).Count -ne 0) {
        throw 'Roundtrip produced RawExport.'
    }
}
$before = (PayloadHashes $inputPath) -join '|'
$originalJson = Join-Path $run 'original.json'
Invoke-Gui @('tojson', $inputPath, $originalJson, $EngineVersion, $mappingPath)
$original = Get-Content -LiteralPath $originalJson -Raw | ConvertFrom-Json
Assert-Parsed $original
if ($null -ne $original.PSObject.Properties['SpecifiedEngineVersion']) { throw 'Runtime engine hint leaked into JSON.' }
$cases = @()
foreach ($mode in @('unchanged', 'obsolete-hint', 'changed')) {
    if ($mode -eq 'changed' -and -not $TestCanPlace) { continue }
    $data = Get-Content -LiteralPath $originalJson -Raw | ConvertFrom-Json
    if ($mode -eq 'obsolete-hint') {
        $hint = if ($EngineVersion -eq '5.8') { 'VER_UE5_7' } else { 'VER_UE5_8' }
        $data | Add-Member -NotePropertyName SpecifiedEngineVersion -NotePropertyValue $hint
    }
    if ($mode -eq 'changed') {
        $functions = @($data.Exports | Where-Object ObjectName -eq 'GenerateAndSetLocation')
        if ($functions.Count -ne 1) { throw 'Expected exactly one GenerateAndSetLocation function.' }
        $assignments = @($functions[0].ScriptBytecode | Where-Object { $_.'$type' -like '*EX_LetBool*' -and ($_ | ConvertTo-Json -Depth 100 -Compress).Contains('CanPlace') })
        if ($assignments.Count -ne 1) { throw 'Expected exactly one CanPlace assignment.' }
        $assignments[0].AssignmentExpression = [pscustomobject]@{ '$type' = 'UAssetAPI.Kismet.Bytecode.Expressions.EX_True, UAssetAPI' }
    }
    $json = Join-Path $run ($mode + '.json')
    [IO.File]::WriteAllText($json, ($data | ConvertTo-Json -Depth 100))
    $output = Join-Path $run ($mode + '.uasset')
    $arguments = @('fromjson', $json, $output, $mappingPath, $EngineVersion)
    Invoke-Gui $arguments
    $reopenedJson = Join-Path $run ($mode + '-reopened.json')
    Invoke-Gui @('tojson', $output, $reopenedJson, $EngineVersion, $mappingPath)
    $reopened = Get-Content -LiteralPath $reopenedJson -Raw | ConvertFrom-Json
    Assert-Parsed $reopened
    if ($null -ne $reopened.PSObject.Properties['SpecifiedEngineVersion']) { throw 'Runtime engine hint leaked into reopened JSON.' }
    $equal = ((PayloadHashes $output) -join '|') -ceq $before
    foreach ($field in @('ObjectVersion', 'ObjectVersionUE5', 'CustomVersionContainer')) {
        if (($original.$field | ConvertTo-Json -Depth 100 -Compress) -cne ($reopened.$field | ConvertTo-Json -Depth 100 -Compress)) {
            throw "$mode changed $field."
        }
    }
    # UAssetAPI deliberately excludes OverrideNameMapHashes from full JSON.
    # Report total byte identity separately; require unchanged export payload.
    if ($mode -ne 'changed') {
        $sourceUexp = [IO.Path]::ChangeExtension($inputPath, '.uexp')
        if (Test-Path -LiteralPath $sourceUexp) {
            if ((Get-FileHash -LiteralPath $sourceUexp).Hash -cne (Get-FileHash -LiteralPath ([IO.Path]::ChangeExtension($output, '.uexp'))).Hash) {
                throw "$mode changed export payload."
            }
        }
    }
    if ($mode -eq 'changed') {
        $function = $reopened.Exports | Where-Object ObjectName -eq 'GenerateAndSetLocation'
        $assignment = @($function.ScriptBytecode | Where-Object { $_.'$type' -like '*EX_LetBool*' -and ($_ | ConvertTo-Json -Depth 100 -Compress).Contains('CanPlace') })
        if ($assignment.Count -ne 1 -or $assignment[0].AssignmentExpression.'$type' -notlike '*EX_True,*') { throw 'True assignment did not survive reopening.' }
    }
    $cases += [pscustomobject]@{ mode = $mode; byteIdentical = $equal; rawExports = 0; output = $output }
}
if (((PayloadHashes $inputPath) -join '|') -cne $before) { throw 'Source changed during the test.' }
$result = [pscustomobject]@{
    status = 'passed'; engineVersion = $EngineVersion; cases = $cases
    guiSha256 = (Get-FileHash -LiteralPath $gui -Algorithm SHA256).Hash
    mappingSha256 = (Get-FileHash -LiteralPath $mappingPath -Algorithm SHA256).Hash
    sourcePayloadHashes = $before; reportPath = (Join-Path $run 'summary.json')
}
[IO.File]::WriteAllText($result.reportPath, ($result | ConvertTo-Json -Depth 8))
$result
