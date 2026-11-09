# Validate the shipped section-based template. This does not read or rewrite player settings.
[CmdletBinding()]
param([Parameter(Mandatory=$true)][string]$Path)
$ErrorActionPreference = 'Stop'
Set-StrictMode -Version Latest
$sections = [Collections.Generic.Dictionary[string,object]]::new([StringComparer]::Ordinal)
$section = ''
$sections.Add('', [Collections.Generic.Dictionary[string,string]]::new([StringComparer]::Ordinal))
foreach ($line in [IO.File]::ReadAllLines((Resolve-Path -LiteralPath $Path).Path)) {
    $text = $line.Trim()
    if (-not $text -or $text.StartsWith('#')) { continue }
    if ($text.StartsWith('[')) {
        if ($text -cnotmatch '^\[(?<section>[^\[\]\r\n]+)\]\s*(?:#.*)?$') { throw "Invalid section: $line" }
        $section = $Matches.section.Trim()
        if ($section -cnotmatch '^.+\..+:(Input|Output)$' -or $sections.ContainsKey($section)) { throw "Invalid or duplicate profile: $section" }
        $sections.Add($section, [Collections.Generic.Dictionary[string,string]]::new([StringComparer]::Ordinal))
        continue
    }
    if ($text -cnotmatch '^(?<key>[A-Za-z][A-Za-z0-9]*)\s*=\s*(?<value>[^#]*)(?:#.*)?$') { throw "Invalid template assignment: $line" }
    $key = $Matches.key; $value = $Matches.value.Trim()
    if ($sections[$section].ContainsKey($key)) { throw "Duplicate setting: [$section] $key" }
    $sections[$section].Add($key,$value)
}
$global = $sections['']
if ($global.Count -ne 1 -or -not $global.ContainsKey('UnlockSaltWaterTank') -or $global['UnlockSaltWaterTank'] -notmatch '^(true|false)$') { throw 'Expected global UnlockSaltWaterTank boolean' }
foreach ($name in $sections.Keys) {
    if (-not $name) { continue }
    $values = $sections[$name]
    if ($values.Count -ne 3) { throw "Expected three settings in [$name]" }
    $parsed = @{}
    foreach ($key in @('EnablePortAtPercent','DisablePortAtPercent','GlowMaterialSlot')) {
        $number = 0.0
        if (-not $values.ContainsKey($key) -or $values[$key] -cnotmatch '^[+-]?(?:[0-9]+(?:\.[0-9]*)?|\.[0-9]+)(?:[eE][+-]?[0-9]+)?$' -or
            -not [double]::TryParse($values[$key],[Globalization.NumberStyles]::Float,[Globalization.CultureInfo]::InvariantCulture,[ref]$number) -or
            [double]::IsNaN($number) -or [double]::IsInfinity($number)) { throw "Invalid number: [$name] $key" }
        $parsed[$key] = $number
    }
    $enable=$parsed.EnablePortAtPercent; $disable=$parsed.DisablePortAtPercent; $slot=$parsed.GlowMaterialSlot
    if ($enable -lt 0 -or $enable -gt 100 -or $disable -lt 0 -or $disable -gt 100 -or
        $slot -lt -1 -or $slot -gt 31 -or $slot -ne [math]::Truncate($slot) -or
        ($name.EndsWith(':Input',[StringComparison]::Ordinal) -and $enable -ge $disable) -or
        ($name.EndsWith(':Output',[StringComparison]::Ordinal) -and $enable -le $disable)) { throw "Invalid thresholds or material slot: [$name]" }
}
[pscustomobject]@{status='valid';profiles=$sections.Count-1}
