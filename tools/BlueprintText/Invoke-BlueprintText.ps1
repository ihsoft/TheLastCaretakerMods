<#
.SYNOPSIS
Creates, imports, exports, and discovers a bounded declarative Blueprint-text tool.
.EXAMPLE
  .\Invoke-BlueprintText.ps1 -Action Prepare -OutputRoot .\Tmp\BlueprintText\demo
  .\Invoke-BlueprintText.ps1 -Action RefreshCatalog -ProjectRoot .\Tmp\BlueprintText\demo\Project
  .\Invoke-BlueprintText.ps1 -Action SearchCatalog -Query divide
  .\Invoke-BlueprintText.ps1 -Action Import -ProjectRoot <Project> -InputPath <blueprint.json>
  .\Invoke-BlueprintText.ps1 -Action Export -ProjectRoot <Project> -Asset /Game/BP_Example -Format compact -OutputPath <export.json>

The envelope supports one /Game asset, an engine parent class, ordinary float/double/int/bool
variables, and exactly one EventGraph. Graph bodies are either native Unreal graph text or the
bounded compact node set reported by SearchCatalog. User functions, components, macros,
timelines, and non-baseline variable defaults/attributes are outside the envelope subset.
Compact mode rejects other node classes. This is not a universal Blueprint metadata exporter.

Native exports put editable Unreal graph text in an adjacent .t3d file and reference it with
`bodyFile`; imports also accept the earlier inline `body` form. Unknown envelope, declaration,
graph, and compact-node fields fail closed instead of being discarded.

Compact bodies contain `nodes` and `links`. Reusable node factories are:
  {"id":"start","kind":"event","event":"/Script/Engine.Actor::ReceiveBeginPlay"}
  {"id":"math","kind":"call","operation":"<catalog identity>","arguments":{"left":"12","right":"2"}}
  {"id":"flow","kind":"sequence","outputs":2}
  {"id":"save","kind":"setVariable","variable":"DeclaredVariable"}
  {"id":"read","kind":"getVariable","variable":"DeclaredVariable"}
Links use catalog aliases, for example `math.result` to `save.value`; full-source omission deletes
a supported node. SearchCatalog returns copyable compactTemplate and engine-generated nativeInsertion.
#>
[CmdletBinding()]
param(
    [Parameter(Mandatory)][ValidateSet('Help','Prepare','RefreshCatalog','SearchCatalog','Import','Export','SimulateEdit','SimulateUnsupportedGraph','SimulateNonzeroDefault','SimulateConstructionLogic')][string]$Action,
    [string]$OutputRoot,
    [string]$ProjectRoot,
    [string]$InputPath,
    [string]$OutputPath,
    [string]$Asset,
    [ValidateSet('compact','native')][string]$Format='compact',
    [string]$Query,
    [ValidateRange(1,50)][int]$Limit=10,
    [string]$Variable,
    [switch]$Force,
    [string]$EngineRoot='K:\Epic Games\UE_5.8'
)
$ErrorActionPreference='Stop';Set-StrictMode -Version Latest
$repo=(Resolve-Path (Join-Path $PSScriptRoot '..\..')).Path;$tmpOwner=[IO.Path]::GetFullPath((Join-Path $repo 'Tmp/BlueprintText')).TrimEnd('\')+'\';$legacyTmpOwner=[IO.Path]::GetFullPath((Join-Path $repo 'Tmp/BlueprintTextPrototype')).TrimEnd('\')+'\';$template=Join-Path $PSScriptRoot 'ProjectTemplate';$catalog=Join-Path $PSScriptRoot '.cache/catalog.json'
function Owned([string]$path,[string]$name){if([string]::IsNullOrWhiteSpace($path)){throw "$name is required."};$full=[IO.Path]::GetFullPath($path);if(-not($full.StartsWith($tmpOwner,[StringComparison]::OrdinalIgnoreCase)-or$full.StartsWith($legacyTmpOwner,[StringComparison]::OrdinalIgnoreCase))){throw "$name must be below $tmpOwner"};$full}
function Project([string]$root){$full=Owned $root 'ProjectRoot';$uproject=Join-Path $full 'BlueprintText.uproject';if(-not(Test-Path $uproject -PathType Leaf)){$uproject=Join-Path $full 'BlueprintTextPrototype.uproject'};if(-not(Test-Path $uproject -PathType Leaf)){throw "Prepared project is missing: $uproject"};[pscustomobject]@{root=$full;uproject=$uproject}}
Import-Module (Join-Path $PSScriptRoot '../unreal-coordination.psm1') -Force
if($Action -notin @('Help','SearchCatalog') -and -not $env:CODEX_UNREAL_QUEUE_LEASE){
    Invoke-UnrealQueueEntry -ScriptPath $PSCommandPath -Parameters $PSBoundParameters -Operation "Blueprint text: $Action";return
}
function Closed(){if(@(Get-Process UnrealEditor,UnrealEditor-Cmd -ErrorAction SilentlyContinue).Count){throw 'An uncoordinated editor started during the reserved engine operation.'}}
function Engine($project,[string]$name,[string[]]$arguments,[string]$evidence){Closed;$editor=Join-Path $EngineRoot 'Engine/Binaries/Win64/UnrealEditor-Cmd.exe';if(-not(Test-Path $editor)){throw "UnrealEditor-Cmd.exe is missing: $editor"};[void](New-Item -ItemType Directory -Path $evidence -Force);$log=Join-Path $evidence ($name+'.log');$unreal=Join-Path $evidence ($name+'-unreal.log');&$editor @($project.uproject,'-run=BlueprintText','-unattended','-nop4','-nosplash','-nullrhi') $arguments ('-abslog='+$unreal) *> $log;if($LASTEXITCODE-ne0){$errors=@(Select-String -Path $unreal -Pattern 'LogTemp: Error:'|Select-Object -Last 3|ForEach-Object{$_.Line-replace'^.*LogTemp: Error: ',''});throw "$name failed ($LASTEXITCODE): $($errors-join' | ') Evidence: $evidence"};$log}
if($Action-eq'Help'){Get-Help $PSCommandPath -Detailed;return}
if($Action-eq'Prepare'){$root=Owned $OutputRoot 'OutputRoot';if(Test-Path $root){throw "Prepare requires a new OutputRoot: $root"};[void](New-Item -ItemType Directory -Path $root);$project=Join-Path $root 'Project';Copy-Item $template -Destination $project -Recurse;$build=Join-Path $EngineRoot 'Engine/Build/BatchFiles/Build.bat';$log=Join-Path $root 'build.log';&$build 'BlueprintTextEditor' Win64 Development (Join-Path $project 'BlueprintText.uproject') -WaitMutex -NoHotReloadFromIDE *> $log;if($LASTEXITCODE-ne0){throw "Prepare build failed ($LASTEXITCODE): $log"};[pscustomobject][ordered]@{status='prepared';projectRoot=$project;buildLog=$log;contentRoot=(Join-Path $project 'Content')};return}
if($Action-eq'SearchCatalog'){if([string]::IsNullOrWhiteSpace($Query)){throw 'SearchCatalog requires Query.'};if(-not(Test-Path $catalog)){throw "Local catalog is missing; run RefreshCatalog once: $catalog"};$data=Get-Content $catalog -Raw|ConvertFrom-Json;$terms=$Query.Split(' ',[StringSplitOptions]::RemoveEmptyEntries);$matches=@($data.entries|Where-Object{$hay=([string]$_.identity+' '+[string]$_.title+' '+[string]$_.keywords).ToLowerInvariant();$ok=$true;foreach($term in $terms){$ok=$ok-and$hay.Contains($term.ToLowerInvariant())};$ok}|Select-Object -First $Limit);if($Format-eq'native'){$found=@($matches|Select-Object identity,kind,title,keywords,pins,nativeInsertion)}else{$found=@($matches|Select-Object identity,kind,title,keywords,pins,compactTemplate)};[pscustomobject][ordered]@{status='catalog-results';query=$Query;format=$Format;returned=$found.Count;engineVersion=$data.engineVersion;coverage=$data.coverage;refresh='Invoke-BlueprintText.ps1 -Action RefreshCatalog -ProjectRoot <prepared project> -Force';entries=$found};return}
$p=Project $ProjectRoot;$evidence=Join-Path (Split-Path $p.root -Parent) ('evidence-'+$Action.ToLowerInvariant()+'-'+[DateTime]::UtcNow.ToString('yyyyMMdd-HHmmss')+'-'+[Guid]::NewGuid().ToString('N').Substring(0,6))
if($Action-eq'RefreshCatalog'){if((Test-Path $catalog) -and -not $Force){throw "RefreshCatalog refuses to overwrite the existing cache without -Force: $catalog"};[void](New-Item -ItemType Directory -Path (Split-Path $catalog -Parent) -Force);$temporary=Join-Path $evidence 'catalog.json';$log=Engine $p 'catalog' @('-Mode=Catalog',('-Output='+$temporary)) $evidence;Copy-Item $temporary -Destination $catalog -Force;[pscustomobject][ordered]@{status='catalog-refreshed';catalog=$catalog;log=$log;context=(Get-Content $catalog -Raw|ConvertFrom-Json).coverage};return}
if($Action-eq'Import'){$input=(Resolve-Path $InputPath).Path;$log=Engine $p 'import' @('-Mode=Import',('-Input='+$input)) $evidence;[pscustomobject][ordered]@{status='imported';projectRoot=$p.root;input=$input;contentRoot=(Join-Path $p.root 'Content');log=$log};return}
if($Action-eq'Export'){if([string]::IsNullOrWhiteSpace($Asset)-or$Asset-notmatch'^/Game/[A-Za-z0-9_/]+$'){throw 'Export requires one /Game asset package path.'};$output=Owned $OutputPath 'OutputPath';[void](New-Item -ItemType Directory -Path (Split-Path $output -Parent) -Force);$log=Engine $p 'export' @('-Mode=Export',('-Asset='+$Asset),('-Format='+$Format),('-Output='+$output)) $evidence;[pscustomobject][ordered]@{status='exported';asset=$Asset;format=$Format;output=$output;log=$log};return}
if($Action-eq'SimulateEdit'){if([string]::IsNullOrWhiteSpace($Variable)-or$Variable-notmatch'^[A-Za-z_][A-Za-z0-9_]*$'){throw 'SimulateEdit requires one valid Variable name.'};$log=Engine $p 'simulate-edit' @('-Mode=SimulateEdit',('-Asset='+$Asset),('-Variable='+$Variable)) $evidence;[pscustomobject][ordered]@{status='engine-edit-simulated';asset=$Asset;variable=$Variable;headlessSimulation=$true;log=$log};return}
if($Action-eq'SimulateUnsupportedGraph'){$log=Engine $p 'simulate-unsupported-graph' @('-Mode=SimulateUnsupportedGraph',('-Asset='+$Asset)) $evidence;[pscustomobject][ordered]@{status='unsupported-graph-simulated';asset=$Asset;headlessSimulation=$true;log=$log};return}
if($Action-eq'SimulateNonzeroDefault'){$log=Engine $p 'simulate-nonzero-default' @('-Mode=SimulateNonzeroDefault',('-Asset='+$Asset),('-Variable='+$Variable)) $evidence;[pscustomobject][ordered]@{status='nonzero-default-simulated';asset=$Asset;variable=$Variable;value=12;headlessSimulation=$true;log=$log};return}
if($Action-eq'SimulateConstructionLogic'){$log=Engine $p 'simulate-construction-logic' @('-Mode=SimulateConstructionLogic',('-Asset='+$Asset)) $evidence;[pscustomobject][ordered]@{status='construction-logic-simulated';asset=$Asset;headlessSimulation=$true;log=$log};return}
