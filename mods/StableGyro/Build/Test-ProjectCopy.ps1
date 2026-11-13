$ErrorActionPreference = 'Stop'
$mod = Split-Path -Parent $PSScriptRoot
$repo = [IO.Path]::GetFullPath((Join-Path $mod '../..'))
Import-Module (Join-Path $repo 'tools/VoyageModPipeline.psm1') -Force
$run = New-VoyageModRun -ModRoot $mod -Kind test-copy
$destination = Join-Path $run 'nested/Project'
$project = & (Join-Path $mod 'New-StableGyroProjectCopy.ps1') -Destination $destination
foreach ($source in Get-ChildItem -LiteralPath (Join-Path $mod 'Project/Content') -Recurse -File) {
    $relative = $source.FullName.Substring((Join-Path $mod 'Project').Length + 1)
    $copy = Join-Path $destination $relative
    if ([Convert]::ToBase64String([IO.File]::ReadAllBytes($source.FullName)) -cne [Convert]::ToBase64String([IO.File]::ReadAllBytes($copy))) { throw "Copy mismatch: $relative" }
}
foreach ($json in Get-ChildItem -LiteralPath (Join-Path $destination 'BlueprintText') -Recurse -Filter '*.json') {
    $text = Get-Content -LiteralPath $json.FullName -Raw
    if ($text -notmatch '"asset"\s*:\s*"([^"]+)"') { throw 'BPT asset identity missing.' }
    $asset = $Matches[1]
    $relative = $asset.Substring('/Game/'.Length)
    if ($json.FullName -ine (Join-Path $destination ('BlueprintText/' + $relative + '.json'))) { throw "BPT path mismatch: $asset" }
    if (-not (Test-Path -LiteralPath (Join-Path $destination ('Content/' + $relative + '.uasset')))) { throw "BPT asset missing: $asset" }
}
$support = Get-Content -LiteralPath (Join-Path $destination 'EditorSupport.json') -Raw | ConvertFrom-Json
foreach ($overlay in $support.plugins[0].overlays) {
    if (-not (Test-Path -LiteralPath $overlay.source)) { throw 'Relocated overlay missing.' }
}
foreach ($bad in @($destination, (Join-Path $mod 'invalid-copy'))) {
    $rejected = $false
    try { & (Join-Path $mod 'New-StableGyroProjectCopy.ps1') -Destination $bad | Out-Null } catch { $rejected = $true }
    if (-not $rejected) { throw "Unsafe copy destination accepted: $bad" }
}
Remove-VoyageModScratch -ModRoot $mod -RunRoot $run -Path (Join-Path $run 'nested')
'Project copy, byte preservation, BPT paths, native overlays and destination rejection passed.'
