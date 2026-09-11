[CmdletBinding()]
param([Parameter(Mandatory=$true)][string]$OutputRoot, [switch]$SkipBuild)
$ErrorActionPreference='Stop'
Set-StrictMode -Version Latest
$repo=(Resolve-Path (Join-Path $PSScriptRoot '../..')).Path
$output=[IO.Path]::GetFullPath($OutputRoot)
if (-not $output.StartsWith((Join-Path $repo 'artifacts')+[IO.Path]::DirectorySeparatorChar,[StringComparison]::OrdinalIgnoreCase)) { throw 'Output must be below repository artifacts.' }
if(Test-Path -LiteralPath $output){throw 'Use a new output identity.'}
[IO.Directory]::CreateDirectory($output)|Out-Null
$fp=& "$repo/tools/Get-VoyageBuildFingerprint.ps1"|ConvertFrom-Json
if($fp.steam.buildId -ne '25191271' -or $fp.executable.sha256 -ne '747DC2553F7E68D8EA7ED0B2E0CAC6D08943EA3F50DD6ED822E9293E0B45F58B'){throw 'Revalidate current game before generation.'}
$project=Join-Path $PSScriptRoot 'Voyage.uproject'
$engine='K:\Epic Games\UE_5.8\Engine'
$editor=Join-Path $engine 'Binaries/Win64/UnrealEditor-Cmd.exe'
function Invoke-Stage([string]$Name,[string]$Executable,[string[]]$Arguments){
    $log=Join-Path $output ($Name+'.log')
    & $Executable @Arguments *> $log
    if($LASTEXITCODE -ne 0){throw "$Name failed ($LASTEXITCODE): $log"}
}
if(-not $SkipBuild){Invoke-Stage 'build' (Join-Path $engine 'Build/BatchFiles/Build.bat') @('VoyageEditor','Win64','Development',$project,'-WaitMutex','-NoHotReloadFromIDE')}
$content=Join-Path $PSScriptRoot 'Content'
if(Test-Path -LiteralPath $content){
    $resolved=(Resolve-Path -LiteralPath $content).Path
    if($resolved -cne [IO.Path]::GetFullPath($content)){throw 'Unexpected generated content location.'}
    Move-Item -LiteralPath $resolved -Destination (Join-Path $output 'previous-content')
}
[Environment]::SetEnvironmentVariable('UE-LocalDataCachePath',(Join-Path $PSScriptRoot '.ddc'),'Process')
Invoke-Stage 'generate' $editor @($project,'-run=GenerateMooring','-unattended','-nop4','-nosplash','-nullrhi',('-abslog='+(Join-Path $output 'generate-unreal.log')))
# Tagged properties keep the minimal editor mirror's property indices out of
# the runtime CDO contract; stock packages retain their original serialization.
Invoke-Stage 'cook' $editor @($project,'-run=cook','-targetplatform=Windows','-SkipZenStore','-CookSinglePackageNoRefs','-Package=/Game/Blueprints/Cables/BP_ModuleCable_Mooring','-unattended','-nop4','-nosplash','-nullrhi',('-abslog='+(Join-Path $output 'cook-unreal.log')))
$relative='Voyage/Content/Blueprints/Cables/BP_ModuleCable_Mooring'
$cooked=Join-Path $PSScriptRoot ('Saved/Cooked/Windows/'+$relative)
$result=[ordered]@{status='cooked';gameBuild=$fp.steam.buildId;gameHash=$fp.executable.sha256;graphAsset=$cooked+'.uasset';graphPayload=$cooked+'.uexp';runtimeValidated=$false}
foreach($p in @($result.graphAsset,$result.graphPayload)){if(-not(Test-Path -LiteralPath $p)){throw "Missing cooked file: $p"}}
[IO.File]::WriteAllText((Join-Path $output 'graph-manifest.json'),($result|ConvertTo-Json))
[pscustomobject]@{status='cooked';manifestPath=(Join-Path $output 'graph-manifest.json')}
