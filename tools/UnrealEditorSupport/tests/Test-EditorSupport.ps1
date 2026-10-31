# Engine integration test; every artifact stays in a fresh repository Tmp folder.
param([string]$EngineRoot='P:/UnrealEngine/UE_5.8', [string]$OutputRoot, [string]$PluginPackage)
$ErrorActionPreference='Stop'
$repository=(Resolve-Path (Join-Path $PSScriptRoot '../../..')).Path
if(-not $OutputRoot) { $OutputRoot=Join-Path $repository ('Tmp/UnrealEditorSupport/test-'+[guid]::NewGuid().ToString('N')) }
$output=[IO.Path]::GetFullPath($OutputRoot)
if(-not $output.StartsWith((Join-Path $repository 'Tmp').TrimEnd('\')+'\',[StringComparison]::OrdinalIgnoreCase) -or (Test-Path $output)) { throw 'Use a fresh directory below repository Tmp.' }
New-Item -ItemType Directory -Path (Join-Path $output 'consumer') -Force | Out-Null
$project=Join-Path $output 'consumer/SupportTest.uproject'
'{"FileVersion":3,"EngineAssociation":"5.8","Plugins":[]}' | Set-Content -LiteralPath $project
$tool=Join-Path $repository 'tools/Prepare-UnrealEditorSupport.ps1'
$fixture=Join-Path $PSScriptRoot 'Fixture'
$sourceBefore=@{}
Get-ChildItem -LiteralPath $fixture -Recurse -File | ForEach-Object { $sourceBefore[$_.FullName]=[Convert]::ToBase64String([IO.File]::ReadAllBytes($_.FullName)) }
if($PluginPackage) {
    & $tool -ProjectFile $project -PluginPackage $PluginPackage -EngineRoot $EngineRoot -OutputRoot (Join-Path $output 'build-and-attach')
} else {
    & $tool -ProjectFile $project -PluginSource (Join-Path $PSScriptRoot 'Fixture/SupportFixture.uplugin') -EngineRoot $EngineRoot -OutputRoot (Join-Path $output 'build-and-attach')
}
$result=Get-Content (Join-Path $output 'build-and-attach/result.json') -Raw | ConvertFrom-Json
$sourceAfter=@(Get-ChildItem -LiteralPath $fixture -Recurse -File)
if($sourceAfter.Count -ne $sourceBefore.Count) { throw 'Build changed the source plugin inventory.' }
foreach($file in $sourceAfter) {
    if(-not $sourceBefore.ContainsKey($file.FullName) -or $sourceBefore[$file.FullName] -cne [Convert]::ToBase64String([IO.File]::ReadAllBytes($file.FullName))) { throw 'Build changed owner source bytes.' }
}
& $tool -ProjectFile $project -PluginPackage $result.package -EngineRoot $EngineRoot -OutputRoot (Join-Path $output 'repeat')
$repeat=Get-Content (Join-Path $output 'repeat/result.json') -Raw | ConvertFrom-Json
if($repeat.status -cne 'unchanged' -or $repeat.built) { throw 'Repeat must reuse the existing binary package.' }
$probe=Join-Path $output 'asset-probe.py'
@'
import unreal
factory = unreal.DataAssetFactory()
factory.set_editor_property('data_asset_class', unreal.SupportFixtureAsset.static_class())
asset = unreal.AssetToolsHelpers.get_asset_tools().create_asset('DA_Probe', '/Game/Test', unreal.SupportFixtureAsset, factory)
assert asset.get_editor_property('sample_value') == 42
asset.set_editor_property('sample_value', 73)
assert unreal.EditorAssetLibrary.save_loaded_asset(asset)
'@ | Set-Content -LiteralPath $probe
$worker=Join-Path $output 'engine-test.ps1'
@'
param([string]$Project,[string]$Engine,[string]$Probe,[string]$Root)
$ErrorActionPreference='Stop'
$editor=Join-Path $Engine 'Engine/Binaries/Win64/UnrealEditor-Cmd.exe'
foreach($stage in @('create','reopen','cook')) {
    if($stage -eq 'reopen') {
        "import unreal`nasset=unreal.load_asset('/Game/Test/DA_Probe')`nassert asset.get_editor_property('sample_value')==73`nassert asset.get_class().get_path_name()=='/Script/SupportFixture.SupportFixtureAsset'" | Set-Content -LiteralPath $Probe
    }
    $arguments=@(('"{0}"' -f $Project),'-unattended','-nop4','-nosplash','-nullrhi','-ddc=NoZenLocalFallback',('-abslog="{0}"' -f (Join-Path $Root ($stage+'.log'))))
    if($stage -eq 'cook') {
        $arguments+=@('-run=cook','-targetplatform=Windows','-unversioned','-SkipZenStore','-Package=/Game/Test/DA_Probe','-CookSinglePackageNoRefs',('-OutputDir="{0}"' -f (Join-Path $Root 'cooked')))
    } else {
        $arguments+=@('-EnablePlugins=PythonScriptPlugin,EditorScriptingUtilities','-run=pythonscript',('-script="{0}"' -f $Probe))
    }
    $process=Start-Process -FilePath $editor -ArgumentList $arguments -WindowStyle Hidden -Wait -PassThru
    if($process.ExitCode -ne 0) { throw "Engine $stage failed. See $Root/$stage.log" }
}
'@ | Set-Content -LiteralPath $worker
& (Join-Path $repository 'tools/Invoke-UnrealQueued.ps1') -Operation 'Verify source-free support consumer assets' -FilePath pwsh -ArgumentList @('-NoProfile','-File',$worker,'-Project',$project,'-Engine',$EngineRoot,'-Probe',$probe,'-Root',$output)
$cooked=Join-Path $output 'cooked/SupportTest/Content/Test/DA_Probe.uasset'
if(-not (Test-Path $cooked)) { throw 'Missing cooked fixture asset.' }
$sources=@(Get-ChildItem -LiteralPath (Join-Path $output 'consumer') -File -Recurse | Where-Object { $_.Extension -in @('.cpp','.h','.cs') })
if($sources.Count) { throw 'Consumer contains C++ or build sources.' }
@{status='passed';sourceFiles=0;repeat='unchanged';nativeIdentity='/Script/SupportFixture.SupportFixtureAsset';valueAfterReopen=73;cooked=$cooked;project=$project;package=$result.package} | ConvertTo-Json | Set-Content (Join-Path $output 'result.json')
Get-Content (Join-Path $output 'result.json')
