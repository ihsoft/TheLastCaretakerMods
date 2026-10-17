[CmdletBinding()]
param(
    [Parameter(Mandatory = $true)][string]$SchemaPath,
    [Parameter(Mandatory = $true)][string]$DefaultIniPath,
    [Parameter(Mandatory = $true)][string]$HeaderPath,
    [Parameter(Mandatory = $true)][string]$IniPath,
    [Parameter(Mandatory = $true)][ValidateSet('Array', 'TArray')][string]$CollectionKind,
    [Parameter(Mandatory = $true)][string]$GeneratedBy
)

$ErrorActionPreference = 'Stop'
Set-StrictMode -Version Latest
$invariant = [Globalization.CultureInfo]::InvariantCulture

function Require-Identifier([string]$Value, [string]$Label) {
    if ($Value -notmatch '^[A-Za-z_][A-Za-z0-9_]*$') { throw "Invalid ${Label}: $Value" }
}
function Escape-CppString([string]$Value) {
    return $Value.Replace('\', '\\').Replace('"', '\"')
}
function Format-Number([object]$Value) {
    return ([double]$Value).ToString('0.###############', $invariant)
}
function Assert-Properties([object]$Value, [string[]]$Allowed, [string[]]$Required, [string]$Label) {
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
if ($schema.schemaVersion -ne 1) { throw 'Unsupported settings schema version.' }
$grouped = $schema.PSObject.Properties.Name -contains 'groups'
if ($grouped) {
    Assert-Properties $schema @('schemaVersion','groups') @('schemaVersion','groups') 'settings schema'
    if (@($schema.groups).Count -eq 0) { throw 'Settings schema has no groups.' }
    foreach ($group in @($schema.groups)) {
        Assert-Properties $group @('id','comments','settings') @('id','settings') 'settings group'
        Require-Identifier ([string]$group.id).Replace('-', '_') 'group id'
    }
    $schemaSettings = @($schema.groups | ForEach-Object { $_.settings })
} else {
    Assert-Properties $schema @('schemaVersion','section','settings') @('schemaVersion','section','settings') 'settings schema'
    if ($schema.PSObject.Properties.Name -notcontains 'section' -or [string]$schema.section -cne '') {
        throw 'Flat settings schema requires an empty section.'
    }
    $schemaSettings = @($schema.settings)
}
if ($schemaSettings.Count -eq 0) { throw 'Settings schema has no settings.' }

$resolvedDefaultIni = (Resolve-Path -LiteralPath $DefaultIniPath).Path
$iniValues = New-Object 'System.Collections.Generic.Dictionary[string,string]' ([StringComparer]::Ordinal)
$lines = [IO.File]::ReadAllLines($resolvedDefaultIni)
for ($index = 0; $index -lt $lines.Count; $index++) {
    $line = $lines[$index]
    $trimmed = $line.Trim()
    if (-not $trimmed -or $trimmed.StartsWith('#') -or $trimmed.StartsWith(';')) { continue }
    if ($line -notmatch '^\s*([A-Za-z][A-Za-z0-9]*)\s*=\s*([^\r\n]*)\s*$') {
        throw "Malformed default INI line $($index + 1): $line"
    }
    $key = $matches[1]
    $value = $matches[2].Trim()
    if (-not $value -or $value -match '[#;]') { throw "Invalid default INI value on line $($index + 1): $key" }
    if ($iniValues.ContainsKey($key)) { throw "Duplicate default INI key: $key" }
    $iniValues.Add($key, $value)
}
if ($iniValues.Count -eq 0) { throw 'Default INI has no settings.' }

$settings = New-Object 'System.Collections.Generic.List[object]'
$ids = New-Object 'System.Collections.Generic.HashSet[string]' ([StringComparer]::Ordinal)
$keys = New-Object 'System.Collections.Generic.HashSet[string]' ([StringComparer]::Ordinal)
$runtimeNames = New-Object 'System.Collections.Generic.HashSet[string]' ([StringComparer]::Ordinal)
foreach ($setting in $schemaSettings) {
    $id = [string]$setting.id
    $key = [string]$setting.key
    $type = [string]$setting.type
    Assert-Properties $setting @('id','runtimeName','field','key','type','minimum','maximum') `
        @('id','key','type') "setting '$id'"
    Require-Identifier $id 'setting id'
    if ($key -notmatch '^[A-Za-z][A-Za-z0-9]*$') { throw "Invalid setting key: $key" }
    if (-not $ids.Add($id)) { throw "Duplicate setting id: $id" }
    if (-not $keys.Add($key)) { throw "Duplicate setting key: $key" }
    if (-not $iniValues.ContainsKey($key)) { throw "Setting is missing from default INI: $key" }
    if ($type -notin @('number', 'boolean')) { throw "Unsupported setting type: $id" }
    if ($CollectionKind -eq 'Array' -and $type -eq 'boolean') {
        throw 'Array output does not support boolean settings.'
    }
    $field = if ($setting.PSObject.Properties.Name -contains 'field') { [string]$setting.field } else { $id }
    if ($field -notmatch '^[A-Za-z_][A-Za-z0-9_]*(::[A-Za-z_][A-Za-z0-9_]*)?$') {
        throw "Invalid field expression: $field"
    }
    $runtimeName = if ($setting.PSObject.Properties.Name -contains 'runtimeName') { [string]$setting.runtimeName } else { '' }
    if ($field -match '::') {
        if ($CollectionKind -eq 'TArray') { throw 'TArray output does not support external fields.' }
        if ($runtimeName) { throw "External field must not declare runtimeName: $id" }
    } else {
        Require-Identifier $runtimeName 'runtimeName'
        if (-not $runtimeNames.Add($runtimeName)) { throw "Duplicate runtimeName: $runtimeName" }
    }
    $value = $iniValues[$key]
    if ($type -eq 'number') {
        if ($setting.PSObject.Properties.Name -notcontains 'minimum' -or
            $setting.PSObject.Properties.Name -notcontains 'maximum') { throw "Missing numeric range: $id" }
        $number = 0.0
        if (-not [double]::TryParse($value, [Globalization.NumberStyles]::Float, $invariant, [ref]$number) -or
            [double]::IsNaN($number) -or [double]::IsInfinity($number)) { throw "Invalid numeric default: $id" }
        $min = [double]$setting.minimum
        $max = [double]$setting.maximum
        if ([double]::IsNaN($min) -or [double]::IsInfinity($min) -or
            [double]::IsNaN($max) -or [double]::IsInfinity($max) -or
            $min -gt $max -or $number -lt $min -or $number -gt $max) { throw "Invalid numeric range: $id" }
    } else {
        if ($setting.PSObject.Properties.Name -contains 'minimum' -or
            $setting.PSObject.Properties.Name -contains 'maximum') { throw "Boolean setting has numeric range: $id" }
        if ($value -cnotin @('true', 'false')) { throw "Invalid boolean default: $id" }
    }
    $settings.Add([pscustomobject]@{
        id=$id; key=$key; type=$type; field=$field; runtimeName=$runtimeName;
        value=$value;
        minimum=$(if ($type -eq 'number') { $setting.minimum } else { $null });
        maximum=$(if ($type -eq 'number') { $setting.maximum } else { $null })
    })
}
foreach ($key in $iniValues.Keys) {
    if (-not $keys.Contains($key)) { throw "Default INI key is missing from schema: $key" }
}

$header = New-Object 'System.Collections.Generic.List[string]'
$header.Add("// Generated by $GeneratedBy. Do not edit.")
if ($CollectionKind -eq 'Array') {
    foreach ($setting in $settings) {
        if ($setting.field -notmatch '::') {
            $header.Add(('inline const FName {0}(TEXT("{1}"));' -f $setting.id, (Escape-CppString $setting.runtimeName)))
        }
    }
}
foreach ($setting in $settings) {
    if ($CollectionKind -eq 'TArray') {
        $header.Add(('inline const FName {0}(TEXT("{1}"));' -f $setting.id, (Escape-CppString $setting.runtimeName)))
    }
    $header.Add(('inline constexpr TCHAR {0}Key[] = TEXT("{1}");' -f $setting.id, (Escape-CppString $setting.key)))
    $header.Add(('inline constexpr TCHAR {0}Default[] = TEXT("{1}");' -f $setting.id, (Escape-CppString $setting.value)))
    if ($setting.type -eq 'number') {
        $header.Add(('inline constexpr TCHAR {0}Minimum[] = TEXT("{1}");' -f $setting.id, (Format-Number $setting.minimum)))
        $header.Add(('inline constexpr TCHAR {0}Maximum[] = TEXT("{1}");' -f $setting.id, (Format-Number $setting.maximum)))
    }
}
$numericDeclaration = if ($CollectionKind -eq 'Array') { 'inline const FNumericSetting NumericSettings[] = {' } else { 'inline const TArray<FNumericSetting> NumericSettings = {' }
$header.Add($numericDeclaration)
foreach ($setting in $settings) {
    if ($setting.type -ne 'number') { continue }
    if ($CollectionKind -eq 'Array') {
        $header.Add(('    {{{0}Key, {1}, {0}Default, {0}Minimum, {0}Maximum}},' -f $setting.id, $setting.field))
    } else {
        $header.Add(('    {{{0}Key, {0}, {0}Default, {0}Minimum, {0}Maximum}},' -f $setting.id))
    }
}
$header.Add('};')
if ($CollectionKind -eq 'TArray') {
    $header.Add('inline const TArray<FBooleanSetting> BooleanSettings = {')
    foreach ($setting in $settings) {
        if ($setting.type -eq 'boolean') { $header.Add(('    {{{0}Key, {0}, {0}Default}},' -f $setting.id)) }
    }
    $header.Add('};')
}
$utf8 = New-Object System.Text.UTF8Encoding($false)
foreach ($target in @([IO.Path]::GetFullPath($HeaderPath),[IO.Path]::GetFullPath($IniPath))) {
    foreach ($input in @($resolvedSchema,$resolvedDefaultIni)) {
        if ($target.Equals($input, [StringComparison]::OrdinalIgnoreCase)) {
            throw 'Generated outputs must not overwrite inputs.'
        }
    }
}
if ([IO.Path]::GetFullPath($HeaderPath).Equals([IO.Path]::GetFullPath($IniPath),
        [StringComparison]::OrdinalIgnoreCase)) { throw 'HeaderPath and IniPath must differ.' }
foreach ($path in @($HeaderPath, $IniPath)) {
    $null = New-Item -ItemType Directory -Path (Split-Path -Parent ([IO.Path]::GetFullPath($path))) -Force
}
[IO.File]::WriteAllLines([IO.Path]::GetFullPath($HeaderPath), $header, $utf8)
[IO.File]::WriteAllBytes([IO.Path]::GetFullPath($IniPath), [IO.File]::ReadAllBytes($resolvedDefaultIni))
Write-Host ("settings generated: {0} options" -f $settings.Count)
