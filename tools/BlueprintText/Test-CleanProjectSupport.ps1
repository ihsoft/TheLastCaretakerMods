<# Clean-checkout bootstrap and shared reuse with native support and saved custom call nodes. #>
[CmdletBinding()]
param([Parameter(Mandatory)][string]$OutputRoot,
      [string]$EngineRoot='P:/UnrealEngine/UE_5.8', [string]$PythonPath)
$ErrorActionPreference='Stop'
Set-StrictMode -Version Latest
Import-Module (Join-Path $PSScriptRoot '../unreal-coordination.psm1') -Force
if(-not $env:CODEX_UNREAL_QUEUE_LEASE){Invoke-UnrealQueueEntry -ScriptPath $PSCommandPath -Parameters $PSBoundParameters -Operation 'BPT clean checkout bootstrap';return}
$owner=[IO.Path]::GetFullPath((Join-Path $PSScriptRoot '../../Tmp/BlueprintText')).TrimEnd('\')+'\'
$output=[IO.Path]::GetFullPath($OutputRoot)
if(-not $output.StartsWith($owner,[StringComparison]::OrdinalIgnoreCase) -or (Test-Path -LiteralPath $output)){throw 'Use a fresh directory below Tmp/BlueprintText.'}
if(-not $PythonPath){$PythonPath=Join-Path $env:USERPROFILE '.cache/codex-runtimes/codex-primary-runtime/dependencies/python/python.exe'}
$tool=Join-Path $PSScriptRoot 'Invoke-BlueprintWorkspace.ps1'
$editor=Join-Path $EngineRoot 'Engine/Binaries/Win64/UnrealEditor-Cmd.exe'
$seed=Join-Path $output 'Seed'
[void](New-Item -ItemType Directory -Path $seed)
$project=Join-Path $seed 'Bootstrap.uproject'
'{"FileVersion":3,"EngineAssociation":"5.8"}' | Set-Content -LiteralPath $project -Encoding UTF8
Copy-Item -LiteralPath (Join-Path $PSScriptRoot '../UnrealEditorSupport/tests/Fixture') -Destination (Join-Path $seed 'EditorSupport') -Recurse
[void](New-Item -ItemType Directory -Path (Join-Path $seed 'TestOverlay'))
# A fresh fixture identity keeps the changed variant cold on repeated test runs.
$variantFunction='EditorSupportCacheVariant_'+[guid]::NewGuid().ToString('N')
"int $variantFunction() { return 1; }" | Set-Content (Join-Path $seed 'TestOverlay/CacheVariant.cpp')
@{schema=1;plugins=@(@{source='EditorSupport/SupportFixture.uplugin';overlays=@(@{source='../TestOverlay';destination='Source/SupportFixture'})})} | ConvertTo-Json -Depth 5 | Set-Content (Join-Path $seed 'EditorSupport.json')
function Prepare([string]$Name){
    $result=& $tool -Action Prepare -ProjectFile $project -WorkspaceRoot (Join-Path $output $Name) -EngineRoot $EngineRoot -PythonPath $PythonPath
    $result | Set-Content (Join-Path $output ($Name+'.json'))
    return ($result | ConvertFrom-Json)
}
function Engine([string]$Mode,[string]$Name){
    & $editor $project '-run=BlueprintAssetText' ('-Mode='+$Mode) '-Expected=7' '-unattended' '-nop4' '-nosplash' '-nullrhi' ('-abslog='+(Join-Path $output ($Name+'-unreal.log'))) *> (Join-Path $output ($Name+'.log'))
    if($LASTEXITCODE -ne 0){throw "$Mode failed; see $output/$Name.log"}
}
$timer=[Diagnostics.Stopwatch]::StartNew()
$initial=Prepare 'seed-prepare'
$initialSeconds=$timer.Elapsed.TotalSeconds
Engine 'ArgumentFixture' 'fixture'
Engine 'ArgumentCheck' 'seed-check'
# Copy only authoritative inputs, never ready plugins, caches, receipts or binaries.
$checkout=Join-Path $output 'Checkout'
[void](New-Item -ItemType Directory -Path $checkout)
foreach($name in @('Bootstrap.uproject','EditorSupport.json','EditorSupport','TestOverlay','Content')){
    Copy-Item -LiteralPath (Join-Path $seed $name) -Destination $checkout -Recurse
}
$project=Join-Path $checkout 'Bootstrap.uproject'
$before=[IO.File]::ReadAllBytes($project)
$descriptor=Get-Content -LiteralPath $project -Raw | ConvertFrom-Json
if(@($descriptor.Plugins | Where-Object {$_.Name -eq 'BlueprintTextWorkspace' -and $_.Enabled}).Count -ne 1){throw 'Fixture requires mandatory BPT reference.'}
if(Test-Path (Join-Path $checkout 'Plugins')){throw 'Checkout must start without plugins.'}
$packages=@{}
foreach($name in @('BPTArgumentCaller','BPTArgumentCallee')){$packages[$name]=[IO.File]::ReadAllBytes((Join-Path $checkout ('Content/'+$name+'.uasset')))}
$timer.Restart()
$cold=Prepare 'cold-prepare'
$reuseSeconds=$timer.Elapsed.TotalSeconds
if($cold.binaryProvider.built -or $cold.projectSupport[0].built -or $cold.binaryProvider.reuse -ne 'shared' -or $cold.projectSupport[0].reuse -ne 'shared'){throw 'Cold checkout must reuse both shared providers.'}
if(-not [Linq.Enumerable]::SequenceEqual([byte[]]$before,[byte[]][IO.File]::ReadAllBytes($project))){throw 'Preparation changed required plugin references.'}
# No EnablePlugins override: saved custom nodes must survive an ordinary editor launch.
Engine 'ArgumentCheck' 'cold-check'
$repeat=Prepare 'repeat-prepare'
if($repeat.binaryProvider.built -or $repeat.projectSupport[0].built){throw 'Unchanged providers must be reused.'}
$work=Join-Path $output 'workspace'
& $tool -Action Export -ProjectFile $project -WorkspaceRoot $work -Asset @('/Game/BPTArgumentCallee','/Game/BPTArgumentCaller') -EngineRoot $EngineRoot -PythonPath $PythonPath
foreach($name in $packages.Keys){
    if(-not [Linq.Enumerable]::SequenceEqual([byte[]]$packages[$name],[byte[]][IO.File]::ReadAllBytes((Join-Path $checkout ('Content/'+$name+'.uasset'))))){throw 'Bootstrap/read changed an authoritative asset.'}
}
& $PythonPath -B (Join-Path $PSScriptRoot 'argument_fixture.py') $work 'move'
if($LASTEXITCODE -ne 0){throw 'Cannot edit test coordinate.'}
& $tool -Action Apply -WorkspaceRoot $work -EngineRoot $EngineRoot -PythonPath $PythonPath
Engine 'ArgumentCheck' 'changed-reopen'
& $editor $project '-run=cook' '-targetplatform=Windows' '-unattended' '-nop4' '-nullrhi' '-SkipZenStore' '-CookAll' '-NoDefaultMaps' ('-OutputDir='+(Join-Path $output 'cooked')) ('-abslog='+(Join-Path $output 'cook-unreal.log')) *> (Join-Path $output 'cook.log')
if($LASTEXITCODE -ne 0){throw "Cook failed; see $output/cook.log"}
$cooked=Join-Path $output 'cooked/Bootstrap/Content/BPTArgumentCaller.uasset'
if(-not (Test-Path -LiteralPath $cooked)){throw 'Missing cooked Blueprint.'}
# Only native support changes; the common BPT package remains reusable.
"int $variantFunction() { return 2; }" | Set-Content (Join-Path $checkout 'TestOverlay/CacheVariant.cpp')
$variant=Prepare 'overlay-change'
if(-not $variant.projectSupport[0].built -or $variant.binaryProvider.built){throw 'Overlay change must rebuild only native support.'}
"int $variantFunction() { return 1; }" | Set-Content (Join-Path $checkout 'TestOverlay/CacheVariant.cpp')
$restored=Prepare 'overlay-restore'
if($restored.projectSupport[0].built -or $restored.projectSupport[0].reuse -ne 'shared' -or $restored.binaryProvider.built){throw 'Restored overlay must reuse its retained variant.'}
# The explicit public source mode shares the same mechanism.
$direct=Join-Path $output 'Direct'
[void](New-Item -ItemType Directory -Path $direct)
'{"FileVersion":3,"EngineAssociation":"5.8"}' | Set-Content (Join-Path $direct 'Direct.uproject')
$composed=Join-Path $output 'DirectSource'
Copy-Item -LiteralPath (Join-Path $seed 'EditorSupport') -Destination $composed -Recurse
Copy-Item -LiteralPath (Join-Path $seed 'TestOverlay/CacheVariant.cpp') -Destination (Join-Path $composed 'Source/SupportFixture/CacheVariant.cpp')
& (Join-Path $PSScriptRoot '../Prepare-UnrealEditorSupport.ps1') -ProjectFile (Join-Path $direct 'Direct.uproject') -PluginSource (Join-Path $composed 'SupportFixture.uplugin') -EngineRoot $EngineRoot -OutputRoot (Join-Path $output 'source-mode')
$sourceMode=Get-Content (Join-Path $output 'source-mode/result.json') -Raw | ConvertFrom-Json
if($sourceMode.built -or $sourceMode.reuse -ne 'shared'){throw 'Public source mode must reuse identical composed inputs.'}
@{status='passed';cleanCheckout=$true;mandatoryReferencesPreserved=$true;sourceAssetsPreserved=$true;providerReuse=$true;freshCopyBuilds=0;initialBuilds=([int]$initial.binaryProvider.built+[int]$initial.projectSupport[0].built);initialSeconds=$initialSeconds;reuseSeconds=$reuseSeconds;overlayInvalidation=$true;publicSourceReuse=$true;customCallNodesReopened=$true;changedApply=$true;cooked=$cooked;evidence=$output} | ConvertTo-Json | Set-Content (Join-Path $output 'result.json')
Get-Content (Join-Path $output 'result.json')
