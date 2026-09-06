# Fresh, read-only Voyage producer/consumer matrix. No canonical publication or installation.
[CmdletBinding()]
param(
    [Parameter(Mandatory = $true)][string]$CandidateRetoc,
    [Parameter(Mandatory = $true)][string]$UpstreamRetoc,
    [Parameter(Mandatory = $true)][string]$LegacyRetoc,
    [string]$GameRoot = 'P:\SteamLibrary\steamapps\common\Voyage'
)
$ErrorActionPreference = 'Stop'
Set-StrictMode -Version Latest
$repo = (Resolve-Path -LiteralPath (Join-Path $PSScriptRoot '..')).Path
$candidate = (Resolve-Path -LiteralPath $CandidateRetoc).Path
$upstream = (Resolve-Path -LiteralPath $UpstreamRetoc).Path
$canonical = (Resolve-Path -LiteralPath $LegacyRetoc).Path
$canonicalHash = (Get-FileHash -LiteralPath $canonical -Algorithm SHA256).Hash
if ($canonicalHash -cne 'CF6E0A47F343A169413BE46EB750F3441F174D334AC0CAA14962F3F47BA93C1E' -or
    (Get-FileHash -LiteralPath $upstream -Algorithm SHA256).Hash -cne 'DF2B6F5D5087E5A49015E60EC88E41A76D4670A64F4BB31547A20BBCC0B5BD72') {
    throw 'Controls must be the recorded canonical 234f4e5 and upstream 885a8da binaries.'
}
$fp = & (Join-Path $PSScriptRoot 'Get-VoyageBuildFingerprint.ps1') -GameRoot $GameRoot | ConvertFrom-Json
if ($fp.steam.buildId -cne '25056839' -or $fp.executable.sha256 -cne 'CA84428CF4562C703BEDFF053DB727D14CC70C593451C09BE75A92828EFD9933') {
    throw 'Revalidate this Voyage fixture contract for the new game fingerprint.'
}
$filter = 'Equipment/ToolAbilities/BP_ToolAbility_Maintenance_Dismantle'
$run = Join-Path $repo ('artifacts\tests\retoc-imports-' + [Guid]::NewGuid().ToString('N'))
[IO.Directory]::CreateDirectory($run) | Out-Null
$extract = Join-Path $PSScriptRoot 'Extract-VoyagePackage.ps1'
$bounded = Join-Path $PSScriptRoot 'Invoke-VoyageBoundedTool.ps1'
$getJson = Join-Path $PSScriptRoot 'Get-VoyageAssetJson.ps1'
foreach ($mod in @(Get-ChildItem -LiteralPath (Join-Path $GameRoot 'Voyage\Content\Paks') -Filter '*.utoc' -File |
    Where-Object Name -NotMatch '^(global|pakchunk\d+(optional)?-Windows)\.utoc$')) {
    $inventory = & $getJson -GameRoot $GameRoot -ListPackages -Source Mod -ModContainer $mod.FullName
    if (@(Get-Content -LiteralPath $inventory.packageListPath | Where-Object { $_.Contains($filter) }).Count -ne 0) {
        throw "Installed override invalidates mixed-view control: $($mod.Name)"
    }
}
$sources = [ordered]@{}
foreach ($producer in @(@('canonical', $canonical), @('upstream', $upstream))) {
    $sources[$producer[0]] = & $extract -GameRoot $GameRoot -Filter $filter -Retoc $producer[1] `
        -RetocEngineVersion UE5_7 -AllowAdditionalContainers -OutputRoot (Join-Path $run ($producer[0] + '-legacy'))
}
$sources.candidate = & $extract -GameRoot $GameRoot -Filter $filter -Retoc $candidate -RetocEngineVersion UE5_8 -Source Game -OutputRoot (Join-Path $run 'candidate-legacy')
$sources.candidate57 = & $extract -GameRoot $GameRoot -Filter $filter -Retoc $candidate -RetocEngineVersion UE5_7 -Source Game -OutputRoot (Join-Path $run 'candidate57-legacy')
function Get-PayloadIdentity($Root) {
    $files = @(Get-ChildItem -LiteralPath $Root -Recurse -File | Where-Object { $_.Extension -in @('.uasset', '.uexp') -or $_.Name -eq 'scriptobjects.bin' } | Sort-Object FullName)
    if ($files.Count -ne 3) { throw 'Expected exactly one asset pair plus ScriptObjects.' }
    ($files | ForEach-Object { $_.FullName.Substring($Root.Length) + '=' + (Get-FileHash -LiteralPath $_.FullName -Algorithm SHA256).Hash }) -join '|'
}
if ((Get-PayloadIdentity $sources.canonical.outputPath) -cne (Get-PayloadIdentity $sources.candidate.outputPath)) {
    throw 'Candidate to-legacy changed accepted Voyage writer bytes.'
}
if ((Get-PayloadIdentity $sources.upstream.outputPath) -cne (Get-PayloadIdentity $sources.candidate57.outputPath)) {
    throw 'Explicit UE5_7 must match the upstream legacy writer byte-for-byte.'
}
$cases = @()
$zenPayload = $null
foreach ($name in @('canonical', 'upstream', 'candidate', 'candidate57')) {
    $destination = Join-Path $run ($name + '-zen')
    [IO.Directory]::CreateDirectory($destination) | Out-Null
    $utoc = Join-Path $destination 'ImportTest_P.utoc'
    $pack = & $bounded -Executable $candidate -Arguments @('to-zen', '--version', 'UE5_7', $sources[$name].outputPath, $utoc) -MemoryLimitMB 512 -TimeoutSeconds 15
    $verify = & $bounded -Executable $candidate -Arguments @('verify', $utoc) -MemoryLimitMB 512 -TimeoutSeconds 15
    $list = & $bounded -Executable $candidate -Arguments @('list', '--path', '--hash', $utoc) -MemoryLimitMB 512 -TimeoutSeconds 15
    $lines = @(Get-Content -LiteralPath $list.stdoutPath | Where-Object { -not [string]::IsNullOrWhiteSpace($_) })
    $assets = @($lines | Where-Object { $_ -match '\sExportBundleData\s' })
    $headers = @($lines | Where-Object { $_ -match '\sContainerHeader\s+-\s*$' })
    if ($lines.Count -ne 2 -or $headers.Count -ne 1 -or $assets.Count -ne 1 -or -not $assets[0].Contains($filter + '.uasset')) { throw "Unexpected inventory for $name" }
    $rawRoot = Join-Path $destination 'raw'
    $unpack = & $bounded -Executable $candidate -Arguments @('unpack', $utoc, $rawRoot) -MemoryLimitMB 512 -TimeoutSeconds 15
    $rawFiles = @(Get-ChildItem -LiteralPath $rawRoot -Recurse -File -Filter '*.uasset')
    $legacyFiles = @(Get-ChildItem -LiteralPath $sources[$name].outputPath -Recurse -File -Filter '*.uasset')
    if ($rawFiles.Count -ne 1 -or $legacyFiles.Count -ne 1) { throw 'Expected one raw Zen and one legacy header.' }
    $raw = [IO.File]::ReadAllBytes($rawFiles[0].FullName)
    $legacy = [IO.File]::ReadAllBytes($legacyFiles[0].FullName)
    # Exact Steam-25056839 fixture contract: saved-hash legacy header size at
    # offset 44; UE5 Zen summary CookedHeaderSize at offset 20. The converter
    # intentionally preserves the source legacy size, which differs by producer.
    if ($legacy.Length -lt 48 -or [BitConverter]::ToUInt32($legacy, 0) -ne 0x9E2A83C1L -or
        [BitConverter]::ToInt32($legacy, 4) -gt -8 -or [BitConverter]::ToInt32($legacy, 44) -ne $legacy.Length -or
        $raw.Length -lt 24 -or [BitConverter]::ToUInt32($raw, 20) -ne $legacy.Length) {
        throw 'Fixture header-size normalization precondition failed.'
    }
    for ($i = 20; $i -lt 24; $i++) { $raw[$i] = 0 }
    $sha = [Security.Cryptography.SHA256]::Create()
    try { $normalized = [BitConverter]::ToString($sha.ComputeHash($raw)).Replace('-', '') }
    finally { $sha.Dispose() }
    if ($null -eq $zenPayload) { $zenPayload = $normalized }
    elseif ($normalized -cne $zenPayload) { throw 'Zen assets differ outside the proven source-header-size field.' }
    $cases += [pscustomobject]@{ producer = $name; sourceCookedHeaderSize = $legacy.Length; normalizedZenSha256 = $normalized; extractionManifest = $sources[$name].manifestPath; packingReport = $pack.reportPath; verifyReport = $verify.reportPath; inventoryReport = $list.reportPath; rawReport = $unpack.reportPath }
}
if ((Get-FileHash -LiteralPath $canonical -Algorithm SHA256).Hash -cne $canonicalHash) { throw 'Canonical binary changed during the test.' }
$summary = [pscustomobject]@{
    status = 'passed'; cases = $cases; caseCount = $cases.Count; writerByteIdentity = $true; zenIdentityExceptSourceHeaderSize = $true
    steamBuildId = $fp.steam.buildId; executableSha256 = $fp.executable.sha256
    candidateSha256 = (Get-FileHash -LiteralPath $candidate -Algorithm SHA256).Hash
    evidencePath = $run; reportPath = (Join-Path $run 'summary.json'); gameFilesChanged = $false
}
[IO.File]::WriteAllText((Join-Path $run 'summary.json'), ($summary | ConvertTo-Json -Depth 6))
$summary | Select-Object status, caseCount, writerByteIdentity, zenIdentityExceptSourceHeaderSize, candidateSha256, reportPath, gameFilesChanged
