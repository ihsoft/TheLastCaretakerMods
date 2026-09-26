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
    if ($Value -notmatch '^[A-Za-z_][A-Za-z0-9_]*$') {
        throw "Invalid ${Label}: $Value"
    }
}

function Assert-ExactProperties([object]$Value, [string[]]$Allowed, [string[]]$Required, [string]$Label) {
    $names = @($Value.PSObject.Properties.Name)
    foreach ($name in $names) {
        if ($Allowed -cnotcontains $name) { throw "Unknown property in ${Label}: $name" }
    }
    foreach ($name in $Required) {
        if ($names -cnotcontains $name) { throw "Missing property in ${Label}: $name" }
    }
}

$resolvedSchema = (Resolve-Path -LiteralPath $SchemaPath).Path
$schema = Get-Content -LiteralPath $resolvedSchema -Raw | ConvertFrom-Json
Assert-ExactProperties $schema @('schemaVersion', 'section', 'settings') `
    @('schemaVersion', 'section', 'settings') 'settings schema'
if ($schema.schemaVersion -ne 1) { throw 'Unsupported GyroKeyboardControl settings schema version.' }
if ($null -eq $schema.section -or [string]$schema.section -cne '') {
    throw 'GyroKeyboardControl uses a flat INI; schema section must be an empty string.'
}
$schemaSettings = @($schema.settings)
if ($schemaSettings.Count -eq 0) { throw 'GyroKeyboardControl settings schema has no settings.' }

$resolvedDefaultIni = (Resolve-Path -LiteralPath $DefaultIniPath).Path
$defaultIniLines = @([IO.File]::ReadAllLines($resolvedDefaultIni))
$iniValues = New-Object 'System.Collections.Generic.Dictionary[string,string]' ([StringComparer]::Ordinal)
for ($lineIndex = 0; $lineIndex -lt $defaultIniLines.Count; $lineIndex++) {
    $line = $defaultIniLines[$lineIndex]
    $trimmed = $line.Trim()
    if ([string]::IsNullOrWhiteSpace($trimmed) -or $trimmed.StartsWith('#') -or $trimmed.StartsWith(';')) {
        continue
    }
    if ($line -notmatch '^\s*([A-Za-z][A-Za-z0-9]*)\s*=\s*([^\r\n]*)\s*$') {
        throw "Malformed default INI line $($lineIndex + 1): $line"
    }
    $key = $matches[1]
    $rawValue = $matches[2].Trim()
    if ([string]::IsNullOrWhiteSpace($rawValue)) {
        throw "Empty default INI value on line $($lineIndex + 1): $key"
    }
    if ($rawValue -match '[#;]') {
        throw "Inline comments are not supported on default INI line $($lineIndex + 1): $key"
    }
    $value = $rawValue
    if ($iniValues.ContainsKey($key)) { throw "Duplicate setting key in default INI: $key" }
    $iniValues.Add($key, $value)
}
if ($iniValues.Count -eq 0) { throw 'GyroKeyboardControl default INI has no settings.' }

$settings = New-Object 'System.Collections.Generic.List[object]'
$ids = New-Object 'System.Collections.Generic.HashSet[string]' ([StringComparer]::Ordinal)
$keys = New-Object 'System.Collections.Generic.HashSet[string]' ([StringComparer]::Ordinal)
$runtimeNames = New-Object 'System.Collections.Generic.HashSet[string]' ([StringComparer]::Ordinal)
foreach ($setting in $schemaSettings) {
    Assert-ExactProperties $setting @('id', 'runtimeName', 'key', 'type', 'minimum', 'maximum') `
        @('id', 'runtimeName', 'key', 'type') "setting '$($setting.id)'"
    $id = [string]$setting.id
    $runtimeName = [string]$setting.runtimeName
    $key = [string]$setting.key
    $type = [string]$setting.type
    Require-Identifier $id 'setting id'
    Require-Identifier $runtimeName 'runtimeName'
    if ($key -notmatch '^[A-Za-z][A-Za-z0-9]*$') { throw "Invalid setting key: $key" }
    if (-not $ids.Add($id)) { throw "Duplicate setting id: $id" }
    if (-not $runtimeNames.Add($runtimeName)) { throw "Duplicate runtimeName: $runtimeName" }
    if (-not $keys.Add($key)) { throw "Duplicate setting key: $key" }
    if (-not $iniValues.ContainsKey($key)) { throw "Setting is missing from default INI: $key" }
    if ($type -notin @('number', 'boolean')) { throw "Unsupported setting type for ${id}: $type" }

    $resolvedDefault = $iniValues[$key]
    if ($type -eq 'number') {
        foreach ($name in @('minimum', 'maximum')) {
            if ($setting.PSObject.Properties.Name -cnotcontains $name) {
                throw "Missing $name for numeric setting: $id"
            }
        }
        $default = 0.0
        if (-not [double]::TryParse($resolvedDefault, [Globalization.NumberStyles]::Float,
            $invariant, [ref]$default) -or [double]::IsNaN($default) -or [double]::IsInfinity($default)) {
            throw "Default INI value is not a finite number for ${key}: $resolvedDefault"
        }
        $minimum = [double]$setting.minimum
        $maximum = [double]$setting.maximum
        if ([double]::IsNaN($minimum) -or [double]::IsInfinity($minimum) -or
            [double]::IsNaN($maximum) -or [double]::IsInfinity($maximum) -or
            $minimum -gt $maximum -or $default -lt $minimum -or $default -gt $maximum) {
            throw "Numeric default/range contract is invalid: $id"
        }
    } else {
        if ($setting.PSObject.Properties.Name -ccontains 'minimum' -or
            $setting.PSObject.Properties.Name -ccontains 'maximum') {
            throw "Boolean setting must not declare a numeric range: $id"
        }
        if ($resolvedDefault -cnotin @('true', 'false')) {
            throw "Default INI value is not a lowercase boolean for ${key}: $resolvedDefault"
        }
    }

    $setting | Add-Member -NotePropertyName resolvedDefault -NotePropertyValue $resolvedDefault
    $settings.Add($setting)
}
foreach ($key in $iniValues.Keys) {
    if (-not $keys.Contains($key)) { throw "Default INI key is missing from settings schema: $key" }
}

$header = New-Object 'System.Collections.Generic.List[string]'
$header.Add('// Generated by Build/New-GyroKeyboardControlSettings.ps1. Do not edit.')
foreach ($setting in $settings) {
    $id = [string]$setting.id
    $header.Add(('inline const FName {0}(TEXT("{1}"));' -f $id, (Escape-CppString ([string]$setting.runtimeName))))
    $header.Add(('inline constexpr TCHAR {0}Key[] = TEXT("{1}");' -f $id, (Escape-CppString ([string]$setting.key))))
    $header.Add(('inline constexpr TCHAR {0}Default[] = TEXT("{1}");' -f $id, (Escape-CppString ([string]$setting.resolvedDefault))))
    if ([string]$setting.type -eq 'number') {
        $header.Add(('inline constexpr TCHAR {0}Minimum[] = TEXT("{1}");' -f $id, (Format-Number $setting.minimum)))
        $header.Add(('inline constexpr TCHAR {0}Maximum[] = TEXT("{1}");' -f $id, (Format-Number $setting.maximum)))
    }
}
$header.Add('inline const TArray<FNumericSetting> NumericSettings = {')
foreach ($setting in @($settings | Where-Object { [string]$_.type -eq 'number' })) {
    $header.Add(('    {{{0}Key, {0}, {0}Default, {0}Minimum, {0}Maximum}},' -f $setting.id))
}
$header.Add('};')
$header.Add('inline const TArray<FBooleanSetting> BooleanSettings = {')
foreach ($setting in @($settings | Where-Object { [string]$_.type -eq 'boolean' })) {
    $header.Add(('    {{{0}Key, {0}, {0}Default}},' -f $setting.id))
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
