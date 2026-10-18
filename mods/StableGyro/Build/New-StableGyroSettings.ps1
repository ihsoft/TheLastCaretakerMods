[CmdletBinding()]
param(
    [Parameter(Mandatory = $true)][string]$SchemaPath,
    [Parameter(Mandatory = $true)][string]$DefaultIniPath,
    [Parameter(Mandatory = $true)][string]$HeaderPath,
    [Parameter(Mandatory = $true)][string]$IniPath
)

$ErrorActionPreference = 'Stop'
$repoRoot = (Resolve-Path -LiteralPath (Join-Path $PSScriptRoot '..\..\..')).Path
& (Join-Path $repoRoot 'tools\New-VoyageModSettings.ps1') `
    -SchemaPath $SchemaPath -DefaultIniPath $DefaultIniPath `
    -HeaderPath $HeaderPath -IniPath $IniPath `
    -GeneratedBy 'Build/New-StableGyroSettings.ps1'
