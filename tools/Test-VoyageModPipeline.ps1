# Isolated filesystem and failure regressions; never launches Unreal or touches the installed game.
[CmdletBinding()]
param()
$ErrorActionPreference='Stop'
Set-StrictMode -Version Latest
$repo=(Resolve-Path (Join-Path $PSScriptRoot '..')).Path
$fixture=Join-Path $repo ('Tmp/tests/mod-pipeline-' + [guid]::NewGuid().ToString('N'))
foreach ($relative in @('tools/BlueprintText','mods/Example/Project/Content','mods/Railgun','Tmp/Example')) {
    [IO.Directory]::CreateDirectory((Join-Path $fixture $relative)) | Out-Null
}
Copy-Item -LiteralPath (Join-Path $PSScriptRoot 'VoyageModPipeline.psm1') -Destination (Join-Path $fixture 'tools')
Import-Module (Join-Path $fixture 'tools/VoyageModPipeline.psm1') -Force
$mod=Join-Path $fixture 'mods/Example'
$project=Join-Path $mod 'Project'
'{}' | Set-Content -LiteralPath (Join-Path $project 'Voyage.uproject')
'uncommitted asset' | Set-Content -LiteralPath (Join-Path $project 'Content/Owned.uasset')
[IO.Directory]::CreateDirectory((Join-Path $project 'Intermediate')) | Out-Null
'scratch' | Set-Content -LiteralPath (Join-Path $project 'Intermediate/unused')
@'
param($Action,$ProjectFile,$EngineRoot,$WorkspaceRoot)
if ($env:MOD_PIPELINE_PREPARE_FAIL -eq '1') { throw 'fixture prepare failure' }
'prepared'
'@ | Set-Content -LiteralPath (Join-Path $fixture 'tools/BlueprintText/Invoke-BlueprintWorkspace.ps1')
$script:checks=0
function Check([bool]$Condition,[string]$Name) {
    if (-not $Condition) { throw "Check failed: $Name" }
    $script:checks++
}
function Reject([scriptblock]$Action,[string]$Name) {
    $rejected=$false
    try { & $Action | Out-Null } catch { $rejected=$true }
    Check $rejected $Name
}
$copyWrapper=Join-Path $mod 'New-ExampleProjectCopy.ps1'
Copy-Item -LiteralPath (Join-Path $repo 'mods/ContainerLidLabel/New-ContainerLidLabelProjectCopy.ps1') -Destination $copyWrapper
$wrapped=& $copyWrapper -Destination (Join-Path $fixture 'Tmp/Example/wrapped/Project')
$copiedDescriptor=Join-Path (Split-Path -Parent $wrapped) 'Voyage.uproject'
Check ([IO.File]::ReadAllText($copiedDescriptor) -ceq [IO.File]::ReadAllText((Join-Path $project 'Voyage.uproject'))) 'public copy wrapper preserves source descriptor'
$copy=New-VoyageModProjectCopy -ModRoot $mod -Destination (Join-Path $fixture 'Tmp/Example/nested/edit/Project')
Check (Test-Path -LiteralPath $copy) 'copy descriptor'
Check ((Get-Content -LiteralPath (Join-Path (Split-Path $copy -Parent) 'Content/Owned.uasset') -Raw).Trim() -eq 'uncommitted asset') 'current uncommitted content'
Check (-not (Test-Path -LiteralPath (Join-Path (Split-Path $copy -Parent) 'Intermediate'))) 'exclude generated input'
Reject { New-VoyageModProjectCopy -ModRoot $mod -Destination (Split-Path $copy -Parent) } 'existing target'
Reject { New-VoyageModProjectCopy -ModRoot $mod -Destination (Join-Path $fixture 'Tmp/ExampleOther/project') } 'sibling prefix'
Reject { New-VoyageModProjectCopy -ModRoot $mod -Destination (Join-Path $fixture 'Tmp/Example/../escape') } 'traversal'
Reject { New-VoyageModProjectCopy -ModRoot $mod -Destination (Join-Path $fixture 'Tmp/Example') } 'owner root'
'unexpected' | Set-Content -LiteralPath (Join-Path $project 'Saved')
Reject { New-VoyageModProjectCopy -ModRoot $mod -Destination (Join-Path $fixture 'Tmp/Example/rejected') } 'unexpected source'
Check (-not (Test-Path -LiteralPath (Join-Path $fixture 'Tmp/Example/rejected'))) 'reject before copy'
Remove-Item -LiteralPath (Join-Path $project 'Saved')
Rename-Item -LiteralPath (Join-Path $project 'Voyage.uproject') -NewName 'missing'
Reject { New-VoyageModProjectCopy -ModRoot $mod -Destination (Join-Path $fixture 'Tmp/Example/missing') } 'missing descriptor'
Rename-Item -LiteralPath (Join-Path $project 'missing') -NewName 'Voyage.uproject'
$link=Join-Path $fixture 'Tmp/Example/redirect'
New-Item -ItemType Junction -Path $link -Target (Join-Path $fixture 'mods') | Out-Null
Reject { New-VoyageModProjectCopy -ModRoot $mod -Destination (Join-Path $link 'project') } 'linked output ancestor'
$sourceLink=Join-Path $project 'Content/redirect'
New-Item -ItemType Junction -Path $sourceLink -Target (Join-Path $fixture 'Tmp/Example/nested') | Out-Null
Reject { New-VoyageModProjectCopy -ModRoot $mod -Destination (Join-Path $fixture 'Tmp/Example/linked-source') } 'linked input'
# Delete only the verified junction itself, never traverse its target.
if ((Get-Item -LiteralPath $sourceLink).Attributes -band [IO.FileAttributes]::ReparsePoint) { [IO.Directory]::Delete($sourceLink) }
$run=New-VoyageModRun -ModRoot $mod -OutputRoot (Join-Path $fixture 'Tmp/Example/runs/deep')
Push-Location $fixture
try {
    $relativeRun=New-VoyageModRun -ModRoot $mod -OutputRoot 'Tmp/Example/relative'
    Check ($relativeRun -eq (Join-Path $fixture 'Tmp/Example/relative')) 'PowerShell-relative paths'
} finally { Pop-Location }
Reject { New-VoyageModRun -ModRoot $mod -OutputRoot $run } 'run refuses reuse'
Reject { Remove-VoyageModScratch -ModRoot $mod -RunRoot $run -Path $mod } 'cleanup outside run'
Reject { Remove-VoyageModScratch -ModRoot $mod -RunRoot $run -Path $run } 'cleanup evidence root'
$scratch=Join-Path $run 'Project'; [IO.Directory]::CreateDirectory($scratch) | Out-Null
Remove-VoyageModScratch -ModRoot $mod -RunRoot $run -Path $scratch
Check (-not (Test-Path -LiteralPath $scratch)) 'bounded cleanup'
$result=Invoke-VoyageModEditorTest -ModRoot $mod -EngineRoot 'fixture' -Run {
    param($run)
    '{"status":"passed","boundary":"synthetic"}' | Set-Content -LiteralPath $run.reportPath
}
Check ($result.status -eq 'passed' -and (Test-Path -LiteralPath $result.reportPath)) 'test result'
Check (-not (Test-Path -LiteralPath (Join-Path $result.evidence 'Project'))) 'successful test removes project'
foreach ($case in @('missing','failed','incomplete','throw')) {
    $failedRun=Join-Path $fixture ('Tmp/Example/failure-' + $case)
    Reject { Invoke-VoyageModEditorTest -ModRoot $mod -EngineRoot fixture -OutputRoot $failedRun -ExpectedSuiteCount 2 -Run {
        param($run)
        if ($case -eq 'throw') { throw 'fixture execution failure' }
        if ($case -eq 'failed') { '{"status":"failed"}' | Set-Content -LiteralPath $run.reportPath }
        if ($case -eq 'incomplete') { '{"status":"passed","tests":[{}]}' | Set-Content -LiteralPath $run.reportPath }
    } } "test rejection: $case"
    Check (Test-Path -LiteralPath (Join-Path $failedRun 'Project/Voyage.uproject')) "failure retains project: $case"
    if ($case -in @('failed','incomplete')) {
        Check (Test-Path -LiteralPath (Join-Path $failedRun 'test-report.json')) "reject actual report: $case"
    }
}
$previousPrepareFailure=$env:MOD_PIPELINE_PREPARE_FAIL
try {
    $env:MOD_PIPELINE_PREPARE_FAIL='1'
    $failedPrepare=Join-Path $fixture 'Tmp/Example/prepare-failure'
    Reject { Invoke-VoyageModEditorTest -ModRoot $mod -EngineRoot fixture -OutputRoot $failedPrepare -Run { throw 'Must not run after failed prepare' } } 'dependency preparation failure'
    Check (Test-Path -LiteralPath (Join-Path $failedPrepare 'Project/Voyage.uproject')) 'prepare failure retains project'
} finally { $env:MOD_PIPELINE_PREPARE_FAIL=$previousPrepareFailure }

# Exercise the real Railgun installer and migration against a synthetic container installer.
Copy-Item -LiteralPath (Join-Path $repo 'mods/Railgun/Install-Railgun.ps1') -Destination (Join-Path $fixture 'mods/Railgun')
Copy-Item -LiteralPath (Join-Path $PSScriptRoot 'Merge-VoyageModIni.ps1') -Destination (Join-Path $fixture 'tools')
[IO.Directory]::CreateDirectory((Join-Path $fixture 'tools/VoyageModSettings')) | Out-Null
Copy-Item -LiteralPath (Join-Path $PSScriptRoot 'VoyageModSettings/VoyageModIniMerge.psm1') -Destination (Join-Path $fixture 'tools/VoyageModSettings')
@'
param($ReleaseManifest,$GameRoot,[switch]$AllowDirtySource,[switch]$ValidateOnly,[switch]$KeepRecoveryEvidence)
if (-not $AllowDirtySource) { throw 'fixture requires explicit dirty source acceptance' }
$repo=Split-Path $PSScriptRoot -Parent
$paks=Join-Path $GameRoot 'Voyage/Content/Paks'
if ($ValidateOnly) { return [pscustomobject]@{validated=$true;mod='Railgun';paksDirectory=$paks} }
$evidence=Join-Path $repo ('Tmp/Railgun/installations/' + [guid]::NewGuid().ToString('N'))
[IO.Directory]::CreateDirectory($evidence) | Out-Null
$container=Join-Path $paks 'Railgun.pak'
$previous=[IO.File]::ReadAllText($container)
$record=Join-Path $evidence 'installation.json'
@{target=$container;previous=$previous} | ConvertTo-Json | Set-Content -LiteralPath $record
'new container' | Set-Content -LiteralPath $container
[pscustomobject]@{fileCount=1;paksDirectory=$paks;installManifestPath=$record}
'@ | Set-Content -LiteralPath (Join-Path $fixture 'tools/Install-VoyageRelease.ps1')
@'
param($InstallManifest,[switch]$ValidateOnly)
if ($ValidateOnly) { return [pscustomobject]@{validated=$true} }
$record=Get-Content -LiteralPath $InstallManifest -Raw | ConvertFrom-Json
[IO.File]::WriteAllText([string]$record.target,[string]$record.previous)
'@ | Set-Content -LiteralPath (Join-Path $fixture 'tools/Restore-VoyageReleaseInstallation.ps1')
$game=Join-Path $fixture 'game'
$paks=Join-Path $game 'Voyage/Content/Paks'; [IO.Directory]::CreateDirectory($paks) | Out-Null
$ini=Join-Path $paks 'Railgun.ini'
$container=Join-Path $paks 'Railgun.pak'
'old container' | Set-Content -LiteralPath $container
Add-Type -AssemblyName System.IO.Compression.FileSystem
Add-Type -AssemblyName System.IO.Compression
function New-Candidate([string]$Name,[string]$Template) {
    $directory=Join-Path $fixture $Name; [IO.Directory]::CreateDirectory($directory) | Out-Null
    $archive=Join-Path $directory 'Railgun-V1.zip'
    $zip=[IO.Compression.ZipFile]::Open($archive,[IO.Compression.ZipArchiveMode]::Create)
    try {
        $entry=$zip.CreateEntry('Voyage/Content/Paks/Railgun.ini')
        $writer=New-Object IO.StreamWriter($entry.Open())
        try { $writer.Write($Template) } finally { $writer.Dispose() }
    } finally { $zip.Dispose() }
    $manifest=Join-Path $directory 'release-manifest.json'
    @{mod='Railgun';archive=@{path='Railgun-V1.zip'}} | ConvertTo-Json -Depth 3 | Set-Content -LiteralPath $manifest
    $manifest
}
$candidate=New-Candidate valid "FullChargeEnergyKWh=1`nIdleConsumptionKW=2`n"
$installer=Join-Path $fixture 'mods/Railgun/Install-Railgun.ps1'
# The installer runs against a fake game directory; process state is part of the fixture too.
$script:fixtureGameRunning=$false
function Get-Process {
    [CmdletBinding()]
    param([string[]]$Name)
    if ($fixtureGameRunning) { [pscustomobject]@{ProcessName='Voyage'} }
}
$script:fixtureGameRunning=$true
Reject { & $installer -ReleaseManifest $candidate -GameRoot $game -AllowDirtySource } 'running game blocker'
$script:fixtureGameRunning=$false
Reject { & $installer -ReleaseManifest $candidate -GameRoot $game } 'dirty policy forwarded'
$installed=& $installer -ReleaseManifest $candidate -GameRoot $game -AllowDirtySource
Check ($installed.status -eq 'installed' -and (Get-Content -LiteralPath $ini -Raw).Contains('IdleConsumptionKW=2')) 'INI created from archived candidate'
"# custom`nFullChargeEnergyKJ=1000`nIdleConsumptionKW=7`nOfflineDischargeKW=9`nOpticsMousePercent=2`n" | Set-Content -LiteralPath $ini
$installed=& $installer -ReleaseManifest $candidate -GameRoot $game -AllowDirtySource
$text=Get-Content -LiteralPath $ini -Raw
Check ($text.Contains('FullChargeEnergyKWh=1') -and $text.Contains('IdleConsumptionKW=7') -and $text.Contains('# custom')) 'preserve settings and migrate energy'
Check ($text -notmatch 'FullChargeEnergyKJ|OfflineDischargeKW|OpticsMousePercent') 'retire obsolete keys'
$broken=New-Candidate broken "FullChargeEnergyKWh=invalid`n"
'before failed install' | Set-Content -LiteralPath $container
$previousIni=[IO.File]::ReadAllBytes($ini); $previousContainer=[IO.File]::ReadAllText($container)
Reject { & $installer -ReleaseManifest $broken -GameRoot $game -AllowDirtySource } 'migration failure'
Check ([Convert]::ToBase64String([IO.File]::ReadAllBytes($ini)) -ceq [Convert]::ToBase64String($previousIni)) 'restore exact INI'
Check ([IO.File]::ReadAllText($container) -ceq $previousContainer) 'restore container after settings failure'
'legacy' | Set-Content -LiteralPath (Join-Path $paks 'Railgun.autoload')
Reject { & $installer -ReleaseManifest $candidate -GameRoot $game -AllowDirtySource } 'legacy loader blocker'
[pscustomobject]@{status='passed';checks=$script:checks;evidence=$fixture} | ConvertTo-Json
