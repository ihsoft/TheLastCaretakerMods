Set-StrictMode -Version Latest

$script:Invariant = [Globalization.CultureInfo]::InvariantCulture

function Assert-ObjectProperties {
    param($Value, [string[]]$Allowed, [string[]]$Required, [string]$Label)
    $names = @($Value.PSObject.Properties.Name)
    foreach ($name in $names) {
        if ($Allowed -cnotcontains $name) { throw "Unknown property in ${Label}: $name" }
    }
    foreach ($name in $Required) {
        if ($names -cnotcontains $name) { throw "Missing property in ${Label}: $name" }
    }
}

function Assert-Identifier([string]$Value, [string]$Label) {
    if ($Value -cnotmatch '^[A-Za-z_][A-Za-z0-9_]*$') { throw "Invalid ${Label}: $Value" }
}

function ConvertTo-CppString([string]$Value) {
    return $Value.Replace('\', '\\').Replace('"', '\"').Replace("`r", '\r').Replace("`n", '\n')
}

function Format-InvariantNumber([double]$Value) {
    return $Value.ToString('R', $script:Invariant)
}

function Test-NumericJsonValue($Value) {
    return $Value -is [byte] -or $Value -is [sbyte] -or $Value -is [int16] -or
        $Value -is [uint16] -or $Value -is [int32] -or $Value -is [uint32] -or
        $Value -is [int64] -or $Value -is [uint64] -or $Value -is [single] -or
        $Value -is [double] -or $Value -is [decimal]
}

function Assert-NoUnderflowingJsonNumbers([string]$Text) {
    $quoted = $false
    $escaped = $false
    for ($index = 0; $index -lt $Text.Length; $index++) {
        $character = $Text[$index]
        if ($quoted) {
            if ($escaped) { $escaped = $false }
            elseif ($character -eq '\') { $escaped = $true }
            elseif ($character -eq '"') { $quoted = $false }
            continue
        }
        if ($character -eq '"') { $quoted = $true; continue }
        if ($character -notmatch '[-0-9]') { continue }
        $match = [regex]::Match($Text.Substring($index), '^-?(?:0|[1-9][0-9]*)(?:\.[0-9]+)?(?:[eE][+-]?[0-9]+)?')
        if (-not $match.Success) { continue }
        $token = $match.Value
        $value = 0.0
        if ([double]::TryParse($token, [Globalization.NumberStyles]::Float,
                $script:Invariant, [ref]$value)) {
            $significand = ($token -split '[eE]', 2)[0]
            if ($value -eq 0.0 -and $significand -match '[1-9]') {
                throw "Numeric value underflows double storage in settings schema: $token"
            }
        }
        $index += $token.Length - 1
    }
}

function Find-IniComment([string]$Text, [int]$Start) {
    $quoted = $false
    $escaped = $false
    for ($i = $Start; $i -lt $Text.Length; $i++) {
        $ch = $Text[$i]
        if ($escaped) { $escaped = $false; continue }
        if ($quoted -and $ch -eq '\') { $escaped = $true; continue }
        if ($ch -eq '"') { $quoted = -not $quoted; continue }
        if (-not $quoted -and $ch -eq '#') { return $i }
    }
    if ($quoted -or $escaped) { return -2 }
    return -1
}

function ConvertFrom-QuotedIniString([string]$Text, [string]$Label) {
    if ($Text.Length -lt 2 -or $Text[0] -ne '"' -or $Text[$Text.Length - 1] -ne '"') {
        throw "Invalid quoted string for ${Label}: $Text"
    }
    $result = [Text.StringBuilder]::new()
    for ($i = 1; $i -lt $Text.Length - 1; $i++) {
        $ch = $Text[$i]
        if ($ch -eq '\') {
            $i++
            if ($i -ge $Text.Length - 1 -or ($Text[$i] -ne '\' -and $Text[$i] -ne '"')) {
                throw "Invalid string escape for ${Label}: $Text"
            }
            [void]$result.Append($Text[$i])
        } elseif ($ch -eq '"') {
            throw "Unescaped quote in ${Label}: $Text"
        } else {
            [void]$result.Append($ch)
        }
    }
    return $result.ToString()
}

function ConvertFrom-SettingText {
    param([string]$Text, [string]$Type, [string]$Storage = 'double', $Minimum, $Maximum,
        [string]$Label = 'setting')
    switch ($Type) {
        'number' {
            if ($Text -cnotmatch '^[+-]?(?:(?:[0-9]+(?:\.[0-9]*)?)|(?:\.[0-9]+))(?:[eE][+-]?[0-9]+)?$') {
                throw "Invalid numeric value for ${Label}: $Text"
            }
            $number = 0.0
            if (-not [double]::TryParse($Text, [Globalization.NumberStyles]::Float,
                    $script:Invariant, [ref]$number) -or [double]::IsNaN($number) -or
                [double]::IsInfinity($number)) {
                throw "Non-finite numeric value for ${Label}: $Text"
            }
            $significand = ($Text -split '[eE]', 2)[0]
            if ($number -eq 0.0 -and $significand -match '[1-9]') {
                throw "Numeric value underflows double storage for ${Label}: $Text"
            }
            if ($Storage -cne 'float' -and $null -ne $Minimum -and $number -lt [double]$Minimum) {
                throw "Numeric value below minimum for ${Label}: $Text"
            }
            if ($Storage -cne 'float' -and $null -ne $Maximum -and $number -gt [double]$Maximum) {
                throw "Numeric value above maximum for ${Label}: $Text"
            }
            if ($Storage -ceq 'float') {
                if ($number -lt -[single]::MaxValue -or $number -gt [single]::MaxValue) {
                    throw "Numeric value is not representable as float for ${Label}: $Text"
                }
                $stored = [single]$number
                if ($number -ne 0.0 -and $stored -eq 0.0) {
                    throw "Numeric value underflows float storage for ${Label}: $Text"
                }
                if ($null -ne $Minimum -and [double]$stored -lt [double]$Minimum) {
                    throw "Stored float is below minimum for ${Label}: $Text"
                }
                if ($null -ne $Maximum -and [double]$stored -gt [double]$Maximum) {
                    throw "Stored float is above maximum for ${Label}: $Text"
                }
                return $stored
            }
            return $number
        }
        'boolean' {
            if ($Text -ieq 'true') { return $true }
            if ($Text -ieq 'false') { return $false }
            throw "Invalid boolean value for ${Label}: $Text"
        }
        'string' { return ConvertFrom-QuotedIniString $Text $Label }
        default { throw "Unsupported setting type: $Type" }
    }
}

function Read-StrictIni {
    param([string]$Path)
    $values = [Collections.Generic.Dictionary[string,object]]::new([StringComparer]::Ordinal)
    $lines = [IO.File]::ReadAllLines((Resolve-Path -LiteralPath $Path).Path)
    for ($index = 0; $index -lt $lines.Count; $index++) {
        $line = $lines[$index]
        $trimmed = $line.Trim()
        if (-not $trimmed -or $trimmed.StartsWith('#')) { continue }
        $separator = $line.IndexOf('=')
        if ($separator -lt 0) { throw "Malformed default INI line $($index + 1): $line" }
        $key = $line.Substring(0, $separator).Trim()
        if ($key -cnotmatch '^[A-Za-z][A-Za-z0-9]*$') {
            throw "Invalid default INI key on line $($index + 1): $key"
        }
        $comment = Find-IniComment $line ($separator + 1)
        if ($comment -eq -2) { throw "Unterminated quoted value on line $($index + 1): $key" }
        $end = if ($comment -ge 0) { $comment } else { $line.Length }
        $raw = $line.Substring($separator + 1, $end - $separator - 1).Trim()
        if (-not $raw) { throw "Missing default INI value on line $($index + 1): $key" }
        if ($values.ContainsKey($key)) { throw "Duplicate default INI key: $key" }
        $values.Add($key, [pscustomobject]@{ Raw=$raw; Line=$index + 1 })
    }
    if ($values.Count -eq 0) { throw 'Default INI has no settings.' }
    return $values
}

function Read-VoyageModSettingsSchema {
    param([Parameter(Mandatory=$true)][string]$SchemaPath,
        [Parameter(Mandatory=$true)][string]$DefaultIniPath)
    $schemaText = Get-Content -LiteralPath (Resolve-Path -LiteralPath $SchemaPath).Path -Raw
    Assert-NoUnderflowingJsonNumbers $schemaText
    $schema = $schemaText | ConvertFrom-Json
    if ($schema.schemaVersion -ne 2) { throw 'Unsupported settings schema version; expected 2.' }
    $grouped = $schema.PSObject.Properties.Name -ccontains 'groups'
    if ($grouped) {
        Assert-ObjectProperties $schema @('schemaVersion','groups') @('schemaVersion','groups') 'settings schema'
        if (@($schema.groups).Count -eq 0) { throw 'Settings schema has no groups.' }
        foreach ($group in @($schema.groups)) {
            Assert-ObjectProperties $group @('id','comments','settings') @('id','settings') 'settings group'
            Assert-Identifier ([string]$group.id).Replace('-', '_') 'group id'
        }
        $schemaSettings = @($schema.groups | ForEach-Object { $_.settings })
    } else {
        Assert-ObjectProperties $schema @('schemaVersion','settings') @('schemaVersion','settings') 'settings schema'
        $schemaSettings = @($schema.settings)
    }
    if ($schemaSettings.Count -eq 0) { throw 'Settings schema has no settings.' }
    $ini = Read-StrictIni $DefaultIniPath
    $ids = [Collections.Generic.HashSet[string]]::new([StringComparer]::Ordinal)
    $keys = [Collections.Generic.HashSet[string]]::new([StringComparer]::Ordinal)
    $runtimeNames = [Collections.Generic.HashSet[string]]::new([StringComparer]::OrdinalIgnoreCase)
    $result = [Collections.Generic.List[object]]::new()
    foreach ($item in $schemaSettings) {
        Assert-ObjectProperties $item @('id','runtimeName','field','key','type','default','minimum','maximum','storage') `
            @('type','default') 'setting'
        $id = if ($item.PSObject.Properties.Name -ccontains 'id') { [string]$item.id } `
            elseif ($item.PSObject.Properties.Name -ccontains 'key') { [string]$item.key } `
            elseif ($item.PSObject.Properties.Name -ccontains 'runtimeName') { [string]$item.runtimeName } `
            else { throw 'Setting requires at least one of id, key, or runtimeName.' }
        $key = if ($item.PSObject.Properties.Name -ccontains 'key') { [string]$item.key } else { $id }
        $runtimeName = if ($item.PSObject.Properties.Name -ccontains 'runtimeName') { [string]$item.runtimeName } else { $id }
        $field = if ($item.PSObject.Properties.Name -ccontains 'field') { [string]$item.field } else { $id }
        $type = [string]$item.type
        $storage = if ($item.PSObject.Properties.Name -ccontains 'storage') { [string]$item.storage } else { 'double' }
        Assert-Identifier $id 'setting id'
        if ($id -cin @('Settings','FSetting','ESettingType','ENumberStorage')) {
            throw "Reserved setting id: $id"
        }
        if ($key -cnotmatch '^[A-Za-z][A-Za-z0-9]*$') { throw "Invalid setting key: $key" }
        if (-not $ids.Add($id)) { throw "Duplicate setting id: $id" }
        if (-not $keys.Add($key)) { throw "Duplicate setting key: $key" }
        if ($field -cnotmatch '^[A-Za-z_][A-Za-z0-9_]*(::[A-Za-z_][A-Za-z0-9_]*)?$') {
            throw "Invalid field expression: $field"
        }
        if ($field -cnotmatch '::') {
            Assert-Identifier $runtimeName 'runtimeName'
            if (-not $runtimeNames.Add($runtimeName)) { throw "Duplicate runtimeName: $runtimeName" }
        } elseif ($item.PSObject.Properties.Name -ccontains 'runtimeName') {
            throw "External field must not declare runtimeName: $id"
        }
        if ($type -cnotin @('number','boolean','string')) { throw "Unsupported setting type: $id" }
        if ($type -cne 'number' -and ($item.PSObject.Properties.Name -ccontains 'minimum' -or
                $item.PSObject.Properties.Name -ccontains 'maximum' -or
                $item.PSObject.Properties.Name -ccontains 'storage')) {
            throw "Non-number setting has numeric constraints: $id"
        }
        if ($type -ceq 'number' -and $storage -cnotin @('double','float')) {
            throw "Invalid numeric storage for ${id}: $storage"
        }
        if ($type -ceq 'number' -and -not (Test-NumericJsonValue $item.default)) {
            throw "Numeric schema default must be a JSON number: $id"
        }
        foreach ($boundName in @('minimum','maximum')) {
            if ($item.PSObject.Properties.Name -ccontains $boundName -and
                -not (Test-NumericJsonValue $item.$boundName)) {
                throw "Numeric $boundName must be a JSON number: $id"
            }
        }
        $minimum = if ($item.PSObject.Properties.Name -ccontains 'minimum') { [double]$item.minimum } else { $null }
        $maximum = if ($item.PSObject.Properties.Name -ccontains 'maximum') { [double]$item.maximum } else { $null }
        foreach ($bound in @($minimum,$maximum)) {
            if ($null -ne $bound -and ([double]::IsNaN($bound) -or [double]::IsInfinity($bound))) {
                throw "Non-finite numeric range: $id"
            }
        }
        if ($null -ne $minimum -and $null -ne $maximum -and $minimum -gt $maximum) {
            throw "Invalid numeric range: $id"
        }
        $defaultText = switch ($type) {
            'number' { Format-InvariantNumber ([double]$item.default) }
            'boolean' {
                if ($item.default -isnot [bool]) { throw "Boolean schema default must be JSON true/false: $id" }
                if ([bool]$item.default) { 'true' } else { 'false' }
            }
            'string' {
                if ($item.default -isnot [string]) { throw "String schema default must be a JSON string: $id" }
                if ([string]$item.default -match '[\r\n]') { throw "String schema default must be single-line: $id" }
                '"' + ([string]$item.default).Replace('\','\\').Replace('"','\"') + '"'
            }
        }
        $defaultValue = ConvertFrom-SettingText $defaultText $type $storage $minimum $maximum "schema default $id"
        if (-not $ini.ContainsKey($key)) { throw "Setting is missing from default INI: $key" }
        $iniValue = ConvertFrom-SettingText ([string]$ini[$key].Raw) $type $storage $minimum $maximum "default INI $key"
        $result.Add([pscustomobject]@{
            Id=$id; Key=$key; RuntimeName=$runtimeName; Field=$field; Type=$type; Storage=$storage
            DefaultText=$(if ($type -ceq 'string') { [string]$defaultValue } else { $defaultText }); DefaultValue=$defaultValue; IniText=[string]$ini[$key].Raw; IniValue=$iniValue
            Minimum=$minimum; Maximum=$maximum
        })
    }
    foreach ($key in $ini.Keys) {
        if (-not $keys.Contains($key)) { throw "Default INI key is missing from schema: $key" }
    }
    return @($result)
}

function New-SettingsHeader {
    param([object[]]$Settings, [string]$GeneratedBy)
    $lines = [Collections.Generic.List[string]]::new()
    $lines.Add("// Generated by $GeneratedBy. Do not edit.")
    foreach ($s in $Settings) {
        if ($s.Field -cnotmatch '::') { $lines.Add(('inline const FName {0}(TEXT("{1}"));' -f $s.Id,(ConvertTo-CppString $s.RuntimeName))) }
    }
    $lines.Add('inline const FSetting Settings[] = {')
    foreach ($s in $Settings) {
        $field = $s.Field
        $type = 'ESettingType::' + ($s.Type.Substring(0,1).ToUpperInvariant() + $s.Type.Substring(1))
        $storage = if ($s.Type -ceq 'number' -and $s.Storage -ceq 'float') { 'ENumberStorage::Float' } else { 'ENumberStorage::Double' }
        $min = if ($null -ne $s.Minimum) { 'TEXT("' + (ConvertTo-CppString (Format-InvariantNumber $s.Minimum)) + '")' } else { 'nullptr' }
        $max = if ($null -ne $s.Maximum) { 'TEXT("' + (ConvertTo-CppString (Format-InvariantNumber $s.Maximum)) + '")' } else { 'nullptr' }
        $createMember = if ($s.Field -cnotmatch '::') { 'true' } else { 'false' }
        $lines.Add(('    {{TEXT("{0}"), {1}, {2}, TEXT("{3}"), {4}, {5}, {6}, {7}}},' -f `
            (ConvertTo-CppString $s.Key),$field,$type,(ConvertTo-CppString $s.DefaultText),$min,$max,$storage,$createMember))
    }
    $lines.Add('};')
    return @($lines)
}

function Invoke-NewVoyageModSettings {
    [CmdletBinding()]
    param([Parameter(Mandatory=$true)][string]$SchemaPath,
        [Parameter(Mandatory=$true)][string]$DefaultIniPath,
        [Parameter(Mandatory=$true)][string]$HeaderPath,
        [Parameter(Mandatory=$true)][string]$IniPath,
        [Parameter(Mandatory=$true)][string]$GeneratedBy)
    $settings = @(Read-VoyageModSettingsSchema $SchemaPath $DefaultIniPath)
    $schemaResolved = (Resolve-Path -LiteralPath $SchemaPath).Path
    $iniResolved = (Resolve-Path -LiteralPath $DefaultIniPath).Path
    $headerFull = [IO.Path]::GetFullPath($HeaderPath)
    $iniFull = [IO.Path]::GetFullPath($IniPath)
    foreach ($target in @($headerFull,$iniFull)) {
        foreach ($input in @($schemaResolved,$iniResolved)) {
            if ($target.Equals($input,[StringComparison]::OrdinalIgnoreCase)) { throw 'Generated outputs must not overwrite inputs.' }
        }
    }
    if ($headerFull.Equals($iniFull,[StringComparison]::OrdinalIgnoreCase)) { throw 'HeaderPath and IniPath must differ.' }
    foreach ($path in @($headerFull,$iniFull)) { [void][IO.Directory]::CreateDirectory([IO.Path]::GetDirectoryName($path)) }
    $utf8 = [Text.UTF8Encoding]::new($false)
    [IO.File]::WriteAllLines($headerFull,(New-SettingsHeader $settings $GeneratedBy),$utf8)
    [IO.File]::WriteAllBytes($iniFull,[IO.File]::ReadAllBytes($iniResolved))
    [pscustomobject]@{ SettingCount=$settings.Count; HeaderPath=$headerFull; IniPath=$iniFull }
}

Export-ModuleMember -Function Invoke-NewVoyageModSettings,Read-VoyageModSettingsSchema
