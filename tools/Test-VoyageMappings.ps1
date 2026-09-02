param(
    [Parameter(Mandatory = $true)]
    [string]$MappingsPath,

    [Parameter(Mandatory = $true)]
    [string]$ManifestPath,

    [string]$ExpectedSteamBuildId,

    [string]$ExpectedExecutableSha256,

    [string[]]$RequiredNames = @(
        'EModuleResourceType',
        'VoyageInGameBoatWidget',
        'VoyageModuleSocketViewComponent',
        'Diesel'
    )
)

$ErrorActionPreference = 'Stop'

$usmapMagic0 = 0xC4
$usmapMagic1 = 0x30
$usmapHeaderSize = 16
$minimumPayloadLength = 1MB
$sha256Pattern = '^[0-9A-F]{64}$'

$mapping = (Resolve-Path -LiteralPath $MappingsPath).Path
$manifestFile = (Resolve-Path -LiteralPath $ManifestPath).Path
$manifest = Get-Content -Raw -LiteralPath $manifestFile | ConvertFrom-Json
$bytes = [IO.File]::ReadAllBytes($mapping)
$actualSha256 = (Get-FileHash -Algorithm SHA256 -LiteralPath $mapping).Hash

if ($bytes.Length -lt $usmapHeaderSize) {
    throw "Mappings file is shorter than the USMAP header: $mapping"
}
if ($bytes[0] -ne $usmapMagic0 -or $bytes[1] -ne $usmapMagic1) {
    throw "Mappings file has invalid USMAP magic: $mapping"
}

$compressedSize = [BitConverter]::ToUInt32($bytes, 8)
$decompressedSize = [BitConverter]::ToUInt32($bytes, 12)
if ($compressedSize -ne ($bytes.Length - $usmapHeaderSize)) {
    throw "USMAP payload length does not match its header: $mapping"
}
if ($decompressedSize -lt $minimumPayloadLength) {
    throw "USMAP payload is implausibly small ($decompressedSize bytes): $mapping"
}

if ([string]$manifest.kind -cne 'Voyage reflection mappings') {
    throw "Unexpected mappings manifest kind: $manifestFile"
}
if ([string]$manifest.mappingSha256 -notmatch $sha256Pattern -or
    [string]$manifest.mappingSha256 -cne $actualSha256) {
    throw "Mappings SHA-256 does not match its manifest: $mapping"
}
if ([long]$manifest.mappingLength -ne $bytes.Length) {
    throw "Mappings length does not match its manifest: $mapping"
}
if ($ExpectedSteamBuildId -and
    [string]$manifest.steamBuildId -cne $ExpectedSteamBuildId) {
    throw "Mappings Steam build ID does not match the requested build."
}
if ($ExpectedExecutableSha256 -and
    ([string]$manifest.executableSha256 -notmatch $sha256Pattern -or
     [string]$manifest.executableSha256 -cne $ExpectedExecutableSha256)) {
    throw "Mappings executable SHA-256 does not match the requested build."
}

if ($bytes[3] -ne 0) {
    throw 'Required-name validation currently supports only an uncompressed USMAP payload.'
}
$payloadText = [Text.Encoding]::UTF8.GetString(
    $bytes,
    $usmapHeaderSize,
    $bytes.Length - $usmapHeaderSize)
foreach ($requiredName in $RequiredNames) {
    if (-not $payloadText.Contains($requiredName)) {
        throw "Required reflected name is absent from mappings: $requiredName"
    }
}

[pscustomobject]@{
    mappingsPath = $mapping
    sha256 = $actualSha256
    length = $bytes.Length
    usmapVersion = [int]$bytes[2]
    compressionMethod = [int]$bytes[3]
    decompressedPayloadLength = [long]$decompressedSize
    steamBuildId = [string]$manifest.steamBuildId
    executableSha256 = [string]$manifest.executableSha256
    requiredNames = $RequiredNames
}
