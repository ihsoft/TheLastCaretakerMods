[CmdletBinding()]
param(
    [switch]$KeepArtifacts
)

$ErrorActionPreference = 'Stop'
Set-StrictMode -Version Latest

$repositoryRoot = (Resolve-Path -LiteralPath (Join-Path $PSScriptRoot '..\..')).Path
$testParent = Join-Path $repositoryRoot 'Tmp\tests\voyage-mod-settings'
$testRoot = Join-Path $testParent ([Guid]::NewGuid().ToString('N'))
$generator = Join-Path $repositoryRoot 'tools\New-VoyageModSettings.ps1'
$merger = Join-Path $repositoryRoot 'tools\Merge-VoyageModIni.ps1'
$checks = [Collections.Generic.List[object]]::new()
$testPassed = $false
$utf8NoBom = [Text.UTF8Encoding]::new($false)

function Add-Check {
    param([string]$Name, [bool]$Passed, [string]$Evidence)
    $checks.Add([pscustomobject][ordered]@{
        name = $Name
        passed = $Passed
        evidence = $Evidence
    })
    if (-not $Passed) { throw "Check failed: $Name. $Evidence" }
}

function Write-TextFile {
    param([string]$Path, [string]$Text, [Text.Encoding]$Encoding = $script:utf8NoBom)
    [void][IO.Directory]::CreateDirectory([IO.Path]::GetDirectoryName($Path))
    [IO.File]::WriteAllText($Path, $Text, $Encoding)
}

function Write-BytesFile {
    param([string]$Path, [byte[]]$Bytes)
    [void][IO.Directory]::CreateDirectory([IO.Path]::GetDirectoryName($Path))
    [IO.File]::WriteAllBytes($Path, $Bytes)
}

function New-CasePath([string]$Name) {
    $path = Join-Path $testRoot $Name
    [void][IO.Directory]::CreateDirectory($path)
    return $path
}

function Invoke-GeneratorCase {
    param([string]$Name, [string]$Schema, [string]$Ini)
    $case = New-CasePath $Name
    $schemaPath = Join-Path $case 'settings.json'
    $iniPath = Join-Path $case 'default.ini'
    $headerPath = Join-Path $case 'generated\Settings.h'
    $copyPath = Join-Path $case 'generated\Settings.ini'
    Write-TextFile $schemaPath $Schema
    Write-TextFile $iniPath $Ini
    $result = & $generator -SchemaPath $schemaPath -DefaultIniPath $iniPath `
        -HeaderPath $headerPath -IniPath $copyPath -GeneratedBy 'Test-VoyageModSettings.ps1'
    return [pscustomobject]@{
        Result = $result
        HeaderPath = $headerPath
        IniPath = $copyPath
        InputIniPath = $iniPath
        Header = [IO.File]::ReadAllText($headerPath)
    }
}

function Assert-GeneratorRejected {
    param([string]$Name, [string]$Schema, [string]$Ini, [string]$Pattern)
    try {
        [void](Invoke-GeneratorCase $Name $Schema $Ini)
    }
    catch {
        if ($_.Exception.Message -notmatch $Pattern) { throw }
        Add-Check $Name $true $_.Exception.Message
        return
    }
    throw "Expected generator rejection did not occur: $Name ($Pattern)"
}

function Invoke-MergeCase {
    param(
        [string]$Name,
        [byte[]]$TemplateBytes,
        [byte[]]$CurrentBytes,
        [string[]]$RetiredKeys = @(),
        [string[]]$RetiredComments = @(),
        [switch]$NoCurrent
    )
    $case = New-CasePath $Name
    $template = Join-Path $case 'template.ini'
    $current = Join-Path $case 'current.ini'
    $output = Join-Path $case 'output.ini'
    Write-BytesFile $template $TemplateBytes
    if (-not $NoCurrent) { Write-BytesFile $current $CurrentBytes }
    $templateBefore = [IO.File]::ReadAllBytes($template)
    $currentBefore = if ($NoCurrent) { $null } else { [IO.File]::ReadAllBytes($current) }
    $parameters = @{
        TemplatePath = $template
        OutputPath = $output
        RetiredKeys = $RetiredKeys
        RetiredComments = $RetiredComments
    }
    if (-not $NoCurrent) { $parameters.CurrentPath = $current }
    $result = & $merger @parameters
    Add-Check "$Name-inputs-unchanged" `
        ([Linq.Enumerable]::SequenceEqual([byte[]]$templateBefore, [byte[]][IO.File]::ReadAllBytes($template)) -and
            ($NoCurrent -or [Linq.Enumerable]::SequenceEqual([byte[]]$currentBefore, [byte[]][IO.File]::ReadAllBytes($current)))) `
        'Template and current input bytes match their pre-merge snapshots.'
    return [pscustomobject]@{
        Result = $result
        OutputPath = $output
        OutputBytes = [IO.File]::ReadAllBytes($output)
    }
}

function Utf8Bytes([string]$Text, [bool]$Bom = $false) {
    return [Text.UTF8Encoding]::new($Bom).GetBytes($Text)
}

function EncodedBytes([string]$Text, [Text.Encoding]$Encoding) {
    $preamble = $Encoding.GetPreamble()
    $payload = $Encoding.GetBytes($Text)
    $bytes = [byte[]]::new($preamble.Length + $payload.Length)
    [Array]::Copy($preamble, 0, $bytes, 0, $preamble.Length)
    [Array]::Copy($payload, 0, $bytes, $preamble.Length, $payload.Length)
    return $bytes
}

function Schema([string]$SettingsJson) {
    return '{"schemaVersion":2,"settings":[' + $SettingsJson + ']}'
}

[void][IO.Directory]::CreateDirectory($testRoot)

try {
    $basicSchema = Schema '{"id":"OnlyValue","type":"number","default":1}'
    $basic = Invoke-GeneratorCase 'single-setting' $basicSchema "OnlyValue=2`r`n"
    Add-Check 'single-setting-cardinality' `
        (@($basic.Result).Count -eq 1 -and [int]$basic.Result.SettingCount -eq 1) `
        ((@($basic.Result).Count).ToString() + ' result object; SettingCount=' + $basic.Result.SettingCount)
    Add-Check 'minimal-setting-defaulted-identities' `
        ($basic.Header.Contains('inline const FName OnlyValue(TEXT("OnlyValue"));') -and
            $basic.Header.Contains('{TEXT("OnlyValue"), OnlyValue, ESettingType::Number')) `
        $basic.Header
    Add-Check 'ini-copied-byte-for-byte' `
        ([Linq.Enumerable]::SequenceEqual(
            [byte[]][IO.File]::ReadAllBytes($basic.InputIniPath),
            [byte[]][IO.File]::ReadAllBytes($basic.IniPath))) `
        'Generated INI exactly matches the authored input bytes.'

    $rangeSettings = @(
        '{"id":"Lower","type":"number","default":-2,"minimum":-3}',
        '{"id":"Upper","type":"number","default":2,"maximum":3}',
        '{"id":"Free","type":"number","default":-1.25e2}',
        '{"id":"FloatValue","type":"number","default":1.5,"storage":"float","minimum":-2,"maximum":2}',
        '{"id":"Enabled","type":"boolean","default":false}'
    ) -join ','
    $rangeIni = "Lower=-3`nUpper=+3`nFree=-1.25e2`nFloatValue=+1.5`nEnabled=TrUe`n"
    $ranges = Invoke-GeneratorCase 'range-forms' (Schema $rangeSettings) $rangeIni
    Add-Check 'optional-bounds-signed-exponent-boolean' `
        ($ranges.Header.Contains('TEXT("-3")') -and $ranges.Header.Contains('TEXT("3")') -and
            $ranges.Header.Contains('ENumberStorage::Float') -and $ranges.Header.Contains('TEXT("false")')) `
        $ranges.Header

    $zero = Invoke-GeneratorCase 'exponent-zero' `
        (Schema '{"id":"Zero","type":"number","default":0}') "Zero=0e-9`n"
    Add-Check 'exponent-zero-valid' ($zero.Result.SettingCount -eq 1) '0e-9 accepted as exact zero.'

    $differentDefaults = Invoke-GeneratorCase 'schema-ini-defaults-independent' `
        (Schema '{"id":"Mode","type":"string","default":"schema # = \"quoted\" \\ path"}') `
        ('Mode="ini # = \"quoted\" \\ path" # outside comment' + "`n")
    Add-Check 'schema-default-decoded-and-ini-preserved' `
        ($differentDefaults.Header.Contains('TEXT("schema # = \"quoted\" \\ path")') -and
            -not $differentDefaults.Header.Contains('TEXT("\"schema') -and
            ([IO.File]::ReadAllText($differentDefaults.IniPath) -ceq
                [IO.File]::ReadAllText($differentDefaults.InputIniPath))) `
        $differentDefaults.Header

    $stringCases = Invoke-GeneratorCase 'string-values' `
        (Schema (@(
            '{"id":"Complex","type":"string","default":"a#b=c"}',
            '{"id":"Escaped","type":"string","default":"say \"hi\" at c:\\tmp"}',
            '{"id":"Empty","type":"string","default":""}'
        ) -join ',')) `
        ((@('Complex="a#b=c" # comment','Escaped="say \"hi\" at c:\\tmp"','Empty=""') -join "`n") + "`n")
    Add-Check 'strings-hash-equals-escapes-empty' `
        ($stringCases.Result.SettingCount -eq 3 -and
            $stringCases.Header.Contains('TEXT("a#b=c")') -and
            $stringCases.Header.Contains('TEXT("say \"hi\" at c:\\tmp")') -and
            $stringCases.Header.Contains('TEXT("")')) `
        $stringCases.Header

    $duplicateRuntimeSchema = Schema (@(
        '{"id":"One","runtimeName":"Shared","type":"boolean","default":true}',
        '{"id":"Two","runtimeName":"shared","type":"boolean","default":false}'
    ) -join ',')
    Assert-GeneratorRejected 'runtime-name-case-insensitive-unique' $duplicateRuntimeSchema `
        "One=true`nTwo=false`n" 'Duplicate runtimeName'
    Assert-GeneratorRejected 'schema-version-required' `
        '{"settings":[{"id":"Value","type":"number","default":1}]}' `
        "Value=1`n" 'schemaVersion|expected 2|property.*cannot be found'
    Assert-GeneratorRejected 'schema-version-two-required' `
        '{"schemaVersion":1,"settings":[{"id":"Value","type":"number","default":1}]}' `
        "Value=1`n" 'expected 2'

    foreach ($case in @(
        @{ Name='numeric-default-null'; Setting='{"id":"Value","type":"number","default":null}'; Ini="Value=1`n"; Pattern='JSON number' },
        @{ Name='numeric-default-boolean'; Setting='{"id":"Value","type":"number","default":true}'; Ini="Value=1`n"; Pattern='JSON number' },
        @{ Name='numeric-default-string'; Setting='{"id":"Value","type":"number","default":"1"}'; Ini="Value=1`n"; Pattern='JSON number' },
        @{ Name='number-missing-default'; Setting='{"id":"Value","type":"number"}'; Ini="Value=1`n"; Pattern='Missing property.*default' },
        @{ Name='boolean-default-number'; Setting='{"id":"Value","type":"boolean","default":1}'; Ini="Value=true`n"; Pattern='JSON true/false' },
        @{ Name='string-default-boolean'; Setting='{"id":"Value","type":"string","default":false}'; Ini='Value="x"'; Pattern='JSON string' },
        @{ Name='minimum-null'; Setting='{"id":"Value","type":"number","default":1,"minimum":null}'; Ini="Value=1`n"; Pattern='minimum must be a JSON number' },
        @{ Name='maximum-boolean'; Setting='{"id":"Value","type":"number","default":1,"maximum":true}'; Ini="Value=1`n"; Pattern='maximum must be a JSON number' },
        @{ Name='minimum-string'; Setting='{"id":"Value","type":"number","default":1,"minimum":"0"}'; Ini="Value=1`n"; Pattern='minimum must be a JSON number' },
        @{ Name='bad-range'; Setting='{"id":"Value","type":"number","default":1,"minimum":2,"maximum":1}'; Ini="Value=1`n"; Pattern='Invalid numeric range' },
        @{ Name='schema-default-outside-range'; Setting='{"id":"Value","type":"number","default":4,"maximum":3}'; Ini="Value=2`n"; Pattern='above maximum' },
        @{ Name='ini-outside-range'; Setting='{"id":"Value","type":"number","default":2,"maximum":3}'; Ini="Value=4`n"; Pattern='above maximum' },
        @{ Name='double-underflow'; Setting='{"id":"Value","type":"number","default":0}'; Ini="Value=1e-9999`n"; Pattern='underflows double' },
        @{ Name='double-overflow'; Setting='{"id":"Value","type":"number","default":0}'; Ini="Value=1e9999`n"; Pattern='Non-finite' },
        @{ Name='float-overflow'; Setting='{"id":"Value","type":"number","default":1,"storage":"float"}'; Ini="Value=3.5e38`n"; Pattern='not representable as float' },
        @{ Name='float-underflow'; Setting='{"id":"Value","type":"number","default":1,"storage":"float"}'; Ini="Value=1e-46`n"; Pattern='underflows float' },
        @{ Name='float-stored-below-min'; Setting='{"id":"Value","type":"number","default":1,"storage":"float","minimum":0.100000003}'; Ini="Value=0.100000003`n"; Pattern='Stored float is below minimum' },
        @{ Name='unknown-ini-key'; Setting='{"id":"Value","type":"number","default":1}'; Ini="Value=1`nOther=2`n"; Pattern='missing from schema' },
        @{ Name='missing-ini-key'; Setting='{"id":"Value","type":"number","default":1}'; Ini="Other=1`n"; Pattern='missing from default INI' },
        @{ Name='case-sensitive-key'; Setting='{"id":"Value","type":"number","default":1}'; Ini="value=1`n"; Pattern='missing from default INI' },
        @{ Name='duplicate-ini-key'; Setting='{"id":"Value","type":"number","default":1}'; Ini="Value=1`nValue=2`n"; Pattern='Duplicate default INI key' },
        @{ Name='semicolon-not-comment'; Setting='{"id":"Value","type":"boolean","default":true}'; Ini="Value=true ; false`n"; Pattern='Invalid boolean' },
        @{ Name='unquoted-string'; Setting='{"id":"Value","type":"string","default":"x"}'; Ini="Value=x`n"; Pattern='Invalid quoted string' },
        @{ Name='unterminated-string'; Setting='{"id":"Value","type":"string","default":"x"}'; Ini='Value="x'; Pattern='Unterminated quoted value' },
        @{ Name='bad-string-escape'; Setting='{"id":"Value","type":"string","default":"x"}'; Ini='Value="x\q"'; Pattern='Invalid string escape' },
        @{ Name='string-trailing-junk'; Setting='{"id":"Value","type":"string","default":"x"}'; Ini='Value="x" junk'; Pattern='Invalid quoted string|Unescaped quote' },
        @{ Name='number-trailing-junk'; Setting='{"id":"Value","type":"number","default":1}'; Ini="Value=1 junk`n"; Pattern='Invalid numeric value' }
    )) {
        Assert-GeneratorRejected $case.Name (Schema $case.Setting) $case.Ini $case.Pattern
    }

    $copy = Invoke-MergeCase 'merge-first-install' (Utf8Bytes "A=template`n") @() -NoCurrent
    Add-Check 'merge-first-install-exact-copy' `
        ([Linq.Enumerable]::SequenceEqual([byte[]]$copy.OutputBytes, [byte[]](Utf8Bytes "A=template`n")) -and
            [bool]$copy.Result.Created) `
        'First installation copied template bytes unchanged.'

    $currentText = "# player comment`r`nA=user # inline`r`nUnknown=keep`r`nloose unrelated line`r`n"
    $templateText = "# template`nA=template`nB=template-value`n"
    $merged = Invoke-MergeCase 'merge-preserve-and-add' (Utf8Bytes $templateText) (Utf8Bytes $currentText)
    $mergedText = [Text.UTF8Encoding]::new($false, $true).GetString($merged.OutputBytes)
    Add-Check 'merge-preserves-user-and-adds-template-value' `
        ($mergedText.Contains("# player comment`r`nA=user # inline`r`nUnknown=keep`r`nloose unrelated line`r`n") -and
            $mergedText.Contains("B=template-value`r`n") -and
            -not $mergedText.Contains('B=schema-default') -and
            @($merged.Result.AddedKeys).Count -eq 1 -and @($merged.Result.AddedKeys)[0] -ceq 'B') `
        $mergedText

    $retiredCurrent = "# remove me`nOld=custom`nKeep=user`n# remove me`n"
    $retired = Invoke-MergeCase 'merge-retired-explicit' `
        (Utf8Bytes "Keep=template`nNew=template`n") (Utf8Bytes $retiredCurrent) `
        -RetiredKeys @('Old') -RetiredComments @('# remove me')
    $retiredText = [Text.UTF8Encoding]::new($false, $true).GetString($retired.OutputBytes)
    Add-Check 'merge-removes-only-explicit-retired-content' `
        (-not $retiredText.Contains('Old=') -and -not $retiredText.Contains('# remove me') -and
            $retiredText.Contains('Keep=user') -and $retiredText.Contains('New=template') -and
            @($retired.Result.RemovedKeys).Count -eq 1 -and @($retired.Result.RemovedKeys)[0] -ceq 'Old') `
        $retiredText

    $unchangedCurrent = Utf8Bytes "A=user`n# exact bytes stay`n"
    $unchanged = Invoke-MergeCase 'merge-no-change-byte-exact' (Utf8Bytes "A=template`n") $unchangedCurrent
    Add-Check 'merge-no-change-byte-exact' `
        ([Linq.Enumerable]::SequenceEqual([byte[]]$unchanged.OutputBytes, [byte[]]$unchangedCurrent) -and
            -not [bool]$unchanged.Result.Changed) `
        'No-op merge retained current bytes exactly.'

    foreach ($encodingCase in @(
        @{ Name='utf8-bom-crlf'; Encoding=[Text.UTF8Encoding]::new($true); Newline="`r`n"; Prefix=[byte[]](0xEF,0xBB,0xBF) },
        @{ Name='utf16le-lf'; Encoding=[Text.UnicodeEncoding]::new($false,$true); Newline="`n"; Prefix=[byte[]](0xFF,0xFE) },
        @{ Name='utf16be-crlf'; Encoding=[Text.UnicodeEncoding]::new($true,$true); Newline="`r`n"; Prefix=[byte[]](0xFE,0xFF) }
    )) {
        $current = "A=user$($encodingCase.Newline)"
        $encoded = Invoke-MergeCase ("merge-" + $encodingCase.Name) `
            (Utf8Bytes "A=template`nB=added`n") (EncodedBytes $current $encodingCase.Encoding)
        $prefix = $encodingCase.Prefix
        $hasPrefix = $true
        for ($i=0; $i -lt $prefix.Length; $i++) {
            if ($encoded.OutputBytes[$i] -ne $prefix[$i]) { $hasPrefix = $false }
        }
        $decoded = $encodingCase.Encoding.GetString(
            $encoded.OutputBytes, $prefix.Length, $encoded.OutputBytes.Length - $prefix.Length)
        Add-Check ("merge-encoding-newline-" + $encodingCase.Name) `
            ($hasPrefix -and $decoded.Contains("A=user$($encodingCase.Newline)") -and
                $decoded.Contains("B=added$($encodingCase.Newline)") -and
                -not ($encodingCase.Newline -ceq "`n" -and $decoded.Contains("`r`n"))) `
            $decoded
    }

    $conflictCase = New-CasePath 'merge-output-conflicts'
    $conflictTemplate = Join-Path $conflictCase 'template.ini'
    $conflictCurrent = Join-Path $conflictCase 'current.ini'
    Write-TextFile $conflictTemplate "A=template`n"
    Write-TextFile $conflictCurrent "A=user`n"
    foreach ($conflict in @(
        @{ Name='template'; Output=$conflictTemplate },
        @{ Name='current'; Output=$conflictCurrent }
    )) {
        try {
            & $merger -TemplatePath $conflictTemplate -CurrentPath $conflictCurrent `
                -OutputPath $conflict.Output | Out-Null
        }
        catch {
            if ($_.Exception.Message -notmatch 'OutputPath must differ') { throw }
            Add-Check ("merge-rejects-output-conflict-" + $conflict.Name) $true $_.Exception.Message
            continue
        }
        throw "Expected output conflict rejection did not occur: $($conflict.Name)"
    }

    $existingOutput = Join-Path $conflictCase 'already-exists.ini'
    Write-TextFile $existingOutput 'sentinel'
    $existingOutputRejected = $false
    try {
        & $merger -TemplatePath $conflictTemplate -CurrentPath $conflictCurrent `
            -OutputPath $existingOutput | Out-Null
    }
    catch {
        if ($_.Exception.Message -notmatch 'Output already exists') { throw }
        $existingOutputRejected = $true
        Add-Check 'merge-rejects-existing-output' $true $_.Exception.Message
    }
    if (-not $existingOutputRejected) {
        throw 'Expected rejection for an existing merge output did not occur.'
    }
    if ([IO.File]::ReadAllText($existingOutput) -cne 'sentinel') {
        throw 'Existing merge output was modified.'
    }

    $testPassed = $true
    [pscustomobject][ordered]@{
        status = 'passed'
        checks = $checks.Count
        testRoot = $testRoot
    }
}
finally {
    if ($testPassed -and -not $KeepArtifacts -and (Test-Path -LiteralPath $testRoot)) {
        $resolvedTestParent = [IO.Path]::GetFullPath($testParent).TrimEnd(
            [IO.Path]::DirectorySeparatorChar, [IO.Path]::AltDirectorySeparatorChar) +
            [IO.Path]::DirectorySeparatorChar
        $resolvedTestRoot = (Resolve-Path -LiteralPath $testRoot).Path
        $testLeaf = [IO.Path]::GetFileName($resolvedTestRoot.TrimEnd(
                [IO.Path]::DirectorySeparatorChar, [IO.Path]::AltDirectorySeparatorChar))
        if (-not $resolvedTestRoot.StartsWith($resolvedTestParent,
                [StringComparison]::OrdinalIgnoreCase) -or
            $testLeaf -cnotmatch '^[0-9a-f]{32}$') {
            throw "Unsafe test cleanup target: $resolvedTestRoot"
        }
        Remove-Item -LiteralPath $resolvedTestRoot -Recurse -Force
    }
}
