[CmdletBinding()]
param(
    [Parameter(Mandatory)][string]$SchemaPath,
    [Parameter(Mandatory)][string]$DefaultIniPath
)
$ErrorActionPreference = 'Stop'
Import-Module (Join-Path $PSScriptRoot 'VoyageModSettings/VoyageModSettings.psm1') -Force
$settings = @(Read-VoyageModSettingsSchema -SchemaPath $SchemaPath -DefaultIniPath $DefaultIniPath)
[pscustomobject]@{
    status = 'passed'
    settingsCount = $settings.Count
    schemaPath = (Resolve-Path -LiteralPath $SchemaPath).Path
    iniPath = (Resolve-Path -LiteralPath $DefaultIniPath).Path
} | ConvertTo-Json -Compress
