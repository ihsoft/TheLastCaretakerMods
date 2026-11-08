# Offline preservation checks for the installer upgrade, with no game writes.
$ErrorActionPreference = 'Stop'
Set-StrictMode -Version Latest
$mod = Split-Path $PSScriptRoot -Parent
$repo = (Resolve-Path (Join-Path $mod '../..')).Path
$work = Join-Path $repo ('Tmp/SmartOutlet/settings-tests-' + [guid]::NewGuid().ToString('N'))
[IO.Directory]::CreateDirectory($work) | Out-Null
$source = Join-Path $work 'current.ini'
$result = Join-Path $work 'converted.ini'
$again = Join-Path $work 'again.ini'
$template = Join-Path $mod 'Assets/SmartOutlet.ini'
$convert = Join-Path $mod 'Settings/Convert-LegacySettings.ps1'
try {
    @'
# Previous help text: HighPercent / LowPercent.
RuleCount = 3
UnlockSaltWaterTank = false
Rule1Mode = "output"
Rule1SocketName = "MyPort2"
Rule1HighPercent = 91.5
Rule1LowPercent = 12
Rule2Mode = "input" # Incoming water
Rule2HighPercent = 87
Rule2LowPercent = 13
Rule3Mode = "input"
Rule3EnablePortAtPercent = 16
Rule3LowPercent = 14
Rule3HighPercent = 88
Rule3DisablePortAtPercent = 89
Rule4Mode = "output"
Rule4EnableAtPercent = 72
Rule4DisableAtPercent = 18
CustomOption = "keep # this"
'@ | Set-Content -LiteralPath $source -Encoding utf8
    & $convert -CurrentPath $source -TemplatePath $template -OutputPath $result
    $text = [IO.File]::ReadAllText($result)
    foreach ($expected in @('RuleCount = 3', 'UnlockSaltWaterTank = false',
        'Rule1SocketName = "MyPort2"', 'Rule1EnablePortAtPercent = 91.5', 'Rule1DisablePortAtPercent = 12',
        'Rule2EnablePortAtPercent = 13', 'Rule2DisablePortAtPercent = 87',
        'Rule3EnablePortAtPercent = 16', 'Rule3DisablePortAtPercent = 89',
        'Rule4EnablePortAtPercent = 72', 'Rule4DisablePortAtPercent = 18', 'CustomOption = "keep # this"')) {
        if (-not $text.Contains($expected)) { throw "Lost setting: $expected" }
    }
    if ($text -match 'HighPercent|LowPercent|Rule1ActorClass\s*=') { throw 'Legacy names or invented assignments retained' }
    & $convert -CurrentPath $result -TemplatePath $template -OutputPath $again
    if ([IO.File]::ReadAllText($again) -cne $text) { throw 'Migration is not idempotent' }
    # The user's three-rule case must not gain a fourth rule during merge.
    $text = $text -replace '(?m)^Rule4[^\r\n]*\r?\n', ''
    [IO.File]::WriteAllText($source, $text)
    Remove-Item -LiteralPath $result
    & (Join-Path $repo 'tools/Merge-VoyageModIni.ps1') -CurrentPath $source -TemplatePath $template -OutputPath $result | Out-Null
    if ([IO.File]::ReadAllText($result) -match 'Rule4') { throw 'Merge added an unwanted fourth rule' }
    [pscustomobject]@{status='passed';checks='direction mapping, values, explicit precedence, legacy aliases, unknown keys, idempotence, rule count'}
} finally {
    foreach ($file in @($source, $result, $again)) {
        if (Test-Path -LiteralPath $file) { Remove-Item -LiteralPath $file }
    }
    Remove-Item -LiteralPath $work
}
