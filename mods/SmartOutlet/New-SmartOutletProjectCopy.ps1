# Create a source-only working project for build, tests or interactive authoring.
[CmdletBinding()]
param([Parameter(Mandatory=$true)][string]$Destination)
$ErrorActionPreference = 'Stop'
Set-StrictMode -Version Latest
$repo = (Resolve-Path -LiteralPath (Join-Path $PSScriptRoot '../..')).Path
$owner = [IO.Path]::GetFullPath((Join-Path $repo 'Tmp/SmartOutlet'))
$target = [IO.Path]::GetFullPath($Destination)
if (-not $target.StartsWith($owner + [IO.Path]::DirectorySeparatorChar, [StringComparison]::OrdinalIgnoreCase)) {
    throw 'Destination must be below Tmp/SmartOutlet'
}
if (Test-Path -LiteralPath $target) { throw "Use a fresh project destination: $target" }
for ($ancestor = Split-Path $target -Parent; $ancestor; $ancestor = Split-Path $ancestor -Parent) {
    if ((Test-Path -LiteralPath $ancestor) -and ((Get-Item -LiteralPath $ancestor -Force).Attributes -band [IO.FileAttributes]::ReparsePoint)) {
        throw "Linked destination ancestor is not supported: $ancestor"
    }
}
$source = Join-Path $PSScriptRoot 'Project'
$inputs = @('Voyage.uproject','EditorSupport.json','Content','Config','BlueprintText','EditorSupport','Tests')
$entries = @(Get-ChildItem -LiteralPath $source -Force | Where-Object { $_.Name -notin @('Intermediate','.gitignore') })
foreach ($entry in $entries) {
    if ($entry.Name -notin $inputs) { throw "Unexpected project input; keep generated data in Tmp: $($entry.FullName)" }
    foreach ($item in @($entry) + @(Get-ChildItem -LiteralPath $entry.FullName -Recurse -Force)) {
        if ($item.Attributes -band [IO.FileAttributes]::ReparsePoint) { throw "Linked project input is not supported: $($item.FullName)" }
    }
}
foreach ($name in $inputs) {
    if (-not (Test-Path -LiteralPath (Join-Path $source $name))) { throw "Missing maintained project input: $name" }
}
[IO.Directory]::CreateDirectory($target) | Out-Null
foreach ($entry in $entries) { Copy-Item -LiteralPath $entry.FullName -Destination $target -Recurse }
Join-Path $target 'Voyage.uproject'
