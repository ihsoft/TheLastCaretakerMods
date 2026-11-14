[CmdletBinding()]
param([Parameter(Mandatory)][string]$Destination)
$ErrorActionPreference = 'Stop'
$repo = [IO.Path]::GetFullPath((Join-Path $PSScriptRoot '../..'))
Import-Module (Join-Path $repo 'tools/VoyageModPipeline.psm1') -Force
$project = New-VoyageModProjectCopy -ModRoot $PSScriptRoot -Destination $Destination
# The mod consumes two shared native overlays outside the project. Resolve their
# declared source identities before relocating the working copy to arbitrary depth.
$declaration = Join-Path (Split-Path -Parent $project) 'EditorSupport.json'
$support = Get-Content -LiteralPath $declaration -Raw | ConvertFrom-Json
foreach ($plugin in $support.plugins) {
    $source = Join-Path (Join-Path $PSScriptRoot 'Project') $plugin.source
    foreach ($overlay in $plugin.overlays) {
        $overlay.source = (Resolve-Path -LiteralPath (Join-Path (Split-Path -Parent $source) $overlay.source)).Path
    }
}
$support | ConvertTo-Json -Depth 8 | Set-Content -LiteralPath $declaration -Encoding UTF8
$project
