param(
    [Parameter(Mandatory = $true)][string]$Container,
    [Parameter(Mandatory = $true)][string]$Archive,
    [string]$ContentPluginDescriptor,
    [switch]$LogicMods,
    [string[]]$AdditionalFile = @(),
    [string[]]$AdditionalDestination = @(),
    [switch]$RootLayout,
    [switch]$PaksRoot
)

$ErrorActionPreference = 'Stop'
Add-Type -AssemblyName System.IO.Compression
Add-Type -AssemblyName System.IO.Compression.FileSystem

function Assert-SafeArchivePath([string]$Path) {
    if ([string]::IsNullOrWhiteSpace($Path) -or $Path.StartsWith('/') -or
        $Path.Contains('\') -or $Path.Contains(':') -or
        $Path.Contains('//') -or $Path.EndsWith('/') -or
        $Path -match '(^|/)\.\.?(/|$)') {
        throw "Unsafe archive destination: $Path"
    }
}

$utoc = (Resolve-Path -LiteralPath $Container).Path
if ([IO.Path]::GetExtension($utoc) -cne '.utoc') { throw 'Container must be a .utoc file.' }
$containerFiles = @(@('pak', 'ucas', 'utoc') | ForEach-Object {
    $file = [IO.Path]::ChangeExtension($utoc, ".$($_)")
    if (-not (Test-Path -LiteralPath $file -PathType Leaf)) { throw "Missing container file: $file" }
    $file
})
if ($AdditionalDestination.Count -ne 0 -and $AdditionalDestination.Count -ne $AdditionalFile.Count) {
    throw 'AdditionalDestination must have one entry per AdditionalFile.'
}
$hasPlugin = -not [string]::IsNullOrWhiteSpace($ContentPluginDescriptor)
$hasExtras = $AdditionalFile.Count -gt 0
if ($PaksRoot -and ($LogicMods -or $hasPlugin -or $RootLayout)) {
    throw '-PaksRoot applies only to a plain PAK release.'
}
$layout = if ($RootLayout -or $hasPlugin -or ($LogicMods -and $hasExtras)) { 'root' }
    elseif ($LogicMods) { 'logicmods' }
    elseif ($hasExtras -or $PaksRoot) { 'pakalt' }
    else { 'pak' }
$prefix = switch ($layout) {
    'root' { if ($LogicMods) { 'Voyage/Content/Paks/LogicMods/' } else { 'Voyage/Content/Paks/' } }
    'logicmods' { 'LogicMods/' }
    default { '' }
}
$files = New-Object 'System.Collections.Generic.List[object]'
foreach ($file in $containerFiles) {
    $files.Add([pscustomobject]@{ source = $file; entry = ($prefix + [IO.Path]::GetFileName($file)) })
}
for ($i = 0; $i -lt $AdditionalFile.Count; $i++) {
    $source = (Resolve-Path -LiteralPath $AdditionalFile[$i]).Path
    $destination = if ($AdditionalDestination.Count -gt 0) { $AdditionalDestination[$i] }
        elseif ($layout -eq 'root') { 'Voyage/Content/Paks/' + [IO.Path]::GetFileName($source) }
        else { [IO.Path]::GetFileName($source) }
    $files.Add([pscustomobject]@{ source = $source; entry = $destination })
}
if ($hasPlugin) {
    $descriptor = (Resolve-Path -LiteralPath $ContentPluginDescriptor).Path
    if ([IO.Path]::GetExtension($descriptor) -cne '.uplugin') { throw 'Descriptor must be a .uplugin file.' }
    $pluginName = [IO.Path]::GetFileNameWithoutExtension($descriptor)
    if ($pluginName -cnotmatch '^[0-9A-Za-z_]+$') { throw 'Unsafe plugin identity.' }
    $files.Add([pscustomobject]@{
        source = $descriptor
        entry = "Voyage/Mods/$pluginName/$pluginName.uplugin"
    })
}
$entries = @($files | ForEach-Object { $_.entry })
foreach ($entry in $entries) { Assert-SafeArchivePath $entry }
if ($layout -eq 'root' -and @($entries | Where-Object { -not $_.StartsWith('Voyage/', [StringComparison]::Ordinal) }).Count) {
    throw 'Every game-root archive destination must start with Voyage/.'
}
if ($layout -eq 'pakalt' -and @($entries | Where-Object { $_.StartsWith('Voyage/', [StringComparison]::Ordinal) }).Count) {
    throw 'A Paks-root override destination must be relative to Paks, without Voyage/.'
}
if (@($entries | Select-Object -Unique).Count -ne $entries.Count) { throw 'Duplicate archive destination.' }
$archivePath = [IO.Path]::GetFullPath($Archive)
if (Test-Path -LiteralPath $archivePath) { throw "Archive already exists: $archivePath" }
$parent = [IO.Path]::GetDirectoryName($archivePath)
if (-not (Test-Path -LiteralPath $parent -PathType Container)) { throw "Archive directory is missing: $parent" }

$override = $null
if ($layout -eq 'pakalt') {
    $override = @($files | ForEach-Object {
        [ordered]@{ type = 'copy'; source = $_.entry; destination = $_.entry }
    }) + @([ordered]@{
        type = 'setmodtype'
        value = 'thelastcaretaker-pakalt'
    })
}
$stream = [IO.File]::Open($archivePath, [IO.FileMode]::CreateNew, [IO.FileAccess]::ReadWrite, [IO.FileShare]::None)
try {
    $zip = [IO.Compression.ZipArchive]::new($stream, [IO.Compression.ZipArchiveMode]::Create, $false)
    try {
        if ($layout -eq 'logicmods') {
            $null = $zip.CreateEntry('LogicMods/')
        }
        foreach ($file in $files) {
            [IO.Compression.ZipFileExtensions]::CreateEntryFromFile(
                $zip, $file.source, $file.entry, [IO.Compression.CompressionLevel]::Optimal) | Out-Null
        }
        if ($null -ne $override) {
            $json = ($override | ConvertTo-Json -Depth 5) + [Environment]::NewLine
            $overrideEntry = $zip.CreateEntry('vortex_override_instructions.json')
            $writer = New-Object IO.StreamWriter($overrideEntry.Open(), (New-Object Text.UTF8Encoding($false)))
            try { $writer.Write($json) } finally { $writer.Dispose() }
        }
    } finally { $zip.Dispose() }
} finally { $stream.Dispose() }

$readback = [IO.Compression.ZipFile]::OpenRead($archivePath)
try {
    $expected = @($entries)
    if ($layout -eq 'logicmods') { $expected += 'LogicMods/' }
    if ($null -ne $override) { $expected += 'vortex_override_instructions.json' }
    $actual = @($readback.Entries | ForEach-Object { $_.FullName })
    if (Compare-Object $expected $actual) { throw 'Archive entry list does not match the selected layout.' }
    foreach ($file in $files) {
        $entry = @($readback.Entries | Where-Object { $_.FullName -ceq $file.entry })[0]
        $hasher = [Security.Cryptography.SHA256]::Create()
        $entryStream = $entry.Open()
        try { $hash = [BitConverter]::ToString($hasher.ComputeHash($entryStream)).Replace('-', '') }
        finally { $entryStream.Dispose(); $hasher.Dispose() }
        if ($hash -cne (Get-FileHash -LiteralPath $file.source -Algorithm SHA256).Hash) {
            throw "Archive readback mismatch: $($file.entry)"
        }
    }
    if ($null -ne $override) {
        $entry = @($readback.Entries | Where-Object { $_.FullName -ceq 'vortex_override_instructions.json' })[0]
        $reader = New-Object IO.StreamReader($entry.Open())
        try { $instructions = ConvertFrom-Json -InputObject $reader.ReadToEnd() } finally { $reader.Dispose() }
        if ($instructions.Count -ne ($files.Count + 1)) {
            throw "Override instruction count mismatch: expected $($files.Count + 1), got $($instructions.Count)."
        }
        for ($i = 0; $i -lt $files.Count; $i++) {
            if ($instructions[$i].type -cne 'copy' -or
                $instructions[$i].source -cne $files[$i].entry -or
                $instructions[$i].destination -cne $files[$i].entry) {
                throw 'Override copy mapping mismatch.'
            }
        }
    }
} catch {
    $readback.Dispose()
    Remove-Item -LiteralPath $archivePath -Force
    throw
} finally { $readback.Dispose() }

[pscustomobject][ordered]@{
    status = 'verified'
    archivePath = $archivePath
    layout = $layout
    entries = $expected
    overrideModType = $(if ($null -ne $override) { $override[-1].value } else { $null })
}
