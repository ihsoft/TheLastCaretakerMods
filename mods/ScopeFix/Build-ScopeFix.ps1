param(
    [string]$OutputRoot,
    [switch]$IgnorePinnedGameVersion,
    [switch]$AllowDirtySource
)
$ErrorActionPreference = 'Stop'
$repo = Split-Path (Split-Path $PSScriptRoot -Parent) -Parent
Set-Location $repo
$ownerTmp = [IO.Path]::GetFullPath((Join-Path $repo 'Tmp/ScopeFix'))
$publishedVersionText = (Get-Content -LiteralPath (Join-Path $PSScriptRoot 'version.txt') -Raw).Trim()
if ($publishedVersionText -cnotmatch '^(0|[1-9][0-9]*)$') {
    throw 'ScopeFix published version must be a non-negative integer in version.txt.'
}
$candidateVersion = 'V' + ([int]$publishedVersionText + 1) + '-candidate'
if ([string]::IsNullOrWhiteSpace($OutputRoot)) {
    $OutputRoot = Join-Path $ownerTmp ('build-' + (Get-Date -Format 'yyyyMMdd-HHmmss'))
}
$root = if ([IO.Path]::IsPathRooted($OutputRoot)) {
    [IO.Path]::GetFullPath($OutputRoot)
} else {
    [IO.Path]::GetFullPath((Join-Path $repo $OutputRoot))
}
if (-not $root.StartsWith($ownerTmp.TrimEnd('\') + '\',
        [StringComparison]::OrdinalIgnoreCase)) {
    throw 'OutputRoot must be a fresh directory below Tmp/ScopeFix.'
}
if (Test-Path -LiteralPath $root) { throw 'OutputRoot must be new.' }
$fingerprint = (& tools/Get-VoyageBuildFingerprint.ps1 | ConvertFrom-Json)
$gameBuild = & tools/Assert-VoyageModGameBuild.ps1 `
    -ModRoot $PSScriptRoot -ActualSteamBuildId ([string]$fingerprint.steam.buildId) `
    -IgnorePinnedGameVersion:$IgnorePinnedGameVersion
$loose = Join-Path $root 'loose'
$changes = @()
$extractRoots = @()
foreach ($spec in @(
    @{ Path = 'Data/Assets/Weapon/DA_Weapon_ElectricRifle'; Before = -2.0; After = -2.74; Z = 13.5 },
    @{ Path = 'Data/Assets/Attachments/DA_Weapon_Attachment_Sight_RedDot'; Before = 1.0; After = 1.13; Z = 0.275 }
)) {
    $jsonResult = & tools/Get-VoyageAssetJson.ps1 ('/Game/' + $spec.Path)
    $json = Get-Content $jsonResult.jsonPath -Raw | ConvertFrom-Json
    $owner = @($json | Where-Object { $_.Properties.ADSCameraOffset })
    if ($owner.Count -ne 1) { throw 'Expected exactly one ADSCameraOffset owner.' }
    $vector = $owner[0].Properties.ADSCameraOffset
    if ($vector.X -ne $spec.Before -or $vector.Y -ne 0 -or $vector.Z -ne $spec.Z) { throw 'Stock ADS vector changed.' }
    $extracted = & tools/Extract-VoyagePackage.ps1 -Filter $spec.Path -RetocEngineVersion UE5_8 -OutputRoot (Join-Path $root ($owner[0].Name))
    $extractRoots += [IO.Path]::GetFullPath($extracted.outputPath)
    $relative = 'Voyage/Content/' + $spec.Path
    $source = Join-Path $extracted.outputPath ($relative + '.uexp')
    $bytes = [IO.File]::ReadAllBytes($source)
    [byte[]]$needle = @([BitConverter]::GetBytes([double]$spec.Before)) + @([BitConverter]::GetBytes([double]0)) + @([BitConverter]::GetBytes([double]$spec.Z))
    $matches = @()
    for ($i = 0; $i -le $bytes.Length - $needle.Length; $i++) {
        $equal = $true
        for ($n = 0; $n -lt $needle.Length; $n++) { if ($bytes[$i+$n] -ne $needle[$n]) { $equal = $false; break } }
        if ($equal) { $matches += $i }
    }
    if ($matches.Count -ne 1) { throw 'Expected a unique serialized ADS vector.' }
    $offset = $matches[0]
    [Array]::Copy([BitConverter]::GetBytes([double]$spec.After), 0, $bytes, $offset, 8)
    $destination = Join-Path $loose $relative
    New-Item -ItemType Directory -Force (Split-Path $destination -Parent) | Out-Null
    Copy-Item -LiteralPath (Join-Path $extracted.outputPath ($relative + '.uasset')) -Destination ($destination + '.uasset')
    [IO.File]::WriteAllBytes(($destination + '.uexp'), $bytes)
    $changes += [pscustomobject]@{ package = '/Game/' + $spec.Path; property = 'ADSCameraOffset.X'; before = $spec.Before; after = $spec.After; exportByteOffset = $offset; sourceSha256 = (Get-FileHash $source).Hash; outputSha256 = (Get-FileHash ($destination + '.uexp')).Hash }
}
$container = Join-Path $root 'ScopeFix_P.utoc'
& tools/Invoke-VoyageBoundedTool.ps1 -Executable .tools/bin/retoc.exe -Arguments @('to-zen', $loose, $container, '--version', 'UE5_8') | Out-Null
& tools/Invoke-VoyageBoundedTool.ps1 -Executable .tools/bin/retoc.exe -Arguments @('verify', $container) | Out-Null
@{ status = 'container-verified-runtime-pending'; fingerprint = $fingerprint; gameBuild = $gameBuild; changes = $changes } | ConvertTo-Json -Depth 10 | Set-Content (Join-Path $root 'build-evidence.json') -Encoding UTF8
foreach ($scratch in @($extractRoots) + @($loose)) {
    if (-not $scratch.StartsWith($root.TrimEnd('\') + '\',
            [StringComparison]::OrdinalIgnoreCase)) {
        throw "Refusing to clean scratch outside ScopeFix run: $scratch"
    }
    Remove-Item -LiteralPath $scratch -Recurse -Force
}
$archive = Join-Path $root "ScopeFix-$candidateVersion.zip"
& (Join-Path $repo 'tools/New-VoyageVortexArchive.ps1') -Container $container -Archive $archive | Out-Null
$release = & (Join-Path $repo 'tools/New-VoyageReleaseManifest.ps1') `
    -ReleaseRoot $root -Mod 'ScopeFix' -Version $candidateVersion `
    -Container $container -Archive $archive -SourcePath $PSScriptRoot `
    -AllowDirtySource:$AllowDirtySource
[pscustomobject]@{ container = $container; archive = $archive; releaseManifest = $release.manifestPath; evidence = Join-Path $root 'build-evidence.json'; status = 'runtime-pending' }
