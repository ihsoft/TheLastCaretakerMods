<# Isolated pure/static library, external class hierarchy, two callers and cook acceptance. #>
[CmdletBinding()]
param([Parameter(Mandatory)][string]$OutputRoot,[string]$EngineRoot='P:\UnrealEngine\UE_5.8',[string]$PythonPath)
$ErrorActionPreference='Stop'
Set-StrictMode -Version Latest
Import-Module (Join-Path $PSScriptRoot '../unreal-coordination.psm1') -Force
if(-not $env:CODEX_UNREAL_QUEUE_LEASE){Invoke-UnrealQueueEntry -ScriptPath $PSCommandPath -Parameters $PSBoundParameters -Operation 'BPT function library acceptance';return}
$owner=[IO.Path]::GetFullPath((Join-Path $PSScriptRoot '../../Tmp/BlueprintText')).TrimEnd('\')+'\'
$output=[IO.Path]::GetFullPath($OutputRoot)
if(-not $output.StartsWith($owner,[StringComparison]::OrdinalIgnoreCase) -or (Test-Path -LiteralPath $output)){throw 'OutputRoot must be a new directory under Tmp/BlueprintText.'}
[void](New-Item -ItemType Directory -Path (Join-Path $output 'Project'))
$project=Join-Path $output 'Project/Libraries.uproject'
'{"FileVersion":3,"EngineAssociation":"5.8","Category":"Tests"}' | Set-Content -LiteralPath $project -Encoding UTF8
if(-not $PythonPath){$PythonPath=Join-Path $env:USERPROFILE '.cache/codex-runtimes/codex-primary-runtime/dependencies/python/python.exe'}
$tool=Join-Path $PSScriptRoot 'Invoke-BlueprintWorkspace.ps1'
$work=Join-Path $output 'workspace'
$assets=@('/Game/BPTTypeLibrary','/Game/BPTLibraryCallerA','/Game/BPTLibraryCallerB')
function Engine([string]$Mode,[string]$Name,[switch]$Changed){
    $log=Join-Path $output ($Name+'-unreal.log')
    $extra=@();if($Changed){$extra+='-Changed'}
    & (Join-Path $EngineRoot 'Engine/Binaries/Win64/UnrealEditor-Cmd.exe') $project '-run=BlueprintAssetText' '-unattended' '-nop4' '-nosplash' '-nullrhi' ('-Mode='+$Mode) ('-abslog='+$log) @extra *> (Join-Path $output ($Name+'.log'))
    if($LASTEXITCODE){throw "$Mode failed ($LASTEXITCODE); see $log"}
}
& $tool -Action Prepare -ProjectFile $project -WorkspaceRoot (Join-Path $output 'prepare') -EngineRoot $EngineRoot -PythonPath $PythonPath
Engine 'FunctionLibraryFixture' 'fixture'
Engine 'FunctionLibraryCheck' 'initial'
& $tool -Action Export -ProjectFile $project -WorkspaceRoot $work -Asset $assets -EngineRoot $EngineRoot -PythonPath $PythonPath
$templates=(& $tool -Action CallTemplates -WorkspaceRoot $work -Asset '/Game/BPTLibraryCallerA' -Operation @('/Game/BPTTypeLibrary.BPTTypeLibrary_C::IsLarge','/Game/BPTTypeLibrary.BPTTypeLibrary_C::IsMedium','/Game/BPTTypeLibrary.BPTTypeLibrary_C::Optional') -EngineRoot $EngineRoot -PythonPath $PythonPath) | ConvertFrom-Json
$rejected=$false
try { & $tool -Action CallTemplates -WorkspaceRoot $work -Asset '/Game/BPTLibraryCallerA' -Operation '/Game/BPTLibraryCallerA.BPTLibraryCallerA_C::Probe' -EngineRoot $EngineRoot -PythonPath $PythonPath *> (Join-Path $output 'non-library-rejection.log') }
catch { $rejected=$true }
if(-not $rejected -or (Get-Content (Join-Path $output 'non-library-rejection.log') -Raw) -notmatch 'supported pure static Blueprint library'){throw 'Expected rejection of non-library Blueprint templates.'}
foreach($stage in @('move','rename')){
    & $PythonPath -B (Join-Path $PSScriptRoot 'function_library_fixture.py') $work $stage $templates.catalog
    if($LASTEXITCODE){throw 'Library fixture edit failed.'}
    & $tool -Action Apply -WorkspaceRoot $work -EngineRoot $EngineRoot -PythonPath $PythonPath
    Engine 'FunctionLibraryCheck' $stage -Changed:($stage -eq 'rename')
}
& $tool -Action Export -ProjectFile $project -WorkspaceRoot (Join-Path $output 'fresh') -Asset $assets -EngineRoot $EngineRoot -PythonPath $PythonPath
& $tool -Action Apply -WorkspaceRoot (Join-Path $output 'fresh') -EngineRoot $EngineRoot -PythonPath $PythonPath
& $tool -Action Preview -WorkspaceRoot $work -EngineRoot $EngineRoot -PythonPath $PythonPath
$copies=Join-Path $output 'copies'
& $tool -Action AddSource -ProjectFile $project -WorkspaceRoot $copies -SourcePath (Join-Path $work 'blueprints/BPTTypeLibrary.json') -Asset '/Game/Copies/Types' -EngineRoot $EngineRoot -PythonPath $PythonPath
& $tool -Action AddSource -WorkspaceRoot $copies -SourcePath (Join-Path $PSScriptRoot 'Library/BFL_FunctionLibrary.json') -Asset '/Game/Copies/Empty' -EngineRoot $EngineRoot -PythonPath $PythonPath
& $tool -Action Apply -WorkspaceRoot $copies -EngineRoot $EngineRoot -PythonPath $PythonPath
& (Join-Path $EngineRoot 'Engine/Binaries/Win64/UnrealEditor-Cmd.exe') $project '-run=cook' '-targetplatform=Windows' '-unattended' '-nop4' '-nullrhi' '-SkipZenStore' '-CookAll' '-NoDefaultMaps' ('-OutputDir='+(Join-Path $output 'cooked')) ('-abslog='+(Join-Path $output 'cook-unreal.log')) *> (Join-Path $output 'cook.log')
if($LASTEXITCODE){throw "Cook failed; see $output/cook.log"}
foreach($asset in $assets){
    $name=$asset.Substring(6)
    $cooked=Join-Path $output ('cooked/Libraries/Content/'+$name+'.uasset')
    $json=Join-Path $output ($name+'.cooked.json')
    & (Join-Path $PSScriptRoot '../Invoke-VoyageBoundedTool.ps1') -Executable (Join-Path $PSScriptRoot '../../.tools/bin/UAssetGUI.exe') -Arguments @('--portable','tojson',$cooked,$json,'5.8') -OutputRoot (Join-Path $output ('read-'+$name)) -MemoryLimitMB 1024 -TimeoutSeconds 60 | Out-Null
    if((Get-Content -LiteralPath $json -Raw) -match 'BlueprintTextCallFunction|/Script/BlueprintText|BPTArgumentOverrides|BPTDeclarationDefaults|BlueprintText.OptionalParameters'){throw 'Editor BPT state leaked into cooked package.'}
}
@{status='passed';changedApplies=2;freshReopen=$true;callers=2;casesPerCaller=6;pureStatic=$true;renameAndSignature=$true;optionalDefaults=$true;addSource=$true;cookedWithoutBpt=$true;evidence=$output} | ConvertTo-Json | Set-Content (Join-Path $output 'result.json')
Get-Content (Join-Path $output 'result.json')
