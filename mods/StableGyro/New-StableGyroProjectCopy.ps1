[CmdletBinding()]
param([Parameter(Mandatory)][string]$Destination)
$ErrorActionPreference = 'Stop'
$repo = [IO.Path]::GetFullPath((Join-Path $PSScriptRoot '../..'))
Import-Module (Join-Path $repo 'tools/VoyageModPipeline.psm1') -Force
New-VoyageModProjectCopy -ModRoot $PSScriptRoot -Destination $Destination
