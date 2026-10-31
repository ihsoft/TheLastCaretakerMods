# Internal worker. The public entry point holds the Unreal queue through rollback.
param(
    [Parameter(Mandatory)][string]$Project,
    [Parameter(Mandatory)][string]$Recipe,
    [Parameter(Mandatory)][string]$EngineRoot,
    [Parameter(Mandatory)][string]$GameRoot,
    [Parameter(Mandatory)][string]$OutputRoot
)
$ErrorActionPreference='Stop'
Set-StrictMode -Version Latest
$repository=(Resolve-Path (Join-Path $PSScriptRoot '../..')).Path
$projectPath=(Resolve-Path -LiteralPath $Project).Path
$projectRoot=Split-Path $projectPath -Parent
if([IO.Path]::GetFileName($projectPath) -cne 'Voyage.uproject') { throw 'This profile requires a Voyage.uproject with declared editor support.' }
if(-not (Test-Path -LiteralPath (Join-Path $projectRoot 'EditorSupport.json'))) { throw 'Declare the project native support in EditorSupport.json before updating a VoyageItem.' }
$output=[IO.Path]::GetFullPath($OutputRoot)
$allowed=(Join-Path $repository 'Tmp').TrimEnd('\')+'\'
if(-not $output.StartsWith($allowed,[StringComparison]::OrdinalIgnoreCase) -or (Test-Path -LiteralPath $output)) { throw 'Use a fresh output directory below repository Tmp.' }
New-Item -ItemType Directory -Path $output | Out-Null
$python=Join-Path $env:USERPROFILE '.cache/codex-runtimes/codex-primary-runtime/dependencies/python/python.exe'
$editor=Join-Path $EngineRoot 'Engine/Binaries/Win64/UnrealEditor-Cmd.exe'
$recipeObject=Get-Content -LiteralPath $Recipe -Raw | ConvertFrom-Json
if($recipeObject.profile -cne 'voyage-item-default-generator-v1' -or
   $recipeObject.asset -cnotmatch '^/Game/(?:[A-Za-z0-9_]+/)*[A-Za-z0-9_]+$') { throw 'Unsupported recipe/profile or invalid package.' }
$mapping=& (Join-Path $repository 'tools/Get-VoyageMappings.ps1') -GameRoot $GameRoot
if($mapping.steamBuildId -cne '25982825' -or $mapping.engineVersion -notlike '5.8*') { throw 'Item profile supports reviewed Steam 25982825 / UE 5.8 only; review the profile for this game build.' }
$extraction=& (Join-Path $repository 'tools/Extract-VoyagePackage.ps1') -GameRoot $GameRoot -Source Game -Filter ('Voyage/Content/'+$recipeObject.asset.Substring(6)+'.uasset') -OutputRoot (Join-Path $output 'original')
$manifest=Get-Content -LiteralPath $extraction.manifestPath -Raw | ConvertFrom-Json
if($manifest.source -cne 'Game' -or $manifest.steamBuildId -cne $mapping.steamBuildId -or @($manifest.assets).Count -ne 1) { throw 'Expected one current-game source asset.' }
$original=Join-Path $extraction.outputPath $manifest.assets[0].path
$sourceJson=Join-Path $output 'source.json'
$schema=Join-Path $output 'generator-schema.json'
$specPath=Join-Path $output 'import.json'
Copy-Item -LiteralPath $Recipe -Destination (Join-Path $output 'recipe.json')
& pwsh -NoProfile -File (Join-Path $PSScriptRoot 'Read-Item.ps1') -AssetPath $original -MappingsPath $mapping.mappingsPath -OutputPath $sourceJson -SchemaPath $schema | Out-Null
if($LASTEXITCODE -ne 0) { throw 'Complete original read failed.' }
& $python -B (Join-Path $PSScriptRoot 'item_patch.py') prepare --source $sourceJson --schema $schema --recipe $Recipe --output $specPath | Out-Null
if($LASTEXITCODE -ne 0) { throw "Unsupported source or recipe. Project unchanged. Evidence: $output" }
$spec=Get-Content -LiteralPath $specPath -Raw | ConvertFrom-Json
$records=[Collections.Generic.List[object]]::new()
function Backup-File([string]$Path) {
    $full=[IO.Path]::GetFullPath($Path)
    if(-not $full.StartsWith($projectRoot.TrimEnd('\')+'\',[StringComparison]::OrdinalIgnoreCase)) { throw 'Project path escaped its root.' }
    if(@($records | Where-Object path -eq $full).Count) { return }
    $relative=$full.Substring($projectRoot.Length+1)
    $saved=Join-Path $output ('backup/'+$relative)
    $exists=Test-Path -LiteralPath $full -PathType Leaf
    if($exists) {
        New-Item -ItemType Directory -Path (Split-Path $saved -Parent) -Force | Out-Null
        Copy-Item -LiteralPath $full -Destination $saved
    }
    $records.Add([pscustomobject]@{path=$full;backup=$saved;existed=$exists})
}
function Run-Editor([string]$Stage,[string[]]$Extra) {
    $arguments=@(('"{0}"' -f $projectPath),'-unattended','-nop4','-nosplash','-nullrhi','-ddc=NoZenLocalFallback',('-abslog="{0}"' -f (Join-Path $output ($Stage+'.log'))))+$Extra
    $process=Start-Process -FilePath $editor -ArgumentList $arguments -WindowStyle Hidden -Wait -PassThru
    if($process.ExitCode -ne 0) { throw "Editor $Stage failed ($($process.ExitCode)). Evidence: $output" }
}
$packagePaths=@($spec.package,$spec.blueprint)+@($spec.dependencies | ForEach-Object { ([string]$_.path -split '\.')[0] })+@($spec.tables.PSObject.Properties.Name | ForEach-Object { ($_ -split '\.')[0] })
$assetPaths=@()
foreach($package in @($packagePaths | Sort-Object -Unique)) {
    if($package -cnotmatch '^/Game/(?:[A-Za-z0-9_]+/)*[A-Za-z0-9_]+$') { throw "Invalid project package: $package" }
    $path=Join-Path $projectRoot ('Content/'+$package.Substring(6)+'.uasset')
    foreach($extension in @('.uexp','.ubulk','.uptnl')) {
        if(Test-Path -LiteralPath ([IO.Path]::ChangeExtension($path,$extension))) { throw "Unexpected editor package companion: $path" }
    }
    Backup-File $path
    $assetPaths+= $path
}
# Native preparation owns its own transaction. Asset rollback below covers only
# item-owned packages, never another tool's plugin installation or cache.
try {
    $preparedSupport=& (Join-Path $repository 'tools/Prepare-UnrealEditorSupport.ps1') -ProjectFile $projectPath -EngineRoot $EngineRoot -OutputRoot (Join-Path $output 'editor-support') | ConvertFrom-Json
    $loadedModules=@($preparedSupport.providers | ForEach-Object loadedModules)
    if('Voyage' -notin $loadedModules -or 'VoyageItemPatchEditor' -notin $loadedModules) { throw 'Project support must provide Voyage and VoyageItemPatchEditor.' }
} catch { throw "Native support preparation failed. Evidence: $output/editor-support. $($_.Exception.Message)" }
try {
    $command=@('-run=VoyageItemPatchEditor',('-Spec="{0}"' -f $specPath))
    Run-Editor 'preflight' ($command+@('-Preflight',('-Report="{0}"' -f (Join-Path $output 'preflight.json'))))
    foreach($package in @($spec.package,$spec.blueprint)) {
        $path=Join-Path $projectRoot ('Content/'+$package.Substring(6)+'.uasset')
        if(Test-Path -LiteralPath $path) { Remove-Item -LiteralPath $path -Force }
    }
    foreach($stage in @('create','reopen')) {
        $report=Join-Path $output ($stage+'.json')
        $extra=$command+@('-Report="{0}"' -f $report)
        if($stage -eq 'reopen') { $extra+='-Reopen' }
        Run-Editor $stage $extra
        if(-not (Test-Path -LiteralPath $report)) { throw "Missing $stage report" }
    }
    if([IO.File]::ReadAllText((Join-Path $output 'create.json')) -cne [IO.File]::ReadAllText((Join-Path $output 'reopen.json'))) { throw 'Item changed across fresh editor reopen.' }
    foreach($path in $assetPaths) { if(-not (Test-Path -LiteralPath $path)) { throw "Missing project asset: $path" } }
    $cookRoot=Join-Path $output 'cooked'
    Run-Editor 'cook' @('-run=cook','-targetplatform=Windows','-unversioned','-SkipZenStore',('-Package='+$spec.package),'-CookSinglePackageNoRefs',('-OutputDir="{0}"' -f $cookRoot))
    $cooked=Join-Path $cookRoot ('Voyage/Content/'+$spec.package.Substring(6)+'.uasset')
    & (Join-Path $repository 'tools/Test-VoyageItemPatch.ps1') -OriginalAsset $original -CookedAsset $cooked -MappingsPath $mapping.mappingsPath -Recipe $Recipe -OutputRoot (Join-Path $output 'validation') | Out-Null
    # Dependencies are reused; no write to an unrelated existing package is permitted.
    foreach($record in $records | Where-Object { $_.existed -and $_.path -like '*.uasset' }) {
        if($record.path -in @((Join-Path $projectRoot ('Content/'+$spec.package.Substring(6)+'.uasset')),(Join-Path $projectRoot ('Content/'+$spec.blueprint.Substring(6)+'.uasset')))) { continue }
        if([Convert]::ToBase64String([IO.File]::ReadAllBytes($record.path)) -cne [Convert]::ToBase64String([IO.File]::ReadAllBytes($record.backup))) { throw "Existing dependency changed: $($record.path)" }
    }
} catch {
    $failureMessage=$_.Exception.Message
    $rollbackErrors=@()
    foreach($record in $records) {
        try {
            if($record.existed) { Copy-Item -LiteralPath $record.backup -Destination $record.path -Force }
            elseif(Test-Path -LiteralPath $record.path) { Remove-Item -LiteralPath $record.path -Force }
        } catch { $rollbackErrors+= "$($record.path): $($_.Exception.Message)" }
    }
    [IO.File]::WriteAllText((Join-Path $output 'failure.json'),(@{status='failed';projectRestored=($rollbackErrors.Count -eq 0);error=$failureMessage;rollbackErrors=$rollbackErrors} | ConvertTo-Json))
    if($rollbackErrors.Count) { throw "Update failed: $failureMessage. Rollback incomplete; preserve backups and inspect $output/failure.json" }
    throw "Update failed; project files restored: $failureMessage. Evidence: $output"
}
$result=[ordered]@{status='passed';profile=$recipeObject.profile;project=$projectPath;asset=$spec.package;allowedChanges=@($recipeObject.patches | ForEach-Object property);assetCount=$assetPaths.Count;validation=(Join-Path $output 'validation/summary.json');cookedAsset=$cooked;sourceAsset=$original;mappingsPath=$mapping.mappingsPath;recipe=(Join-Path $output 'recipe.json');evidencePath=$output}
[IO.File]::WriteAllText((Join-Path $output 'result.json'),($result | ConvertTo-Json -Depth 5))
foreach($record in $records | Where-Object existed) { Remove-Item -LiteralPath $record.backup }
Write-Output ($result | ConvertTo-Json -Depth 5 -Compress)
