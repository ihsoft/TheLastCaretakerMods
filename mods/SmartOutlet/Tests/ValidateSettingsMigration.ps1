# Offline migration acceptance checks; never writes game files.
$ErrorActionPreference='Stop'
Set-StrictMode -Version Latest
$mod=Split-Path $PSScriptRoot -Parent
$repo=(Resolve-Path (Join-Path $mod '../..')).Path
$work=Join-Path $repo ('Tmp/SmartOutlet/settings-tests-'+[guid]::NewGuid().ToString('N'))
[IO.Directory]::CreateDirectory($work) | Out-Null
$source=Join-Path $work 'current.ini';$result=Join-Path $work 'result.ini';$again=Join-Path $work 'again.ini'
$template=Join-Path $mod 'Assets/SmartOutlet.ini';$convert=Join-Path $mod 'Settings/Convert-LegacySettings.ps1'
try {
    & (Join-Path $mod 'Settings/Test-SettingsTemplate.ps1') -Path $template | Out-Null
    $legacy=@(
        'RuleCount=3','UnlockSaltWaterTank=false','CustomOption="keep # this"',
        'Rule1ActorClass="/Game/Test/Battery.Battery_C"','Rule1Mode="output"','Rule1SocketName="PortA"','Rule1HighPercent=91.5','Rule1LowPercent=12','Rule1GlowMaterialSlot=-1',
        'Rule2ActorClass="/Game/Test/Water.Water_C"','Rule2Mode="input"','Rule2HighPercent=87','Rule2LowPercent=13',
        'Rule3ActorClass="/Game/Test/Battery.Battery_C"','Rule3Mode="input"','Rule3EnablePortAtPercent=16','Rule3LowPercent=14','Rule3HighPercent=88','Rule3DisablePortAtPercent=89'
    ) -join "`n"
    [IO.File]::WriteAllText($source,$legacy)
    & $convert -CurrentPath $source -TemplatePath $template -OutputPath $result
    $text=[IO.File]::ReadAllText($result)
    foreach ($expected in @('[Battery.Battery_C:Output]','[Battery.Battery_C:Input]','[Water.Water_C:Input]',
        'EnablePortAtPercent = 91.5','DisablePortAtPercent = 12','EnablePortAtPercent = 13','DisablePortAtPercent = 87',
        'EnablePortAtPercent = 16','DisablePortAtPercent = 89','GlowMaterialSlot = -1','UnlockSaltWaterTank = false','CustomOption = "keep # this"')) {
        if (-not $text.Contains($expected)) { throw "Lost setting: $expected" }
    }
    if ($text -match 'RuleCount|SocketName|ActorClass|HighPercent|LowPercent|\[Water.Water_C:Output\]') { throw 'Retained old selectors or invented a mode' }
    & $convert -CurrentPath $result -TemplatePath $template -OutputPath $again
    if ([IO.File]::ReadAllText($again) -cne $text) { throw 'Sectioned file was rewritten' }
    # Removing every section must not restore the shipped modes on the next install.
    [IO.File]::WriteAllText($source,"# no smart modes`nUnlockSaltWaterTank=false`n")
    & $convert -CurrentPath $source -TemplatePath $template -OutputPath $result
    if ([IO.File]::ReadAllText($source) -cne [IO.File]::ReadAllText($result)) { throw 'Empty configuration changed' }
    [IO.File]::WriteAllText($source,$legacy.Replace('Water.Water_C','Battery.Battery_C').Replace('Rule2Mode="input"','Rule2Mode="output"'))
    $rejected=$false
    try { & $convert -CurrentPath $source -TemplatePath $template -OutputPath $result } catch { $rejected=$_.Exception.Message.Contains('Conflicting legacy rules') }
    if (-not $rejected) { throw 'Conflicting class profiles accepted' }
    [IO.File]::WriteAllText($source,"RuleCount=0`nUnlockSaltWaterTank=false")
    & $convert -CurrentPath $source -TemplatePath $template -OutputPath $result
    if ([IO.File]::ReadAllText($result) -match '\[') { throw 'RuleCount=0 gained profiles' }
    [pscustomobject]@{status='passed';checks='values, aliases, independent profiles, absent modes, idempotence, empty config, conflicts, disabled legacy rules'}
} catch {
    throw "Settings migration checks failed; evidence: $work. $_"
}
if (-not ([IO.Path]::GetFullPath($work)).StartsWith((Join-Path $repo 'Tmp/SmartOutlet') + [IO.Path]::DirectorySeparatorChar, [StringComparison]::OrdinalIgnoreCase)) { throw 'Unexpected fixture cleanup path' }
foreach ($file in @($source,$result,$again)) { if (Test-Path -LiteralPath $file) { Remove-Item -LiteralPath $file } }
Remove-Item -LiteralPath $work
