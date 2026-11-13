$ErrorActionPreference = 'Stop'
Import-Module (Join-Path $PSScriptRoot 'SourceUpdate.psm1') -Force
$mod = Split-Path -Parent $PSScriptRoot
$repo = [IO.Path]::GetFullPath((Join-Path $mod '../..'))
Import-Module (Join-Path $repo 'tools/VoyageModPipeline.psm1') -Force
$run = New-VoyageModRun -ModRoot $mod -Kind test-source-update
$source = Join-Path $run 'source'
$updated = Join-Path $run 'updated'
$null = New-Item -ItemType Directory -Path $source,$updated
foreach ($path in @('asset','state')) {
    [IO.File]::WriteAllText((Join-Path $source $path),'before')
    [IO.File]::WriteAllText((Join-Path $updated $path),'after')
}
function Reject([scriptblock]$Action) { $failed=$false; try { & $Action } catch { $failed=$true }; if (-not $failed) { throw 'Expected rejection.' } }
$snapshot=Get-StableGyroSourceSnapshot -ModRoot $source -Paths @('asset','state')
[IO.File]::WriteAllText((Join-Path $source 'asset'),'concurrent')
Reject { Save-StableGyroSourceUpdate -ModRoot $source -RunRoot $updated -Paths @('asset','state') -Snapshot $snapshot }
if ([IO.File]::ReadAllText((Join-Path $source 'state')) -cne 'before') { throw 'Concurrent edit caused partial acceptance.' }
[IO.File]::WriteAllText((Join-Path $source 'asset'),'before')
Remove-Item -LiteralPath (Join-Path $updated 'state')
Reject { Save-StableGyroSourceUpdate -ModRoot $source -RunRoot $updated -Paths @('asset','state') -Snapshot $snapshot }
if ([IO.File]::ReadAllText((Join-Path $source 'asset')) -cne 'before') { throw 'Partial update was not rolled back.' }
[IO.File]::WriteAllText((Join-Path $updated 'state'),'after')
# Fail after one replacement and exercise the ordinary exception recovery path.
$module=Get-Module SourceUpdate
& $module {
    $script:originalReplace=(Get-Command Set-StableGyroSourceFile).ScriptBlock
    $script:replacementCount=0
    function script:Set-StableGyroSourceFile([string]$Destination,[string]$BytesPath,[string]$Stage) {
        $script:replacementCount++
        if ($script:replacementCount -eq 2) { throw 'Injected second replacement failure.' }
        & $script:originalReplace $Destination $BytesPath $Stage
    }
}
Reject { Save-StableGyroSourceUpdate -ModRoot $source -RunRoot $updated -Paths @('asset','state') -Snapshot $snapshot }
Import-Module (Join-Path $PSScriptRoot 'SourceUpdate.psm1') -Force
foreach ($path in @('asset','state')) { if ([IO.File]::ReadAllText((Join-Path $source $path)) -cne 'before') { throw 'Exception did not recover the whole pair.' } }
Assert-StableGyroSourceReady -ModRoot $source
Save-StableGyroSourceUpdate -ModRoot $source -RunRoot $updated -Paths @('asset','state') -Snapshot $snapshot
foreach ($path in @('asset','state')) { if ([IO.File]::ReadAllText((Join-Path $source $path)) -cne 'after') { throw 'Pair was not accepted.' } }
# A present empty source is distinct from a previously absent file.
[IO.File]::WriteAllBytes((Join-Path $source 'empty'),[byte[]]@())
$empty=Get-StableGyroSourceSnapshot -ModRoot $source -Paths @('empty','missing')
if ($null -eq $empty['empty'] -or $null -ne $empty['missing']) { throw 'Snapshot confused empty and absent files.' }
foreach ($path in @($source,$updated)) { Remove-VoyageModScratch -ModRoot $mod -RunRoot $run -Path $path }
'Concurrent-source rejection, partial-copy recovery and paired acceptance passed.'
