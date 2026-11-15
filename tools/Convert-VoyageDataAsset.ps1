[CmdletBinding()]
param(
    [Parameter(Mandatory)][ValidatePattern('^/Game/[A-Za-z0-9_/]+$')][string]$Package,
    [Parameter(Mandatory)][ValidatePattern('^/Script/[A-Za-z0-9_]+\.[A-Za-z0-9_]+$')][string]$ExpectedClass,
    [Parameter(Mandatory)][string]$SerializationContract,
    [Parameter(Mandatory)][ValidatePattern('^[0-9]+\.[0-9]+$')][string]$EngineVersion,
    [Parameter(Mandatory)][string]$CookedAsset,
    [Parameter(Mandatory)][string]$NativeReadback,
    [Parameter(Mandatory)][string]$MappingsPath,
    [Parameter(Mandatory)][string]$OutputRoot
)
$ErrorActionPreference='Stop'
Set-StrictMode -Version Latest
$repo=[IO.Path]::GetFullPath((Join-Path $PSScriptRoot '..'))
function Resolve-WorkingPath([string]$Path) {
    $absolute=[IO.Path]::GetFullPath($ExecutionContext.SessionState.Path.GetUnresolvedProviderPathFromPSPath($Path))
    for ($cursor=$absolute; $cursor; $cursor=Split-Path -Parent $cursor) {
        if ((Test-Path -LiteralPath $cursor) -and ((Get-Item -LiteralPath $cursor -Force).Attributes -band [IO.FileAttributes]::ReparsePoint)) {
            throw "Linked working path is unsupported: $cursor"
        }
    }
    $absolute
}
$output=Resolve-WorkingPath $OutputRoot
$CookedAsset=Resolve-WorkingPath $CookedAsset
$boundary=(Join-Path $repo 'Tmp')+[IO.Path]::DirectorySeparatorChar
if(-not $output.StartsWith($boundary,[StringComparison]::OrdinalIgnoreCase) -or (Test-Path $output)){
    throw 'DataAsset conversion needs a fresh output directory below repository Tmp.'
}
if (-not $CookedAsset.StartsWith($boundary,[StringComparison]::OrdinalIgnoreCase) -or
    [IO.Path]::GetExtension($CookedAsset) -ine '.uasset' -or -not (Test-Path -LiteralPath $CookedAsset -PathType Leaf) -or
    -not (Test-Path -LiteralPath ([IO.Path]::ChangeExtension($CookedAsset,'.uexp')) -PathType Leaf)) {
    throw 'Select a disposable cooked .uasset/.uexp pair below repository Tmp.'
}
Resolve-WorkingPath ([IO.Path]::ChangeExtension($CookedAsset,'.uexp')) | Out-Null
$stem=[IO.Path]::GetFileNameWithoutExtension($CookedAsset)
if (@(Get-ChildItem -LiteralPath (Split-Path -Parent $CookedAsset) -File | Where-Object {
    $_.Name.StartsWith($stem+'.',[StringComparison]::OrdinalIgnoreCase) -and $_.Extension -notin @('.uasset','.uexp')
}).Count) { throw 'Additional package companions are unsupported by DataAsset conversion.' }
$contractPath=(Resolve-Path -LiteralPath $SerializationContract).Path
$NativeReadback=(Resolve-Path -LiteralPath $NativeReadback).Path
$MappingsPath=(Resolve-Path -LiteralPath $MappingsPath).Path
$null=New-Item -ItemType Directory -Path $output
$python=Join-Path $env:USERPROFILE '.cache/codex-runtimes/codex-primary-runtime/dependencies/python/python.exe'
$codec=Join-Path $PSScriptRoot 'AssetPackaging/data_asset.py'
$writer=Join-Path $repo '.tools/bin/UAssetGUI.exe'
function Invoke-Writer([string[]]$Arguments,[string[]]$Expected){
    $r=& (Join-Path $PSScriptRoot 'Invoke-VoyageBoundedTool.ps1') -Executable $writer -Arguments $Arguments -OutputRoot (Join-Path $output 'tool-runs') -MemoryLimitMB 2048 -TimeoutSeconds 45
    foreach($path in $Expected){if(-not(Test-Path -LiteralPath $path -PathType Leaf)){throw "Writer output missing: $path"}}
    $r
}
$fresh=Join-Path $output 'fresh-cook.json'
$null=Invoke-Writer @('--portable','tojson',$CookedAsset,$fresh,$EngineVersion,$MappingsPath) @($fresh)
& $python -B $codec compare-native --package $Package --class $ExpectedClass --source $fresh --actual $NativeReadback --output (Join-Path $output 'native-cook.checked.json') | Write-Host
if($LASTEXITCODE){throw 'Fresh cook differs from maintained DataAsset state'}
$inputJson=Join-Path $output 'unversioned-input.json'
& $python -B $codec convert --package $Package --class $ExpectedClass --source $fresh --contract $contractPath --output $inputJson | Write-Host
if($LASTEXITCODE){throw 'DataAsset conversion input rejected'}
$converted=Join-Path $output ([IO.Path]::GetFileName($CookedAsset))
$convertedExport=[IO.Path]::ChangeExtension($converted,'.uexp')
$write=Invoke-Writer @('--portable','fromjson',$inputJson,$converted,$MappingsPath,$EngineVersion) @($converted,$convertedExport)
$readback=Join-Path $output 'readback.json'
$read=Invoke-Writer @('--portable','tojson',$converted,$readback,$EngineVersion,$MappingsPath) @($readback)
& $python -B $codec compare --package $Package --class $ExpectedClass --source $fresh --actual $readback --output (Join-Path $output 'conversion.checked.json') | Write-Host
if($LASTEXITCODE){throw 'Serialization conversion changed the fresh cooked payload'}
$contract=Get-Content -LiteralPath $contractPath -Raw|ConvertFrom-Json
$actual=Get-Content -LiteralPath $readback -Raw|ConvertFrom-Json
foreach($property in $contract.PSObject.Properties){
    if(($property.Value|ConvertTo-Json -Depth 30 -Compress) -cne ($actual.($property.Name)|ConvertTo-Json -Depth 30 -Compress)){
        throw "DataAsset serialization contract mismatch: $($property.Name)"
    }
}
Copy-Item -LiteralPath $converted -Destination $CookedAsset -Force
Copy-Item -LiteralPath $convertedExport -Destination ([IO.Path]::ChangeExtension($CookedAsset,'.uexp')) -Force
[pscustomobject]@{status='passed';readbackPath=$readback;write=$write;readback=$read}
