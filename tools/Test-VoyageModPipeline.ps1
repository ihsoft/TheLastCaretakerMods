# Isolated filesystem and failure regressions; never launches Unreal or touches the installed game.
[CmdletBinding()]
param()
$ErrorActionPreference='Stop'
Set-StrictMode -Version Latest
$repo=(Resolve-Path (Join-Path $PSScriptRoot '..')).Path
$fixture=Join-Path $repo ('Tmp/tests/mod-pipeline-' + [guid]::NewGuid().ToString('N'))
foreach ($relative in @('tools/BlueprintText','mods/Example/Project/Content','Tmp/Example')) {
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

# Relocation has two bases (project and plugin), with internal and external owners.
$supportRoot=Join-Path $project 'EditorSupport'
[IO.Directory]::CreateDirectory($supportRoot) | Out-Null
$supportDeclaration=Join-Path $project 'EditorSupport.json'
$externalPlugin=Join-Path $fixture 'tools/External/External.uplugin'
$internalOverlay=Join-Path $project 'Content'
@{schema=1;plugins=@(
    @{source='EditorSupport/Local.uplugin';overlays=@(
        @{source='../Content';destination='Source/Internal'},
        @{source='../../../../tools/Shared';destination='Source/Shared'})},
    @{source=$externalPlugin;overlays=@(@{source=$internalOverlay;destination='Source/Internal'})}
)} | ConvertTo-Json -Depth 8 | Set-Content -LiteralPath $supportDeclaration
$declarationBefore=[IO.File]::ReadAllText($supportDeclaration)
$relocated=New-VoyageModProjectCopy -ModRoot $mod -Destination (Join-Path $fixture 'Tmp/Example/deep/authoring/Project')
$relocatedRoot=Split-Path -Parent $relocated
$support=Get-Content -LiteralPath (Join-Path $relocatedRoot 'EditorSupport.json') -Raw | ConvertFrom-Json
Check ($support.plugins[0].source -eq (Join-Path $relocatedRoot 'EditorSupport/Local.uplugin')) 'internal plugin follows copy'
Check ($support.plugins[0].overlays[0].source -eq (Join-Path $relocatedRoot 'Content')) 'internal relative overlay follows copy'
Check ($support.plugins[0].overlays[1].source -eq (Join-Path $fixture 'tools/Shared')) 'external relative overlay retains origin'
Check ($support.plugins[1].source -eq $externalPlugin) 'external plugin retains origin'
Check ($support.plugins[1].overlays[0].source -eq (Join-Path $relocatedRoot 'Content')) 'absolute internal overlay follows copy'
Check ([IO.File]::ReadAllText($supportDeclaration) -ceq $declarationBefore) 'maintained dependency declaration untouched'

$marker=Join-Path $fixture 'source.json'
Set-VoyageModAssetSourceBuild -StatePath $marker -SteamBuildId '123'
$markerArgs=@{StatePath=$marker;SteamBuildId='123';Label='fixture mesh';RefreshCommand='Update-Example.ps1'}
Assert-VoyageModAssetSourceBuild @markerArgs
$before=[IO.File]::ReadAllText($marker)
$markerArgs.SteamBuildId='456'
try { Assert-VoyageModAssetSourceBuild @markerArgs; throw 'Expected mismatch' } catch {
    Check ($_.Exception.Message -like '*fixture mesh*123*456*Update-Example.ps1*') 'mismatch identifies asset and manual update'
}
Check ([IO.File]::ReadAllText($marker) -ceq $before) 'build check never refreshes marker'
foreach ($invalid in @('{}','{"schemaVersion":1,"steamBuildId":"invalid"}','{"schemaVersion":2,"steamBuildId":"456"}')) {
    $invalid | Set-Content -LiteralPath $marker
    Reject { Assert-VoyageModAssetSourceBuild @markerArgs } 'invalid marker cannot authorize reuse'
}
$markerArgs.StatePath=Join-Path $fixture 'missing-source.json'
Reject { Assert-VoyageModAssetSourceBuild @markerArgs } 'missing marker cannot authorize reuse'

[pscustomobject]@{status='passed';checks=$script:checks;evidence=$fixture} | ConvertTo-Json
