[CmdletBinding()]
param(
    [Parameter(Mandatory = $true)][string]$SchemaPath,
    [Parameter(Mandatory = $true)][string]$DefaultIniPath,
    [Parameter(Mandatory = $true)][string]$HeaderPath,
    [Parameter(Mandatory = $true)][string]$IniPath
)

$ErrorActionPreference = 'Stop'
Set-StrictMode -Version Latest
$invariant = [Globalization.CultureInfo]::InvariantCulture

function Format-Number([object]$Value) {
    return ([double]$Value).ToString('0.###############', $invariant)
}

function Escape-CppString([string]$Value) {
    return $Value.Replace('\', '\\').Replace('"', '\"')
}

function Require-Identifier([string]$Value, [string]$Label) {
    if ($Value -notmatch '^[A-Za-z_][A-Za-z0-9_]*$') { throw "Invalid ${Label}: $Value" }
}

function Require-FieldExpression([string]$Value) {
    if ($Value -notmatch '^[A-Za-z_][A-Za-z0-9_]*(::[A-Za-z_][A-Za-z0-9_]*)?$') {
        throw "Invalid field expression: $Value"
    }
}

$resolvedSchema = (Resolve-Path -LiteralPath $SchemaPath).Path
$schema = Get-Content -LiteralPath $resolvedSchema -Raw | ConvertFrom-Json
if ($schema.schemaVersion -ne 1) { throw 'Unsupported Railgun settings schema version.' }
$groups = @($schema.groups)
if ($groups.Count -eq 0) { throw 'Railgun settings schema has no groups.' }
$resolvedDefaultIni = (Resolve-Path -LiteralPath $DefaultIniPath).Path
$defaultIniLines = @([IO.File]::ReadAllLines($resolvedDefaultIni))
$iniValues = New-Object 'System.Collections.Generic.Dictionary[string,string]' ([StringComparer]::Ordinal)
foreach ($line in $defaultIniLines) {
    if ($line -match '^\s*([^#;][^=]*?)\s*=(.*)$') {
        $key = $matches[1].Trim()
        if ($iniValues.ContainsKey($key)) { throw "Duplicate setting key in default INI: $key" }
        $iniValues.Add($key, $matches[2].Trim())
    }
}
if ($iniValues.Count -eq 0) { throw 'Railgun default INI has no settings.' }

$settings = New-Object 'System.Collections.Generic.List[object]'
$ids = New-Object 'System.Collections.Generic.HashSet[string]' ([StringComparer]::Ordinal)
$keys = New-Object 'System.Collections.Generic.HashSet[string]' ([StringComparer]::Ordinal)
$runtimeNames = New-Object 'System.Collections.Generic.HashSet[string]' ([StringComparer]::Ordinal)
$fontObjectIds = New-Object 'System.Collections.Generic.HashSet[string]' ([StringComparer]::Ordinal)

foreach ($group in $groups) {
    Require-Identifier ([string]$group.id).Replace('-', '_') 'group id'
    foreach ($setting in @($group.settings)) {
        $id = [string]$setting.id
        $key = [string]$setting.key
        Require-Identifier $id 'setting id'
        if ($key -notmatch '^[A-Za-z][A-Za-z0-9]*$') { throw "Invalid setting key: $key" }
        if (-not $ids.Add($id)) { throw "Duplicate setting id: $id" }
        if (-not $keys.Add($key)) { throw "Duplicate setting key: $key" }
        if (-not $iniValues.ContainsKey($key)) { throw "Setting is missing from default INI: $key" }
        if ([string]$setting.type -notin @('number', 'text')) { throw "Unsupported setting type for ${id}: $($setting.type)" }
        $resolvedDefault = $iniValues[$key]

        $field = if ($setting.PSObject.Properties.Name -contains 'field') { [string]$setting.field } else { $id }
        Require-FieldExpression $field
        if ($field -notmatch '::') {
            if (-not ($setting.PSObject.Properties.Name -contains 'runtimeName')) { throw "Missing runtimeName for local setting: $id" }
            $runtimeName = [string]$setting.runtimeName
            if (-not $runtimeNames.Add($runtimeName)) { throw "Duplicate runtimeName: $runtimeName" }
        } elseif ($setting.PSObject.Properties.Name -contains 'runtimeName') {
            throw "External field must not declare runtimeName: $id"
        }

        if ([string]$setting.type -eq 'number') {
            foreach ($name in @('minimum', 'maximum')) {
                if (-not ($setting.PSObject.Properties.Name -contains $name)) { throw "Missing $name for numeric setting: $id" }
            }
            $default = 0.0
            if (-not [double]::TryParse($resolvedDefault, [Globalization.NumberStyles]::Float,
                $invariant, [ref]$default)) { throw "Default INI value is not numeric for ${key}: $resolvedDefault" }
            $minimum = [double]$setting.minimum
            $maximum = [double]$setting.maximum
            if ($minimum -gt $maximum -or $default -lt $minimum -or $default -gt $maximum) {
                throw "Numeric default/range contract is invalid: $id"
            }
        } else {
            if ($resolvedDefault.Contains("`r") -or $resolvedDefault.Contains("`n")) {
                throw "Text default cannot contain a newline: $id"
            }
        }

        if ($setting.PSObject.Properties.Name -contains 'fontObjectId') {
            if ([string]$setting.type -ne 'text') { throw "Font path must be text: $id" }
            $fontObjectId = [string]$setting.fontObjectId
            $fontObjectRuntimeName = [string]$setting.fontObjectRuntimeName
            Require-Identifier $fontObjectId 'font object id'
            if (-not $fontObjectIds.Add($fontObjectId)) { throw "Duplicate font object id: $fontObjectId" }
            if (-not $runtimeNames.Add($fontObjectRuntimeName)) { throw "Duplicate runtimeName: $fontObjectRuntimeName" }
        }

        $setting | Add-Member -NotePropertyName resolvedField -NotePropertyValue $field
        $setting | Add-Member -NotePropertyName resolvedDefault -NotePropertyValue $resolvedDefault
        $settings.Add($setting)
    }
}
foreach ($key in $iniValues.Keys) {
    if (-not $keys.Contains($key)) { throw "Default INI key is missing from settings schema: $key" }
}

$header = New-Object 'System.Collections.Generic.List[string]'
$header.Add('// Generated by Build/New-RailgunSettings.ps1. Do not edit.')
foreach ($setting in $settings) {
    if ([string]$setting.resolvedField -notmatch '::') {
        $header.Add(('inline const FName {0}(TEXT("{1}"));' -f $setting.id, (Escape-CppString ([string]$setting.runtimeName))))
    }
}
foreach ($setting in $settings) {
    if ($setting.PSObject.Properties.Name -contains 'fontObjectId') {
        $header.Add(('inline const FName {0}(TEXT("{1}"));' -f $setting.fontObjectId, (Escape-CppString ([string]$setting.fontObjectRuntimeName))))
    }
}
foreach ($setting in $settings) {
    $id = [string]$setting.id
    $header.Add(('inline constexpr TCHAR {0}Key[] = TEXT("{1}");' -f $id, (Escape-CppString ([string]$setting.key))))
    $header.Add(('inline constexpr TCHAR {0}Default[] = TEXT("{1}");' -f $id, (Escape-CppString ([string]$setting.resolvedDefault))))
    if ([string]$setting.type -eq 'number') {
        $header.Add(('inline constexpr TCHAR {0}Minimum[] = TEXT("{1}");' -f $id, (Format-Number $setting.minimum)))
        $header.Add(('inline constexpr TCHAR {0}Maximum[] = TEXT("{1}");' -f $id, (Format-Number $setting.maximum)))
    }
}
$header.Add('inline const FNumericSetting NumericSettings[] = {')
foreach ($setting in @($settings | Where-Object { [string]$_.type -eq 'number' })) {
    $header.Add(('    {{{0}Key, {1}, {0}Default, {0}Minimum, {0}Maximum}},' -f $setting.id, $setting.resolvedField))
}
$header.Add('};')
$header.Add('inline const FTextSetting TextSettings[] = {')
foreach ($setting in @($settings | Where-Object { [string]$_.type -eq 'text' })) {
    $header.Add(('    {{{0}Key, {1}, {0}Default}},' -f $setting.id, $setting.resolvedField))
}
$header.Add('};')
$header.Add('inline const FFontSetting FontSettings[] = {')
foreach ($setting in @($settings | Where-Object { $_.PSObject.Properties.Name -contains 'fontObjectId' })) {
    $header.Add(('    {{{0}, {1}}},' -f $setting.resolvedField, $setting.fontObjectId))
}
$header.Add('};')

$utf8 = New-Object System.Text.UTF8Encoding($false)
foreach ($path in @($HeaderPath, $IniPath)) {
    $directory = Split-Path -Parent ([IO.Path]::GetFullPath($path))
    $null = New-Item -ItemType Directory -Path $directory -Force
}
[IO.File]::WriteAllLines([IO.Path]::GetFullPath($HeaderPath), $header, $utf8)
[IO.File]::WriteAllBytes([IO.Path]::GetFullPath($IniPath), [IO.File]::ReadAllBytes($resolvedDefaultIni))
Write-Host ("settings generated: {0} options" -f $settings.Count)
