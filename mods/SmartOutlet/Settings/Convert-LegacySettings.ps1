# Offline upgrade of old threshold names; explicit new keys always win.
[CmdletBinding()]
param(
    [Parameter(Mandatory=$true)][string]$CurrentPath,
    [Parameter(Mandatory=$true)][string]$TemplatePath,
    [Parameter(Mandatory=$true)][string]$OutputPath
)
$ErrorActionPreference = 'Stop'
Set-StrictMode -Version Latest
$text = [IO.File]::ReadAllText((Resolve-Path -LiteralPath $CurrentPath).Path)
$templateText = [IO.File]::ReadAllText((Resolve-Path -LiteralPath $TemplatePath).Path)
if ($templateText -cnotmatch '(?m)^Rule[0-9]+EnablePortAtPercent\s*=') {
    throw 'This installer requires a candidate with explicit EnablePortAtPercent/DisablePortAtPercent settings. For an older candidate, use its matching installer and INI backup.'
}
$assignment = '(?m)^[ \t]*(?<key>[A-Za-z][A-Za-z0-9]*)[ \t]*=(?<value>[^\r\n]*)'
$values = [Collections.Generic.Dictionary[string,string]]::new([StringComparer]::Ordinal)
foreach ($match in [regex]::Matches($text, $assignment)) {
    $values[$match.Groups['key'].Value] = $match.Groups['value'].Value.Trim()
}
$legacyKeys = @($values.Keys | Where-Object { $_ -cmatch '^Rule[0-9]+(HighPercent|LowPercent|EnableAtPercent|DisableAtPercent)$' })
if ($legacyKeys.Count) {
    # Convert the oldest output-only names first, then the mode-aware range names.
    foreach ($pair in @(@('EnableAtPercent','HighPercent'), @('DisableAtPercent','LowPercent'))) {
        foreach ($key in @($values.Keys)) {
            if ($key -cmatch ('^(Rule[0-9]+)' + $pair[0] + '$')) {
                $newKey = $Matches[1] + $pair[1]
                if (-not $values.ContainsKey($newKey)) { $values[$newKey] = $values[$key] }
                $values.Remove($key) | Out-Null
            }
        }
    }
    foreach ($key in @($values.Keys)) {
        if ($key -cmatch '^(Rule[0-9]+)(HighPercent|LowPercent)$') {
            $prefix = $Matches[1]; $high = $Matches[2] -ceq 'HighPercent'
            $inputMode = $values.ContainsKey($prefix + 'Mode') -and $values[$prefix + 'Mode'] -cmatch '^"input"\s*(#.*)?$'
            $suffix = if ($high -ne $inputMode) { 'EnablePortAtPercent' } else { 'DisablePortAtPercent' }
            $newKey = $prefix + $suffix
            if (-not $values.ContainsKey($newKey)) { $values[$newKey] = $values[$key] }
            $values.Remove($key) | Out-Null
        }
    }
    # Retire obsolete threshold comments with the old keys. The template owns the
    # short help text; only existing assignments are emitted here, with their
    # original values. The normal merge adds missing defaults afterwards.
    $lines = [Collections.Generic.List[string]]::new()
    foreach ($line in [IO.File]::ReadAllLines((Resolve-Path -LiteralPath $TemplatePath).Path)) {
        if ($line -cmatch '^([A-Za-z][A-Za-z0-9]*)\s*=') {
            $key = $Matches[1]
            if ($values.ContainsKey($key)) {
                $lines.Add($key + ' = ' + $values[$key])
                $values.Remove($key) | Out-Null
            }
        } else { $lines.Add($line) }
    }
    if ($values.Count) {
        $lines.Add(''); $lines.Add('# Additional settings and rules retained from your file.')
        foreach ($key in $values.Keys) { $lines.Add($key + ' = ' + $values[$key]) }
    }
    $text = ($lines -join "`n") + "`n"
}
[IO.File]::WriteAllText([IO.Path]::GetFullPath($OutputPath), $text, [Text.UTF8Encoding]::new($false))
