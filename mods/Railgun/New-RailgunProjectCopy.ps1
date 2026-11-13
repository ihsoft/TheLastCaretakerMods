# Copies maintained inputs only. Unreal runs against this disposable project.
[CmdletBinding()]
param([Parameter(Mandatory)][string]$Destination)
$ErrorActionPreference = 'Stop'
Set-StrictMode -Version Latest
$source = Join-Path $PSScriptRoot 'Project'
$repo = (Resolve-Path (Join-Path $PSScriptRoot '../..')).Path
$owner = [IO.Path]::GetFullPath((Join-Path $repo 'Tmp/Railgun'))
$destinationPath = [IO.Path]::GetFullPath($Destination)
if (-not $destinationPath.StartsWith($owner + [IO.Path]::DirectorySeparatorChar, [StringComparison]::OrdinalIgnoreCase)) {
    throw 'Destination must stay below Tmp/Railgun'
}
if (Test-Path -LiteralPath $destinationPath) { throw 'Use a fresh project destination' }
# Reject redirects before copying either inputs or output into another tree.
foreach ($path in @($source, $destinationPath)) {
    for ($cursor = $path; $cursor; $cursor = Split-Path -Parent $cursor) {
        if ((Test-Path -LiteralPath $cursor) -and ((Get-Item -LiteralPath $cursor -Force).Attributes -band [IO.FileAttributes]::ReparsePoint)) {
            throw "Linked project path is unsupported: $cursor"
        }
    }
}
$inputs = @('Voyage.uproject', 'EditorSupport.json', 'EditorSupport', 'Content', 'Config', 'BlueprintText', 'Tests')
$entries = @(Get-ChildItem -LiteralPath $source -Force)
foreach ($entry in $entries) {
    if ($entry.Name -in @('.gitignore', 'Intermediate')) { continue }
    if ($entry.Name -notin $inputs) { throw "Unexpected project entry; keep generated data in Tmp: $($entry.FullName)" }
    $tree = @($entry) + @(if ($entry.PSIsContainer) { Get-ChildItem -LiteralPath $entry.FullName -Force -Recurse })
    if ($tree | Where-Object { $_.Attributes -band [IO.FileAttributes]::ReparsePoint }) { throw "Linked source tree: $($entry.FullName)" }
}
[IO.Directory]::CreateDirectory($destinationPath) | Out-Null
foreach ($entry in $entries | Where-Object Name -in $inputs) {
    Copy-Item -LiteralPath $entry.FullName -Destination $destinationPath -Recurse -Force
}
Join-Path $destinationPath 'Voyage.uproject'
