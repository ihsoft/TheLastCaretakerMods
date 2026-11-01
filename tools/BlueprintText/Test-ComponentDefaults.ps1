<#
.SYNOPSIS
Check inherited and overridden child-actor defaults across repeated BPT batch imports.
Creates an isolated project under Tmp/BlueprintText; launches Unreal through the
shared queue. Uses strict workspace Apply and independent fresh-process value checks.
#>
[CmdletBinding()]
param(
    [Parameter(Mandatory)][string]$OutputRoot,
    [string]$EngineRoot='P:\UnrealEngine\UE_5.8',
    [string]$PythonPath
)
$ErrorActionPreference='Stop'
Set-StrictMode -Version Latest
Import-Module (Join-Path $PSScriptRoot '../unreal-coordination.psm1') -Force
if(-not $env:CODEX_UNREAL_QUEUE_LEASE){Invoke-UnrealQueueEntry -ScriptPath $PSCommandPath -Parameters $PSBoundParameters -Operation 'BPT component defaults regression';return}
$owner=[IO.Path]::GetFullPath((Join-Path $PSScriptRoot '../../Tmp/BlueprintText')).TrimEnd('\')+'\'
$output=[IO.Path]::GetFullPath($OutputRoot)
if(-not $output.StartsWith($owner,[StringComparison]::OrdinalIgnoreCase)){throw 'OutputRoot must stay below Tmp/BlueprintText.'}
if(Test-Path -LiteralPath $output){throw 'OutputRoot must be a new test evidence directory.'}
[void](New-Item -ItemType Directory -Path (Join-Path $output 'Project'))
$project=Join-Path $output 'Project/ComponentDefaults.uproject'
'{"FileVersion":3,"EngineAssociation":"5.8","Category":"Tests"}' | Set-Content -LiteralPath $project -Encoding UTF8
if(-not $PythonPath){$PythonPath=Join-Path $env:USERPROFILE '.cache/codex-runtimes/codex-primary-runtime/dependencies/python/python.exe'}
$tool=Join-Path $PSScriptRoot 'Invoke-BlueprintWorkspace.ps1'
$work=Join-Path $output 'workspace'
function RunEngine([string]$Mode,[string]$Name){
    if(@(Get-Process UnrealEditor,UnrealEditor-Cmd -ErrorAction SilentlyContinue).Count){throw 'An uncoordinated editor started during the reserved test operation.'}
    $log=Join-Path $output ($Name+'-unreal.log')
    & (Join-Path $EngineRoot 'Engine/Binaries/Win64/UnrealEditor-Cmd.exe') $project '-run=BlueprintAssetText' '-unattended' '-nop4' '-nosplash' '-nullrhi' ('-Mode='+$Mode) ('-abslog='+$log) *> (Join-Path $output ($Name+'.log'))
    if($LASTEXITCODE -ne 0){throw "$Mode failed ($LASTEXITCODE); diagnostic: $log"}
}
& $tool -Action Prepare -ProjectFile $project -WorkspaceRoot (Join-Path $output 'prepare') -EngineRoot $EngineRoot -PythonPath $PythonPath
RunEngine 'ComponentDefaultsFixture' 'fixture'
RunEngine 'ComponentDefaultsCheck' 'original-check'
# Consumer first: restoration must wait for all peer CDOs, not depend on input order.
& $tool -Action Export -ProjectFile $project -WorkspaceRoot $work -Asset @('/Game/BPTDefaultsOwner','/Game/BPTDefaultsHelper') -EngineRoot $EngineRoot -PythonPath $PythonPath
@'
import json, sys
from pathlib import Path
work=Path(sys.argv[1])
manifest=json.loads((work/'workspace.json').read_text(encoding='utf-8'))
owner=json.loads((work/manifest['assets'][0]['source']).read_text(encoding='utf-8'))
templates=[o for o in owner['components']['objects'] if o['class'].endswith('.BPTDefaultsHelper_C')]
assert len(templates)==2
inherited=next(o for o in templates if o['name'].startswith('Inherited_'))
overridden=next(o for o in templates if o['name'].startswith('Overridden_'))
assert inherited['properties']=={}, inherited['properties']
assert overridden['properties']['Enabled']=='False'
assert float(overridden['properties']['Rate'])==4.25
assert 'TickGroup=TG_PostUpdateWork' in overridden['properties']['PrimaryActorTick']
'@ | & $PythonPath -B - $work
if($LASTEXITCODE -ne 0){throw 'Fixture does not cover both an empty delta and explicit overrides'}
foreach($iteration in 1..2){
    # Force an actual rebuild each time; an unchanged Apply would not exercise import.
    @'
import json, sys
from pathlib import Path
work=Path(sys.argv[1])
manifest=json.loads((work/'workspace.json').read_text(encoding='utf-8'))
for entry in manifest['assets']:
    path=work/entry['source']
    data=json.loads(path.read_text(encoding='utf-8'))
    graph=next(g for g in data['graphs'] if g['name']=='EventGraph')
    graph['body']['nodes'][0]['properties']['NodePosX']=str(100*int(sys.argv[2]))
    path.write_text(json.dumps(data,indent=2)+'\n',encoding='utf-8')
'@ | & $PythonPath -B - $work $iteration
    if($LASTEXITCODE -ne 0){throw 'Cannot edit regression source'}
    & $tool -Action Apply -WorkspaceRoot $work -EngineRoot $EngineRoot -PythonPath $PythonPath
    RunEngine 'ComponentDefaultsCheck' ('check-'+$iteration)
}
& $tool -Action Apply -WorkspaceRoot $work -EngineRoot $EngineRoot -PythonPath $PythonPath
$result=[pscustomobject]@{status='passed';inheritedDefaults=$true;explicitOverrides=$true;nativeTickDefaults=$true;rebuilds=2;freshProcessChecks=3;evidence=$output}
$result | ConvertTo-Json | Set-Content -LiteralPath (Join-Path $output 'result.json') -Encoding UTF8
$result | ConvertTo-Json -Compress
