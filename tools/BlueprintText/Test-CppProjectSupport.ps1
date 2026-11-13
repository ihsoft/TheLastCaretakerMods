<# Checks ready providers through an ordinary C++ build and real BPT write/reopen. #>
[CmdletBinding()]
param([Parameter(Mandatory)][string]$OutputRoot,
      [string]$EngineRoot='P:/UnrealEngine/UE_5.8', [string]$PythonPath)
$ErrorActionPreference='Stop'
Set-StrictMode -Version Latest
Import-Module (Join-Path $PSScriptRoot '../unreal-coordination.psm1') -Force
if(-not $env:CODEX_UNREAL_QUEUE_LEASE){Invoke-UnrealQueueEntry -ScriptPath $PSCommandPath -Parameters $PSBoundParameters -Operation 'BPT C++ consumer compatibility';return}
$owner=[IO.Path]::GetFullPath((Join-Path $PSScriptRoot '../../Tmp/BlueprintText')).TrimEnd('\')+'\'
$output=[IO.Path]::GetFullPath($OutputRoot)
if(-not $output.StartsWith($owner,[StringComparison]::OrdinalIgnoreCase) -or (Test-Path $output)){throw 'Use a fresh directory below Tmp/BlueprintText.'}
$projectRoot=Join-Path $output 'Project'
[void](New-Item -ItemType Directory -Path (Join-Path $projectRoot 'Source/SupportConsumer'))
$project=Join-Path $projectRoot 'SupportConsumer.uproject'
'{"FileVersion":3,"EngineAssociation":"5.8","Modules":[{"Name":"SupportConsumer","Type":"Runtime","LoadingPhase":"Default"}]}' | Set-Content -LiteralPath $project
@'
using UnrealBuildTool;
public class SupportConsumerEditorTarget : TargetRules {
    public SupportConsumerEditorTarget(TargetInfo Target) : base(Target) {
        Type = TargetType.Editor;
        DefaultBuildSettings = BuildSettingsVersion.Latest;
        IncludeOrderVersion = EngineIncludeOrderVersion.Unreal5_8;
        ExtraModuleNames.Add("SupportConsumer");
    }
}
'@ | Set-Content (Join-Path $projectRoot 'Source/SupportConsumerEditor.Target.cs')
@'
using UnrealBuildTool;
public class SupportConsumer : ModuleRules {
    public SupportConsumer(ReadOnlyTargetRules Target) : base(Target) {
        PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;
        PublicDependencyModuleNames.AddRange(new[] { "Core", "CoreUObject", "Engine" });
    }
}
'@ | Set-Content (Join-Path $projectRoot 'Source/SupportConsumer/SupportConsumer.Build.cs')
@'
#include "Modules/ModuleManager.h"
IMPLEMENT_PRIMARY_GAME_MODULE(FDefaultGameModuleImpl, SupportConsumer, "SupportConsumer");
'@ | Set-Content (Join-Path $projectRoot 'Source/SupportConsumer/SupportConsumer.cpp')
if(-not $PythonPath){$PythonPath=Join-Path $env:USERPROFILE '.cache/codex-runtimes/codex-primary-runtime/dependencies/python/python.exe'}
function BuildConsumer([string]$Name,[switch]$StaleReceipt){
    $extra=@()
    if($StaleReceipt){$extra+= '-DisablePlugin=BlueprintTextWorkspace'}
    & (Join-Path $EngineRoot 'Engine/Build/BatchFiles/Build.bat') SupportConsumerEditor Win64 Development ('-Project='+$project) '-WaitMutex' '-NoHotReloadFromIDE' @extra *> (Join-Path $output ($Name+'.log'))
    if($LASTEXITCODE -ne 0){throw "C++ build failed; see $output/$Name.log"}
}
function RunEngine([string]$Mode,[string]$Name){
    & (Join-Path $EngineRoot 'Engine/Binaries/Win64/UnrealEditor-Cmd.exe') $project '-run=BlueprintAssetText' ('-Mode='+$Mode) '-unattended' '-nop4' '-nosplash' '-nullrhi' ('-abslog='+(Join-Path $output ($Name+'-unreal.log'))) *> (Join-Path $output ($Name+'.log'))
    if($LASTEXITCODE -ne 0){throw "$Mode failed; see $output/$Name.log"}
}
BuildConsumer 'build-before'
$support=Join-Path $PSScriptRoot '../Prepare-UnrealEditorSupport.ps1'
& $support -ProjectFile $project -EngineRoot $EngineRoot -PluginSource (Join-Path $PSScriptRoot '../UnrealEditorSupport/tests/Fixture/SupportFixture.uplugin') -OutputRoot (Join-Path $output 'native-support')
$tool=Join-Path $PSScriptRoot 'Invoke-BlueprintWorkspace.ps1'
& $tool -Action Prepare -ProjectFile $project -WorkspaceRoot (Join-Path $output 'prepare') -EngineRoot $EngineRoot -PythonPath $PythonPath
RunEngine 'ComponentDefaultsFixture' 'fixture'
$work=Join-Path $output 'workspace'
& $tool -Action Export -ProjectFile $project -WorkspaceRoot $work -Asset @('/Game/BPTDefaultsOwner','/Game/BPTDefaultsHelper') -EngineRoot $EngineRoot -PythonPath $PythonPath
# Reproduce the abandoned workaround's persisted receipt, then recover using
# public Prepare and the original build command. No receipt edits or launch overrides.
BuildConsumer 'build-stale-receipt' -StaleReceipt
& $tool -Action Prepare -ProjectFile $project -WorkspaceRoot (Join-Path $output 'prepare-recovery') -EngineRoot $EngineRoot -PythonPath $PythonPath
Add-Content (Join-Path $projectRoot 'Source/SupportConsumer/SupportConsumer.cpp') '// Recompile the consumer with both ready providers installed.'
BuildConsumer 'build-after'
# Reuse after UBT must still recognize a managed binary provider, not refresh it as source.
$package=(Get-Content (Join-Path $output 'native-support/result.json') -Raw | ConvertFrom-Json).package
& $support -ProjectFile $project -EngineRoot $EngineRoot -PluginPackage $package -OutputRoot (Join-Path $output 'reuse-after-build')
$repeat=Get-Content (Join-Path $output 'reuse-after-build/result.json') -Raw | ConvertFrom-Json
if($repeat.status -ne 'unchanged'){throw 'UBT changed managed provider files.'}
foreach($stage in @('edit','restore')){
@'
import json, sys
from pathlib import Path
w=Path(sys.argv[1]); stage=sys.argv[2]
m=json.loads((w/'workspace.json').read_text(encoding='utf-8-sig'))
p=w/m['assets'][0]['source']; d=json.loads(p.read_text())
props=next(g for g in d['graphs'] if g['name']=='EventGraph')['body']['nodes'][0]['properties']
saved=w/'coordinate-test.json'
if stage=='edit':
    saved.write_text(json.dumps({'present':'NodePosX' in props,'value':props.get('NodePosX')}))
    props['NodePosX']=str(int(props.get('NodePosX','0'))+16)
else:
    old=json.loads(saved.read_text())
    if old['present']: props['NodePosX']=old['value']
    else: props.pop('NodePosX',None)
p.write_text(json.dumps(d,indent=2)+'\n',encoding='utf-8')
'@ | & $PythonPath -B - $work $stage
    if($LASTEXITCODE -ne 0){throw 'Cannot edit test coordinate'}
    & $tool -Action Apply -WorkspaceRoot $work -EngineRoot $EngineRoot -PythonPath $PythonPath
    RunEngine 'ComponentDefaultsCheck' ('check-'+$stage)
}
BuildConsumer 'build-repeat'
RunEngine 'ComponentDefaultsCheck' 'check-after-repeat-build'
& (Join-Path $EngineRoot 'Engine/Binaries/Win64/UnrealEditor-Cmd.exe') $project '-run=cook' '-targetplatform=Windows' '-unversioned' '-unattended' '-nop4' '-nullrhi' '-SkipZenStore' '-Package=/Game/BPTDefaultsOwner' '-CookSinglePackageNoRefs' ('-OutputDir='+(Join-Path $output 'cooked')) ('-abslog='+(Join-Path $output 'cook-unreal.log')) *> (Join-Path $output 'cook.log')
if($LASTEXITCODE -ne 0){throw "Cook failed; see $output/cook.log"}
$cooked=Join-Path $output 'cooked/SupportConsumer/Content/BPTDefaultsOwner.uasset'
if(-not (Test-Path $cooked)){throw 'Missing cooked Blueprint'}
@{status='passed';ordinaryCppBuilds=3;staleReceiptRecovery=$true;changedApplies=2;selectedAssets=2;supportReuse='unchanged';cooked=$cooked;project=$project;evidence=$output} | ConvertTo-Json | Set-Content (Join-Path $output 'result.json')
Get-Content (Join-Path $output 'result.json')
