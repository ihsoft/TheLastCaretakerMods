[CmdletBinding()]
param(
    [Parameter(Mandatory = $true)][string]$SchemaPath,
    [Parameter(Mandatory = $true)][string]$DefaultIniPath,
    [Parameter(Mandatory = $true)][string]$HeaderPath,
    [Parameter(Mandatory = $true)][string]$IniPath,
    [Parameter(Mandatory = $true)][string]$GeneratedBy,
    # Legacy adapter compatibility. The unified generator always emits its
    # common array table, independent of this former layout selector.
    [ValidateSet('Array', 'TArray')][string]$CollectionKind
)

$ErrorActionPreference = 'Stop'
Set-StrictMode -Version Latest
Import-Module (Join-Path $PSScriptRoot 'VoyageModSettings\VoyageModSettings.psm1') -Force
Invoke-NewVoyageModSettings -SchemaPath $SchemaPath -DefaultIniPath $DefaultIniPath `
    -HeaderPath $HeaderPath -IniPath $IniPath -GeneratedBy $GeneratedBy
