[CmdletBinding()]
param(
    [ValidateSet('ammo','gun','skill')][string]$Asset = 'ammo',
    [Parameter(Mandatory)][string]$CookedAsset,
    [Parameter(Mandatory)][string]$NativeReadback,
    [Parameter(Mandatory)][string]$MappingsPath,
    [Parameter(Mandatory)][string]$OutputRoot
)
$ErrorActionPreference='Stop'
Set-StrictMode -Version Latest
$repo=[IO.Path]::GetFullPath((Join-Path $PSScriptRoot '../../..'))
$output=[IO.Path]::GetFullPath($OutputRoot)
$boundary=(Join-Path $repo 'Tmp/Railgun')+[IO.Path]::DirectorySeparatorChar
if(-not $output.StartsWith($boundary,[StringComparison]::OrdinalIgnoreCase) -or (Test-Path $output)){
    throw 'DataAsset conversion needs a fresh output directory below Tmp/Railgun.'
}
$null=New-Item -ItemType Directory -Path $output
$python=Join-Path $env:USERPROFILE '.cache/codex-runtimes/codex-primary-runtime/dependencies/python/python.exe'
$codec=Join-Path $PSScriptRoot 'railgun_data_asset.py'
$contractPath=Join-Path $PSScriptRoot ($Asset+'-serialization.json')
$writer=Join-Path $repo '.tools/bin/UAssetGUI.exe'
function Invoke-Writer([string[]]$Arguments,[string[]]$Expected){
    $r=& (Join-Path $repo 'tools/Invoke-VoyageBoundedTool.ps1') -Executable $writer -Arguments $Arguments -OutputRoot (Join-Path $output 'tool-runs') -MemoryLimitMB 2048 -TimeoutSeconds 45
    foreach($path in $Expected){if(-not(Test-Path -LiteralPath $path -PathType Leaf)){throw "Writer output missing: $path"}}
    $r
}
$fresh=Join-Path $output 'fresh-cook.json'
$null=Invoke-Writer @('--portable','tojson',$CookedAsset,$fresh,'5.8',$MappingsPath) @($fresh)
& $python $codec compare-native --asset $Asset --source $fresh --actual $NativeReadback --output (Join-Path $output 'native-cook.checked.json') | Write-Host
if($LASTEXITCODE){throw 'Fresh cook differs from maintained DataAsset state'}
$inputJson=Join-Path $output 'unversioned-input.json'
& $python $codec convert --asset $Asset --source $fresh --contract $contractPath --output $inputJson | Write-Host
if($LASTEXITCODE){throw 'DataAsset conversion input rejected'}
$converted=Join-Path $output ([IO.Path]::GetFileName($CookedAsset))
$convertedExport=[IO.Path]::ChangeExtension($converted,'.uexp')
$write=Invoke-Writer @('--portable','fromjson',$inputJson,$converted,$MappingsPath,'5.8') @($converted,$convertedExport)
$readback=Join-Path $output 'readback.json'
$read=Invoke-Writer @('--portable','tojson',$converted,$readback,'5.8',$MappingsPath) @($readback)
& $python $codec compare --asset $Asset --source $fresh --actual $readback --output (Join-Path $output 'conversion.checked.json') | Write-Host
if($LASTEXITCODE){throw 'Unversioned conversion changed the fresh cooked payload'}
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
