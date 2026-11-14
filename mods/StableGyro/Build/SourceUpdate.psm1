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
function Get-StableGyroSourceSnapshot {
    param([string]$ModRoot,[string[]]$Paths)
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
# Callers order assets first and their source-state record last.
function Save-StableGyroSourceUpdate {
    param([string]$ModRoot,[string]$RunRoot,[string[]]$Paths,[hashtable]$Snapshot)
    Assert-SourceSnapshot $ModRoot $Snapshot
    if (-not $Paths.Count -or @($Paths | Sort-Object -Unique).Count -ne $Paths.Count) { throw 'Expected a nonempty exact acceptance set.' }
    foreach ($path in $Paths) {
        if (-not $Snapshot.ContainsKey($path)) { throw "Acceptance path was not captured: $path" }
        $null=Resolve-SourceChild $ModRoot $path
        if (-not (Test-Path -LiteralPath (Resolve-SourceChild $RunRoot $path) -PathType Leaf)) { throw "Updated source missing: $path" }
    }
    $backup=Resolve-SourceChild $RunRoot 'source-backup'
    $null=New-Item -ItemType Directory -Path $backup
    $absent=@()
    foreach ($path in $Paths) {
        if ($null -eq $Snapshot[$path]) { $absent += $path; continue }
        $saved=Resolve-SourceChild $backup $path
        $null=New-Item -ItemType Directory -Path (Split-Path -Parent $saved) -Force
        [IO.File]::WriteAllBytes($saved,$Snapshot[$path])
    }
    [IO.File]::WriteAllLines((Join-Path $backup 'absent-files.txt'),[string[]]$absent)
    Write-Host "Source backup: $backup"
    try {
        Assert-SourceSnapshot $ModRoot $Snapshot
        foreach ($path in $Paths) {
            $target=Resolve-SourceChild $ModRoot $path
            $null=New-Item -ItemType Directory -Path (Split-Path -Parent $target) -Force
            Copy-Item -LiteralPath (Resolve-SourceChild $RunRoot $path) -Destination $target -Force
        }
    } catch {
        throw "Source copy failed; files may be partially updated. Restore affected paths from $backup or Git after reviewing current edits. Previously absent paths are listed in absent-files.txt. Preserve $RunRoot. Error: $_"
    }
    # This fresh, unlinked directory was resolved strictly below the owning run.
    Remove-Item -LiteralPath $backup -Recurse
}
Export-ModuleMember -Function Get-StableGyroSourceSnapshot,Save-StableGyroSourceUpdate,Get-StableGyroContentSnapshot,Assert-StableGyroItemUpdateScope
