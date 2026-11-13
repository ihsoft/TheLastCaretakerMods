$ErrorActionPreference = 'Stop'
Set-StrictMode -Version Latest

function Resolve-SourceChild([string]$Root,[string]$Relative) {
    $rootPath = [IO.Path]::GetFullPath($Root).TrimEnd('\','/')
    $path = [IO.Path]::GetFullPath((Join-Path $rootPath $Relative))
    if ([IO.Path]::IsPathRooted($Relative) -or -not $path.StartsWith($rootPath + '\',[StringComparison]::OrdinalIgnoreCase)) { throw "Source path escaped root: $Relative" }
    for ($cursor=$path; $cursor; $cursor=Split-Path -Parent $cursor) {
        if ((Test-Path -LiteralPath $cursor) -and ((Get-Item -LiteralPath $cursor -Force).Attributes -band [IO.FileAttributes]::ReparsePoint)) { throw "Linked source path: $cursor" }
    }
    $path
}
function Write-SourceBytes([string]$Path,[byte[]]$Bytes,[switch]$New) {
    $mode = if ($New) { [IO.FileMode]::CreateNew } else { [IO.FileMode]::Create }
    $stream = [IO.File]::Open($Path,$mode,[IO.FileAccess]::Write,[IO.FileShare]::None)
    try { $stream.Write($Bytes,0,$Bytes.Length); $stream.Flush($true) } finally { $stream.Dispose() }
}
function Get-SourceMarker([string]$ModRoot) { Resolve-SourceChild $ModRoot 'Project/Intermediate/StableGyroSourceUpdate.json' }
function Assert-StableGyroSourceReady {
    param([string]$ModRoot)
    $marker=Get-SourceMarker $ModRoot
    if (Test-Path -LiteralPath $marker) { throw "Unfinished source update. Read $marker and run Restore-StableGyroSourceUpdate.ps1 with its recoveryManifest before copying or updating source." }
}
function Get-StableGyroSourceSnapshot {
    param([string]$ModRoot,[string[]]$Paths)
    Assert-StableGyroSourceReady -ModRoot $ModRoot
    $result = @{}
    foreach ($path in $Paths) {
        $file = Resolve-SourceChild $ModRoot $path
        $result[$path] = if (Test-Path -LiteralPath $file) { ,([IO.File]::ReadAllBytes($file)) } else { $null }
    }
    return $result
}
function Assert-SourceSnapshot([string]$ModRoot,[hashtable]$Snapshot) {
    foreach ($path in $Snapshot.Keys) {
        $file = Resolve-SourceChild $ModRoot $path
        $current = if (Test-Path -LiteralPath $file) { [Convert]::ToBase64String([IO.File]::ReadAllBytes($file)) } else { $null }
        $before = if ($null -ne $Snapshot[$path]) { [Convert]::ToBase64String($Snapshot[$path]) } else { $null }
        if ($current -cne $before) { throw "Concurrent source edit: $file" }
    }
}
function Get-StableGyroContentSnapshot {
    param([string]$ProjectRoot)
    $content=Resolve-SourceChild $ProjectRoot 'Content'
    $paths=@(Get-ChildItem -LiteralPath $content -Recurse -File | ForEach-Object { $_.FullName.Substring($ProjectRoot.TrimEnd('\','/').Length+1) })
    Get-StableGyroSourceSnapshot -ModRoot $ProjectRoot -Paths $paths
}
function Assert-StableGyroItemUpdateScope {
    param([string]$ProjectRoot,[hashtable]$Before,[string[]]$AllowedPaths)
    $after=Get-StableGyroContentSnapshot -ProjectRoot $ProjectRoot
    foreach ($path in @(@($Before.Keys)+@($after.Keys) | Sort-Object -Unique)) {
        if ($path.Replace('\','/') -in $AllowedPaths) { continue }
        if (-not $Before.ContainsKey($path) -or -not $after.ContainsKey($path) -or
            [Convert]::ToBase64String($Before[$path]) -cne [Convert]::ToBase64String($after[$path])) {
            throw "Item update changed dependency outside the accepted pair: $path. Maintained source is unchanged; preserve $ProjectRoot and review the dependency separately."
        }
    }
}
# The old and new bytes remain on disk until all atomic file replacements finish.
function Set-StableGyroSourceFile([string]$Destination,[string]$BytesPath,[string]$Stage) {
    Copy-Item -LiteralPath $BytesPath -Destination $Stage -Force
    if ([IO.File]::Exists($Destination)) { [IO.File]::Replace($Stage,$Destination,($Stage + '.previous')) }
    else { [IO.File]::Move($Stage,$Destination) }
    if ((Get-FileHash -LiteralPath $Destination).Hash -cne (Get-FileHash -LiteralPath $BytesPath).Hash) { throw "Source copy failed: $Destination" }
    if (Test-Path -LiteralPath ($Stage + '.previous')) { Remove-Item -LiteralPath ($Stage + '.previous') }
}
function Complete-SourceTransaction([string]$Recovery,[string]$Status) {
    $temporary=Join-Path $Recovery 'completion.tmp'
    Write-SourceBytes $temporary ([Text.Encoding]::UTF8.GetBytes($Status))
    [IO.File]::Move($temporary,(Join-Path $Recovery 'completed.txt'))
}
function Clear-SourceRecoveryData([string]$Recovery,$Journal) {
    foreach ($entry in $Journal.entries) {
        foreach ($name in @($entry.before,$entry.after)) {
            $file=Resolve-SourceChild $Recovery $name
            if (Test-Path -LiteralPath $file) { Remove-Item -LiteralPath $file }
        }
    }
    foreach ($name in @('write-stage','write-stage.previous','restore-stage','restore-stage.previous','completion.tmp')) {
        $file=Resolve-SourceChild $Recovery $name
        if (Test-Path -LiteralPath $file) { Remove-Item -LiteralPath $file }
    }
    # Keep only the completed operation's identity. Integrity digests and bytes
    # have no consumer once recovery is complete and the active marker is gone.
    $summary=@{schema=1;modRoot=$Journal.modRoot;entries=@()}
    $temporary=Join-Path $Recovery 'summary.tmp'
    Write-SourceBytes $temporary ([Text.Encoding]::UTF8.GetBytes(($summary | ConvertTo-Json)))
    [IO.File]::Replace($temporary,(Join-Path $Recovery 'recovery.json'),(Join-Path $Recovery 'journal.previous'))
    Remove-Item -LiteralPath (Join-Path $Recovery 'journal.previous')
}
function Restore-StableGyroSourceUpdate {
    param([string]$ModRoot,[string]$RecoveryManifest)
    $manifestPath=(Resolve-Path -LiteralPath $RecoveryManifest).Path
    $recovery=Split-Path -Parent $manifestPath
    $journal=Get-Content -LiteralPath $manifestPath -Raw | ConvertFrom-Json
    if ($journal.schema -ne 1 -or $journal.modRoot -ine [IO.Path]::GetFullPath($ModRoot)) { throw 'Recovery manifest belongs to another source root.' }
    $marker=Get-SourceMarker $ModRoot
    if (Test-Path -LiteralPath $marker) {
        $active=Get-Content -LiteralPath $marker -Raw | ConvertFrom-Json
        if ($active.recoveryManifest -ine $manifestPath) { throw 'Another source transaction is active.' }
    }
    $completion=Join-Path $recovery 'completed.txt'
    if (-not (Test-Path -LiteralPath $completion)) {
        if (-not (Test-Path -LiteralPath $marker)) { throw 'Incomplete recovery requires its active source marker.' }
        # Reject unknown current bytes before any recovery write: they may be a
        # later user edit. Repeated recovery can accept either old or new bytes.
        foreach ($entry in $journal.entries) {
            $target=Resolve-SourceChild $ModRoot $entry.path
            $before=Resolve-SourceChild $recovery $entry.before
            $after=Resolve-SourceChild $recovery $entry.after
            if ((Get-FileHash -LiteralPath $after).Hash -cne $entry.afterHash -or
                ($entry.existed -and (Get-FileHash -LiteralPath $before).Hash -cne $entry.beforeHash)) { throw "Damaged recovery bytes: $manifestPath" }
            $current=if (Test-Path -LiteralPath $target) { (Get-FileHash -LiteralPath $target).Hash } else { $null }
            if ($current -cne $entry.beforeHash -and $current -cne $entry.afterHash) { throw "Concurrent source edit prevents recovery: $target. Preserve $recovery" }
        }
        foreach ($entry in $journal.entries) {
            $target=Resolve-SourceChild $ModRoot $entry.path
            if ($entry.existed) { Set-StableGyroSourceFile $target (Resolve-SourceChild $recovery $entry.before) (Join-Path $recovery 'restore-stage') }
            elseif (Test-Path -LiteralPath $target) { Remove-Item -LiteralPath $target }
        }
        Complete-SourceTransaction $recovery 'restored'
    }
    $status=[IO.File]::ReadAllText($completion)
    if ($status -cnotin @('committed','restored')) { throw "Invalid completion record: $completion" }
    if (Test-Path -LiteralPath $marker) { Remove-Item -LiteralPath $marker }
    Clear-SourceRecoveryData $recovery $journal
    [pscustomobject]@{status=$status;recoveryManifest=$manifestPath}
}
function Save-StableGyroSourceUpdate {
    param([string]$ModRoot,[string]$RunRoot,[string[]]$Paths,[hashtable]$Snapshot)
    Assert-StableGyroSourceReady -ModRoot $ModRoot
    Assert-SourceSnapshot $ModRoot $Snapshot
    if (-not $Paths.Count -or @($Paths | Sort-Object -Unique).Count -ne $Paths.Count) { throw 'Expected a nonempty exact acceptance set.' }
    foreach ($path in $Paths) {
        if (-not $Snapshot.ContainsKey($path)) { throw "Acceptance path was not captured: $path" }
        $null=Resolve-SourceChild $ModRoot $path
        if (-not (Test-Path -LiteralPath (Resolve-SourceChild $RunRoot $path) -PathType Leaf)) { throw "Updated source missing: $path" }
    }
    $recovery=Resolve-SourceChild $RunRoot ('source-recovery-' + [guid]::NewGuid().ToString('N'))
    $null=New-Item -ItemType Directory -Path $recovery
    $entries=@()
    foreach ($path in $Paths) {
        $index=$entries.Count
        $before=Join-Path $recovery "$index.before"
        $after=Join-Path $recovery "$index.after"
        $existed=$null -ne $Snapshot[$path]
        if ($existed) { Write-SourceBytes $before $Snapshot[$path] -New }
        Write-SourceBytes $after ([IO.File]::ReadAllBytes((Resolve-SourceChild $RunRoot $path))) -New
        $entries += [ordered]@{path=$path;existed=$existed;before="$index.before";after="$index.after";
            beforeHash=$(if ($existed) { (Get-FileHash -LiteralPath $before).Hash } else { $null });afterHash=(Get-FileHash -LiteralPath $after).Hash}
    }
    $manifestPath=Join-Path $recovery 'recovery.json'
    $journal=@{schema=1;modRoot=[IO.Path]::GetFullPath($ModRoot);entries=$entries}
    Write-SourceBytes $manifestPath ([Text.Encoding]::UTF8.GetBytes(($journal | ConvertTo-Json -Depth 6))) -New
    $marker=Get-SourceMarker $ModRoot
    $null=New-Item -ItemType Directory -Path (Split-Path -Parent $marker) -Force
    $markerStage=Join-Path $recovery 'active-marker.json'
    Write-SourceBytes $markerStage ([Text.Encoding]::UTF8.GetBytes((@{recoveryManifest=$manifestPath} | ConvertTo-Json))) -New
    [IO.File]::Move($markerStage,$marker)
    Write-Host "Source recovery manifest: $manifestPath"
    try {
        Assert-SourceSnapshot $ModRoot $Snapshot
        foreach ($entry in $entries) {
            Set-StableGyroSourceFile (Resolve-SourceChild $ModRoot $entry.path) (Join-Path $recovery $entry.after) (Join-Path $recovery 'write-stage')
        }
        Complete-SourceTransaction $recovery 'committed'
        Remove-Item -LiteralPath $marker
    } catch {
        $failure=$_
        try { Restore-StableGyroSourceUpdate -ModRoot $ModRoot -RecoveryManifest $manifestPath | Out-Null }
        catch { throw "Source update failed: $failure. Recovery incomplete: $_. Preserve $manifestPath" }
        throw $failure
    }
    Clear-SourceRecoveryData $recovery ([pscustomobject]$journal)
}
Export-ModuleMember -Function Get-StableGyroSourceSnapshot,Save-StableGyroSourceUpdate,Restore-StableGyroSourceUpdate,Assert-StableGyroSourceReady,Get-StableGyroContentSnapshot,Assert-StableGyroItemUpdateScope
