[CmdletBinding()]
param(
    [Parameter(Mandatory=$true)][string]$GameRoot,
    [Parameter(Mandatory=$true)][string]$ExpectedExecutableSha256,
    [Parameter(Mandatory=$true)][string]$UnrealPak,
    [Parameter(Mandatory=$true)][string]$OutputRoot
)
$ErrorActionPreference = 'Stop'
Set-StrictMode -Version Latest
$OutputRoot = [IO.Path]::GetFullPath($OutputRoot)

# This is deliberately limited to the reviewed Voyage Steam 25191271 binary.
# No key bytes, game registry, or CryptoKeys.json are source/release inputs.
$reviewedSha = '747DC2553F7E68D8EA7ED0B2E0CAC6D08943EA3F50DD6ED822E9293E0B45F58B'
$callbackRva = 0x056AD1F0
$expectedIndexOffset = 54414670L
$expectedIndexSize = 66304L
$exe = Join-Path $GameRoot 'Voyage/Binaries/Win64/VoyageSteam-Win64-Shipping.exe'
$pak = Join-Path $GameRoot 'Voyage/Content/Paks/pakchunk0-Windows.pak'
if ($ExpectedExecutableSha256 -cne $reviewedSha -or
    (Get-FileHash -LiteralPath $exe -Algorithm SHA256).Hash -cne $reviewedSha) {
    throw 'Unreviewed Voyage executable: stock registry extraction refused.'
}
if (-not (Test-Path -LiteralPath $pak -PathType Leaf) -or
    -not (Test-Path -LiteralPath $UnrealPak -PathType Leaf)) {
    throw 'Stock PAK or UnrealPak is missing.'
}
$exeBytes = [IO.File]::ReadAllBytes($exe)
if ([BitConverter]::ToUInt16($exeBytes,0) -ne 0x5A4D) { throw 'Invalid PE DOS header.' }
$pe = [BitConverter]::ToInt32($exeBytes,0x3C)
if ([BitConverter]::ToUInt32($exeBytes,$pe) -ne 0x00004550) { throw 'Invalid PE signature.' }
$sections = [BitConverter]::ToUInt16($exeBytes,$pe+6)
$sectionTable = $pe + 24 + [BitConverter]::ToUInt16($exeBytes,$pe+20)
$callbackOffset = -1
for ($section=0; $section -lt $sections; $section++) {
    $at = $sectionTable + 40*$section
    $virtualSize = [BitConverter]::ToUInt32($exeBytes,$at+8)
    $virtualAddress = [BitConverter]::ToUInt32($exeBytes,$at+12)
    $rawSize = [BitConverter]::ToUInt32($exeBytes,$at+16)
    if ($callbackRva -ge $virtualAddress -and $callbackRva+0x56 -le $virtualAddress+[Math]::Min($virtualSize,$rawSize)) {
        $callbackOffset = [int]([BitConverter]::ToUInt32($exeBytes,$at+20) + $callbackRva - $virtualAddress)
        break
    }
}
if ($callbackOffset -lt 0) { throw 'Reviewed key callback RVA is not mapped to PE raw bytes.' }
if ($exeBytes[$callbackOffset] -ne 0x40) { throw 'Reviewed key callback entry prefix changed.' }
$callbackOffset++
$prologue = @(0x55,0x48,0x8B,0xEC,0x48,0x83,0xEC,0x30)
for ($i=0; $i -lt $prologue.Count; $i++) {
    if ($exeBytes[$callbackOffset+$i] -ne $prologue[$i]) { throw 'Reviewed key callback prologue changed.' }
}
$key = New-Object byte[] 32
$movOffsets = @(8,15,22,29,40,47,54,61)
for ($i=0; $i -lt 8; $i++) {
    $at = $callbackOffset + $movOffsets[$i]
    if ($exeBytes[$at] -ne 0xC7 -or $exeBytes[$at+1] -ne 0x45 -or
        $exeBytes[$at+2] -ne (0xD0 + 4*$i)) {
        throw 'Reviewed key callback instruction layout changed.'
    }
    [Array]::Copy($exeBytes,$at+3,$key,4*$i,4)
}
foreach ($check in @(@(36,0x0F),@(37,0x10),@(38,0x45),@(39,0xD0),
                    @(68,0x0F),@(69,0x10),@(70,0x4D),@(71,0xE0),
                    @(72,0x0F),@(73,0x11),@(74,0x01),
                    @(75,0x0F),@(76,0x11),@(77,0x49),@(78,0x10),
                    @(84,0xC3))) {
    if ($exeBytes[$callbackOffset+$check[0]] -ne $check[1]) {
        throw 'Reviewed key callback body changed.'
    }
}

# FPakInfo version 12: footer size 221; Guid, encrypted flag, magic, version,
# index offset/size, then the SHA1 of the *decrypted* complete index.
$footerSize = 221
$stream = [IO.File]::OpenRead($pak)
try {
    if ($stream.Length -lt $footerSize) { throw 'Stock PAK is too short.' }
    $footer = New-Object byte[] $footerSize
    $stream.Position = $stream.Length-$footerSize
    if ($stream.Read($footer,0,$footer.Length) -ne $footer.Length) { throw 'Cannot read PAK footer.' }
    if ($footer[16] -ne 1 -or [BitConverter]::ToUInt32($footer,17) -ne 0x5A6F12E1 -or
        [BitConverter]::ToInt32($footer,21) -ne 12 -or
        [BitConverter]::ToInt64($footer,25) -ne $expectedIndexOffset -or
        [BitConverter]::ToInt64($footer,33) -ne $expectedIndexSize -or
        (@($footer[0..15] | Where-Object { $_ -ne 0 }).Count -ne 0)) {
        throw 'Stock PAK footer does not match the reviewed encrypted-index layout.'
    }
    $encrypted = New-Object byte[] ([int]$expectedIndexSize)
    $stream.Position = $expectedIndexOffset
    if ($stream.Read($encrypted,0,$encrypted.Length) -ne $encrypted.Length) { throw 'Cannot read complete PAK index.' }
} finally { $stream.Dispose() }
$aes = [Security.Cryptography.Aes]::Create()
try {
    $aes.KeySize = 256
    $aes.Mode = [Security.Cryptography.CipherMode]::ECB
    $aes.Padding = [Security.Cryptography.PaddingMode]::None
    $aes.Key = $key
    $decryptor = $aes.CreateDecryptor()
    try { $plain = $decryptor.TransformFinalBlock($encrypted,0,$encrypted.Length) }
    finally { $decryptor.Dispose() }
} finally { $aes.Dispose() }
$sha1 = [Security.Cryptography.SHA1]::Create()
try { $actualHash = $sha1.ComputeHash($plain) }
finally { $sha1.Dispose() }
for ($i=0; $i -lt 20; $i++) {
    if ($actualHash[$i] -ne $footer[41+$i]) { throw 'PAK index SHA1 mismatch: key extraction refused.' }
}

$null = New-Item -ItemType Directory -Path $OutputRoot -Force
$keyPath = Join-Path $OutputRoot 'CryptoKeys.json'
$json = @{EncryptionKey=@{Key=[Convert]::ToBase64String($key)}} | ConvertTo-Json -Depth 3
[IO.File]::WriteAllText($keyPath,$json,(New-Object Text.UTF8Encoding($false)))
$listLog = Join-Path $OutputRoot 'stock-pak-list.log'
& $UnrealPak -List $pak ('-cryptokeys=' + $keyPath) *> $listLog
if ($LASTEXITCODE -ne 0) { throw "Stock PAK listing failed; log: $listLog" }
$matches = @(Select-String -LiteralPath $listLog -Pattern 'AssetRegistry\.bin' -SimpleMatch:$false)
if ($matches.Count -ne 1) { throw "Expected one AssetRegistry.bin in stock PAK; observed $($matches.Count)." }
$extract = Join-Path $OutputRoot 'extracted'
$null = New-Item -ItemType Directory -Path $extract -Force
$extractLog = Join-Path $OutputRoot 'stock-registry-extract.log'
& $UnrealPak $pak -Extract $extract '-Filter=Voyage/AssetRegistry.bin' ('-cryptokeys=' + $keyPath) *> $extractLog
if ($LASTEXITCODE -ne 0) { throw "Stock registry extraction failed; log: $extractLog" }
$files = @(Get-ChildItem -LiteralPath $extract -File -Recurse)
if ($files.Count -ne 1 -or $files[0].Name -cne 'AssetRegistry.bin' -or $files[0].Length -lt 1024) {
    throw 'Stock registry extraction produced an unexpected file set.'
}
[pscustomobject]@{
    status = 'passed'
    registryPath = $files[0].FullName
    registrySha256 = (Get-FileHash -LiteralPath $files[0].FullName -Algorithm SHA256).Hash
    registryLength = $files[0].Length
    stockPakSha256 = (Get-FileHash -LiteralPath $pak -Algorithm SHA256).Hash
    listLog = $listLog
    extractLog = $extractLog
}
