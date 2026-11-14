[CmdletBinding()]
param([Parameter(Mandatory = $true)][string]$Destination)
$ErrorActionPreference = 'Stop'
Import-Module (Join-Path $PSScriptRoot '../../tools/VoyageModPipeline.psm1') -Force
New-VoyageModProjectCopy -ModRoot $PSScriptRoot -Destination $Destination
