# Upgrade flat port rules to section profiles. Sectioned player files remain unchanged.
[CmdletBinding()]
param([Parameter(Mandatory=$true)][string]$CurrentPath,
      [Parameter(Mandatory=$true)][string]$TemplatePath,
      [Parameter(Mandatory=$true)][string]$OutputPath)
$ErrorActionPreference='Stop'
Set-StrictMode -Version Latest
& (Join-Path $PSScriptRoot 'Test-SettingsTemplate.ps1') -Path $TemplatePath | Out-Null
$text=[IO.File]::ReadAllText((Resolve-Path -LiteralPath $CurrentPath).Path)
$destination=[IO.Path]::GetFullPath($OutputPath)
if ($text -match '(?m)^\s*\[' -or $text -cnotmatch '(?m)^\s*RuleCount\s*=') {
    # An absent profile is intentional: never merge template modes into a player file.
    [IO.File]::WriteAllText($destination,$text,[Text.UTF8Encoding]::new($false)); return
}
$values=[Collections.Generic.Dictionary[string,string]]::new([StringComparer]::Ordinal)
foreach ($match in [regex]::Matches($text,'(?m)^[ \t]*(?<key>[A-Za-z][A-Za-z0-9]*)[ \t]*=(?<value>[^\r\n]*)')) {
    $values[$match.Groups['key'].Value]=$match.Groups['value'].Value.Trim()
}
function Read-LegacyString([string]$key,[string]$fallback='') {
    if (-not $values.ContainsKey($key)) { return $fallback }
    if ($values[$key] -cnotmatch '^"(?<value>(?:[^"\\]|\\["\\])*)"\s*(?:#.*)?$') { throw "Invalid legacy string: $key; original INI is unchanged." }
    return [regex]::Replace($Matches.value,'\\(["\\])','$1')
}
function Read-LegacyNumber([string[]]$keys,[string]$fallback) {
    foreach ($key in $keys) {
        if ($values.ContainsKey($key)) {
            $raw=($values[$key] -split '#',2)[0].Trim(); $number=0.0
            if ($raw -cnotmatch '^[+-]?(?:[0-9]+(?:\.[0-9]*)?|\.[0-9]+)(?:[eE][+-]?[0-9]+)?$' -or
                -not [double]::TryParse($raw,[Globalization.NumberStyles]::Float,[Globalization.CultureInfo]::InvariantCulture,[ref]$number) -or
                [double]::IsNaN($number) -or [double]::IsInfinity($number)) { throw "Invalid legacy number: $key; original INI is unchanged." }
            return $number.ToString('R',[Globalization.CultureInfo]::InvariantCulture)
        }
    }
    return $fallback
}
$count=[double](Read-LegacyNumber @('RuleCount') '0')
if ($count -lt 0 -or $count -gt 64 -or $count -ne [math]::Truncate($count)) { throw 'Invalid legacy RuleCount' }
$profiles=[Collections.Generic.Dictionary[string,string]]::new([StringComparer]::Ordinal)
for ($i=1; $i -le $count; $i++) {
    $prefix="Rule$i"; $actor=Read-LegacyString ($prefix+'ActorClass'); $mode=Read-LegacyString ($prefix+'Mode') 'output'
    if ($mode -cnotin @('input','output') -or -not $actor -or $actor -match '[\[\]\r\n]') { throw "Cannot migrate $prefix without a valid ActorClass and mode" }
    $classKey=($actor -split '/')[-1]; $inputMode=$mode -ceq 'input'
    $enableAliases=if ($inputMode) { @('LowPercent','DisableAtPercent') } else { @('HighPercent','EnableAtPercent') }
    $disableAliases=if ($inputMode) { @('HighPercent','EnableAtPercent') } else { @('LowPercent','DisableAtPercent') }
    $enable=Read-LegacyNumber (@($prefix+'EnablePortAtPercent')+@($enableAliases | ForEach-Object {$prefix+$_})) $(if ($inputMode) {'20'} else {'80'})
    $disable=Read-LegacyNumber (@($prefix+'DisablePortAtPercent')+@($disableAliases | ForEach-Object {$prefix+$_})) $(if ($inputMode) {'80'} else {'20'})
    $slot=Read-LegacyNumber @($prefix+'GlowMaterialSlot') '0'
    $section=$classKey+$(if ($inputMode) {':Input'} else {':Output'})
    $body="EnablePortAtPercent = $enable`nDisablePortAtPercent = $disable`nGlowMaterialSlot = $slot"
    if ($profiles.ContainsKey($section) -and $profiles[$section] -cne $body) { throw "Conflicting legacy rules for [$section]. Choose one class-wide profile before installing; original INI is unchanged." }
    $profiles[$section]=$body
}
$lines=[Collections.Generic.List[string]]::new()
$lines.Add('# Smart Outlet. Restart the game after editing.')
$lines.Add('# Sections apply to every port of the named storage class; missing modes are unavailable.')
$lines.Add('# V selects a mode. Enable/Disable are fill percentages; -1 disables glow.')
$lines.Add('')
foreach ($key in $values.Keys) {
    if ($key -cne 'RuleCount' -and $key -cnotmatch '^Rule[0-9]+') { $lines.Add($key+' = '+$values[$key]) }
}
foreach ($section in $profiles.Keys) { $lines.Add('');$lines.Add('['+$section+']');$lines.Add($profiles[$section]) }
[IO.File]::WriteAllText($destination,($lines -join "`n")+"`n",[Text.UTF8Encoding]::new($false))
