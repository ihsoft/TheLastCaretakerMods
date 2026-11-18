# Authoring -> source-only checkout -> shared cook; no mod assets or installed game.
[CmdletBinding()]
param([Parameter(Mandatory)][string]$OutputRoot,
      [string]$EngineRoot='P:/UnrealEngine/UE_5.8', [string]$PythonPath)
$ErrorActionPreference='Stop'
Set-StrictMode -Version Latest
Import-Module (Join-Path $PSScriptRoot 'unreal-coordination.psm1') -Force
if(-not $env:CODEX_UNREAL_QUEUE_LEASE){Invoke-UnrealQueueEntry -ScriptPath $PSCommandPath -Parameters $PSBoundParameters -Operation 'Shared mod cook acceptance';return}
$repo=[IO.Path]::GetFullPath((Join-Path $PSScriptRoot '..'))
$owner=Join-Path $repo 'Tmp/BlueprintText'
$output=[IO.Path]::GetFullPath($OutputRoot)
if(-not $output.StartsWith($owner+'\',[StringComparison]::OrdinalIgnoreCase) -or (Test-Path -LiteralPath $output)){throw 'Use a fresh directory below Tmp/BlueprintText.'}
for($cursor=$output;$cursor;$cursor=Split-Path -Parent $cursor){
    if((Test-Path -LiteralPath $cursor) -and ((Get-Item -LiteralPath $cursor -Force).Attributes -band [IO.FileAttributes]::ReparsePoint)){throw "Linked test path: $cursor"}
}
if(-not $PythonPath){$PythonPath=Join-Path $env:USERPROFILE '.cache/codex-runtimes/codex-primary-runtime/dependencies/python/python.exe'}
$tool=Join-Path $PSScriptRoot 'BlueprintText/Invoke-BlueprintWorkspace.ps1'
$seed=Join-Path $output 'Seed'
[void](New-Item -ItemType Directory -Path $seed)
$project=Join-Path $seed 'Voyage.uproject'
'{"FileVersion":3,"EngineAssociation":"5.8"}' | Set-Content -LiteralPath $project -Encoding UTF8
& $tool -Action Prepare -ProjectFile $project -WorkspaceRoot (Join-Path $output 'authoring') -EngineRoot $EngineRoot -PythonPath $PythonPath > (Join-Path $output 'authoring.json')
foreach($mode in @('ArgumentFixture','ArgumentCheck')){
    & (Join-Path $EngineRoot 'Engine/Binaries/Win64/UnrealEditor-Cmd.exe') $project '-run=BlueprintAssetText' ('-Mode='+$mode) '-Expected=7' '-unattended' '-nop4' '-nosplash' '-nullrhi' ('-abslog='+(Join-Path $output ($mode+'.log'))) *> (Join-Path $output ($mode+'-console.log'))
    if($LASTEXITCODE -ne 0){throw "$mode failed; see $output"}
}
# Isolate repository ownership checks without introducing a permanent fixture mod.
# Adapters forward engine work to the real tools; only workspace paths are relocated.
$fixture=Join-Path $output 'Repository'
$fixtureTools=Join-Path $fixture 'tools'
$mod=Join-Path $fixture 'mods/Example'
[void](New-Item -ItemType Directory -Path (Join-Path $fixtureTools 'BlueprintText'))
[void](New-Item -ItemType Directory -Path (Join-Path $mod 'Project'))
Copy-Item -LiteralPath (Join-Path $PSScriptRoot 'VoyageModPipeline.psm1') -Destination $fixtureTools
Copy-Item -LiteralPath (Join-Path $PSScriptRoot 'UnrealCookSupport') -Destination $fixtureTools -Recurse
foreach($name in @('Voyage.uproject','Content')){Copy-Item -LiteralPath (Join-Path $seed $name) -Destination (Join-Path $mod 'Project') -Recurse}
$proxies=@{
    'BlueprintText/Invoke-BlueprintWorkspace.ps1'=@'
param($Action,$ProjectFile,$EngineRoot,$WorkspaceRoot)
$PSBoundParameters.WorkspaceRoot=Join-Path '{{OUTPUT}}' ('cook-prepare-'+[guid]::NewGuid().ToString('N'))
& '{{TOOLS}}/BlueprintText/Invoke-BlueprintWorkspace.ps1' @PSBoundParameters
'@
    'Prepare-UnrealEditorSupport.ps1'=@'
param($ProjectFile,$PluginSource,$EngineRoot,$OutputRoot)
& '{{TOOLS}}/Prepare-UnrealEditorSupport.ps1' @PSBoundParameters
'@
    'Invoke-UnrealQueued.ps1'=@'
param($Operation,$FilePath,$ArgumentList)
& '{{TOOLS}}/Invoke-UnrealQueued.ps1' @PSBoundParameters
'@
}
foreach($name in $proxies.Keys){
    $proxies[$name].Replace('{{TOOLS}}',$PSScriptRoot.Replace("'","''")).Replace('{{OUTPUT}}',$output.Replace("'","''")) | Set-Content -LiteralPath (Join-Path $fixtureTools $name) -Encoding UTF8
}
Import-Module (Join-Path $fixtureTools 'VoyageModPipeline.psm1') -Force
$run=New-VoyageModRun -ModRoot $mod
$copy=New-VoyageModProjectCopy -ModRoot $mod -Destination (Join-Path $run 'Project')
if(Test-Path (Join-Path (Split-Path -Parent $copy) 'Plugins')){throw 'Fresh checkout contains installed plugins.'}
$packages=@('/Game/BPTArgumentCaller','/Game/BPTArgumentCallee')
$result=Invoke-VoyageModCookAssets -ModRoot $mod -ProjectFile $copy -EngineRoot $EngineRoot -OutputRoot $run -Packages $packages -TaggedPackages $packages
$cooked=Join-Path $result.stage 'Voyage/Content/BPTArgumentCaller.uasset'
$readback=Join-Path $output 'cooked-caller.json'
& (Join-Path $PSScriptRoot 'Invoke-VoyageBoundedTool.ps1') -Executable (Join-Path $repo '.tools/bin/UAssetGUI.exe') -Arguments @('--portable','tojson',$cooked,$readback,'5.8') -OutputRoot (Join-Path $output 'readback') -MemoryLimitMB 1024 -TimeoutSeconds 60 | Out-Null
# Check the compiled operation and operands, not an import/name-table entry or graph shape.
@'
import json, sys
d=json.load(open(sys.argv[1],encoding='utf-8-sig'))
probe=next(e for e in d['Exports'] if e['ObjectName']=='Probe')
def walk(v):
    if isinstance(v,dict):
        yield v
        for child in v.values(): yield from walk(child)
    elif isinstance(v,list):
        for child in v: yield from walk(child)
calls=[n for n in walk(probe['ScriptBytecode']) if 'EX_CallMath,' in n.get('$type','')]
assert any(n['StackNode'] < 0 and d['Imports'][-n['StackNode']-1]['ObjectName']=='Add_IntInt'
           and [p.get('Value') for p in n['Parameters']]==[11,1] for n in calls), 'BPT arithmetic call/arguments lost during cook'
assert any(n.get('VirtualFunctionName')=='EchoInt' and [p.get('Value') for p in n.get('Parameters',[])]==[7]
           for n in walk(probe['ScriptBytecode'])), 'BPT declaration default lost during cook'
assert '/Script/BlueprintText' not in json.dumps(d), 'Editor compiler dependency leaked into cooked package'
print('Cooked BPT native call and declaration default preserved.')
'@ | & $PythonPath -B - $readback
if($LASTEXITCODE -ne 0){throw 'Cooked behavior check failed.'}
# An unavailable declared provider must stop the operation before cook/staging.
$badRun=New-VoyageModRun -ModRoot $mod
$badCopy=New-VoyageModProjectCopy -ModRoot $mod -Destination (Join-Path $badRun 'Project')
'{"schema":1,"plugins":[{"source":"EditorSupport/MissingCompiler.uplugin"}]}' | Set-Content -LiteralPath (Join-Path (Split-Path -Parent $badCopy) 'EditorSupport.json')
$rejected=$false
try{Invoke-VoyageModCookAssets -ModRoot $mod -ProjectFile $badCopy -EngineRoot $EngineRoot -OutputRoot $badRun -Packages $packages -TaggedPackages $packages | Out-Null}catch{$rejected=$true;$_ | Out-String | Set-Content (Join-Path $output 'missing-provider.txt')}
if(-not $rejected -or (Test-Path (Join-Path $badRun 'stage')) -or (Test-Path (Join-Path $badRun 'cook-tagged.log'))){throw 'Unavailable dependency must fail before cook and staging.'}
if((Get-Content (Join-Path $badRun 'prepare.log') -Raw) -notmatch 'MissingCompiler'){throw 'Failure did not identify the missing provider.'}
Copy-Item -LiteralPath (Join-Path $run 'cook-tagged.log') -Destination (Join-Path $output 'cook.log')
Copy-Item -LiteralPath (Join-Path $run 'prepare.log') -Destination (Join-Path $output 'cook-prepare.json')
Copy-Item -LiteralPath (Join-Path $badRun 'prepare.log') -Destination (Join-Path $output 'missing-provider.json')
foreach($scratch in @($seed,$fixture)){
    $absolute=[IO.Path]::GetFullPath($scratch)
    if(-not $absolute.StartsWith($output+'\',[StringComparison]::OrdinalIgnoreCase)){throw "Scratch escaped test output: $absolute"}
    if(((Get-Item -LiteralPath $absolute -Force).Attributes -band [IO.FileAttributes]::ReparsePoint) -or
       @(Get-ChildItem -LiteralPath $absolute -Recurse -Force -Attributes ReparsePoint).Count){throw "Linked scratch: $absolute"}
    Remove-Item -LiteralPath $absolute -Recurse -Force
}
@{status='passed';freshCopyCook=$true;compiledCallPreserved=$true;declarationDefaultPreserved=$true;missingProviderRejected=$true;evidence=$output} | ConvertTo-Json | Set-Content (Join-Path $output 'result.json')
Get-Content (Join-Path $output 'result.json')
