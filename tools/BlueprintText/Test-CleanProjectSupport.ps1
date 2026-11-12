<# Clean-checkout bootstrap with mandatory BPT, native support and saved custom call nodes. #>
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
@{schema=1;plugins=@(@{source=[IO.Path]::GetFullPath((Join-Path $PSScriptRoot '../UnrealEditorSupport/tests/Fixture/SupportFixture.uplugin'))})} | ConvertTo-Json -Depth 5 | Set-Content (Join-Path $seed 'EditorSupport.json')
function Prepare([string]$Name){
    $result=& $tool -Action Prepare -ProjectFile $project -WorkspaceRoot (Join-Path $output $Name) -EngineRoot $EngineRoot -PythonPath $PythonPath
    $result | Set-Content (Join-Path $output ($Name+'.json'))
    return ($result | ConvertFrom-Json)
}
function Engine([string]$Mode,[string]$Name){
    & $editor $project '-run=BlueprintAssetText' ('-Mode='+$Mode) '-Expected=7' '-unattended' '-nop4' '-nosplash' '-nullrhi' ('-abslog='+(Join-Path $output ($Name+'-unreal.log'))) *> (Join-Path $output ($Name+'.log'))
    if($LASTEXITCODE -ne 0){throw "$Mode failed; see $output/$Name.log"}
}
$null=Prepare 'seed-prepare'
Engine 'ArgumentFixture' 'fixture'
Engine 'ArgumentCheck' 'seed-check'
# Copy only authoritative inputs, never ready plugins, caches, receipts or binaries.
$checkout=Join-Path $output 'Checkout'
[void](New-Item -ItemType Directory -Path $checkout)
foreach($name in @('Bootstrap.uproject','EditorSupport.json','Content')){
    Copy-Item -LiteralPath (Join-Path $seed $name) -Destination $checkout -Recurse
}
$project=Join-Path $checkout 'Bootstrap.uproject'
$before=[IO.File]::ReadAllBytes($project)
$descriptor=Get-Content -LiteralPath $project -Raw | ConvertFrom-Json
if(@($descriptor.Plugins | Where-Object {$_.Name -eq 'BlueprintTextWorkspace' -and $_.Enabled}).Count -ne 1){throw 'Fixture requires mandatory BPT reference.'}
if(Test-Path (Join-Path $checkout 'Plugins')){throw 'Checkout must start without plugins.'}
$packages=@{}
foreach($name in @('BPTArgumentCaller','BPTArgumentCallee')){$packages[$name]=[IO.File]::ReadAllBytes((Join-Path $checkout ('Content/'+$name+'.uasset')))}
$cold=Prepare 'cold-prepare'
if(-not $cold.binaryProvider.built -or -not $cold.projectSupport[0].built){throw 'Cold checkout must build both providers.'}
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
@{status='passed';cleanCheckout=$true;mandatoryReferencesPreserved=$true;sourceAssetsPreserved=$true;providerReuse=$true;customCallNodesReopened=$true;changedApply=$true;cooked=$cooked;evidence=$output} | ConvertTo-Json | Set-Content (Join-Path $output 'result.json')
Get-Content (Join-Path $output 'result.json')
