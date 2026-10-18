[CmdletBinding()]
param(
    [Parameter(Mandatory = $true)][string]$TemplatePath,
    [Parameter(Mandatory = $true)][string]$OutputPath,
    [string]$CurrentPath,
    [string[]]$RetiredKeys = @(),
    [string[]]$RetiredComments = @(),
    [string]$DefaultsComment = '# Defaults added by a newer mod build.'
)

$ErrorActionPreference = 'Stop'
Import-Module (Join-Path $PSScriptRoot 'VoyageModSettings\VoyageModIniMerge.psm1') -Force
Invoke-MergeVoyageModIni @PSBoundParameters
