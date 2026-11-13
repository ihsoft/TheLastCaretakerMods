# Create a source-only working copy through the shared pipeline.
[CmdletBinding()]
param([Parameter(Mandatory)][string]$Destination)
$ErrorActionPreference = 'Stop'
Import-Module (Join-Path $PSScriptRoot '../../tools/VoyageModPipeline.psm1')
New-VoyageModProjectCopy -ModRoot $PSScriptRoot -Destination $Destination
