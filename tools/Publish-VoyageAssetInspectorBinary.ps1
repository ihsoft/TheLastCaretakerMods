[CmdletBinding()]
param()

$ErrorActionPreference = 'Stop'
Set-StrictMode -Version Latest
$repo = (Resolve-Path -LiteralPath (Join-Path $PSScriptRoot '..')).Path
$resolver = Join-Path $PSScriptRoot 'Get-VoyageAssetInspectorBinary.ps1'
$inputs = & $resolver -BuildInputsOnly
$sourceChanges = @(& git -C $repo status --porcelain --untracked-files=all -- tools/VoyageAssetInspector)
if ($LASTEXITCODE -ne 0 -or $sourceChanges.Count -ne 0) {
    throw 'Commit and validate Inspector source changes before replacing its canonical binary.'
}
$sourceCommit = (& git -C $repo log -1 --format=%H -- tools/VoyageAssetInspector).Trim()
if ($LASTEXITCODE -ne 0 -or $sourceCommit -notmatch '^[0-9a-f]{40}$') {
    throw 'Could not identify the Inspector source checkpoint.'
}
$existing = $null
try { $existing = & $resolver } catch { Write-Verbose $_.Exception.Message }
if ($null -ne $existing) {
    $existing | Add-Member -NotePropertyName Rebuilt -NotePropertyValue $false -PassThru
    return
}

$workspace = Join-Path $repo ('artifacts\tools\inspector-publish-' + [Guid]::NewGuid().ToString('N'))
$publish = Join-Path $workspace 'publish'
[IO.Directory]::CreateDirectory($workspace) | Out-Null
$log = Join-Path $workspace 'publish.log'
$project = Join-Path $PSScriptRoot 'VoyageAssetInspector\VoyageAssetInspector.csproj'
$savedPreference = $ErrorActionPreference
try {
    $ErrorActionPreference = 'Continue'
    & dotnet publish $project -c Release -r win-x64 --self-contained false `
        -p:PublishSingleFile=true -o $publish *> $log
    $publishExit = $LASTEXITCODE
}
finally { $ErrorActionPreference = $savedPreference }
if ($publishExit -ne 0) { throw "Inspector publish failed ($publishExit). Log: $log" }
$candidate = Join-Path $publish 'VoyageAssetInspector.exe'
if (-not (Test-Path -LiteralPath $candidate -PathType Leaf)) { throw "No published Inspector executable. Log: $log" }
$sidecars = @(Get-ChildItem -LiteralPath $publish -File | Where-Object {
    $_.Name -cne 'VoyageAssetInspector.exe' -and $_.Extension -cne '.pdb'
})
if ($sidecars.Count -ne 0) { throw "Single-file publish left required sidecars: $($sidecars.Name -join ', '). Log: $log" }
$savedPreference = $ErrorActionPreference
try {
    $ErrorActionPreference = 'Continue'
    $usage = (& $candidate 2>&1 | Out-String)
    $smokeExit = $LASTEXITCODE
}
finally { $ErrorActionPreference = $savedPreference }
if ($smokeExit -ne 2 -or $usage -notmatch 'Usage: VoyageAssetInspector') {
    throw "Published Inspector smoke failed ($smokeExit): $usage"
}
$inputsAfter = & $resolver -BuildInputsOnly
if ($inputsAfter.Fingerprint -cne $inputs.Fingerprint) { throw 'Inspector inputs changed during publication.' }
$manifest = [ordered]@{
    schemaVersion = 1
    kind = 'Voyage canonical AssetInspector'
    sourceCommit = $sourceCommit
    inputFingerprint = $inputs.Fingerprint
    inputs = $inputs.Files
    runtimeIdentifier = 'win-x64'
    targetFramework = 'net10.0'
    selfContained = $false
    singleFile = $true
    publishedAtUtc = [DateTime]::UtcNow.ToString('o')
    length = (Get-Item -LiteralPath $candidate).Length
    sha256 = (Get-FileHash -LiteralPath $candidate -Algorithm SHA256).Hash
}
$candidateManifest = Join-Path $workspace 'publish-manifest.json'
[IO.File]::WriteAllText($candidateManifest, ($manifest | ConvertTo-Json -Depth 6), [Text.UTF8Encoding]::new($false))
$destination = Join-Path $repo '.tools\bin\VoyageAssetInspector.exe'
$destinationManifest = Join-Path $repo '.tools\bin\VoyageAssetInspector.publish-manifest.json'
$targets = @($destination, $destinationManifest)
$previous = @{}
foreach ($path in $targets) {
    if (Test-Path -LiteralPath $path -PathType Leaf) {
        $backup = Join-Path $workspace ('previous-' + [IO.Path]::GetFileName($path))
        Copy-Item -LiteralPath $path -Destination $backup
        $previous[$path] = $backup
    }
}
try {
    Copy-Item -LiteralPath $candidate -Destination $destination -Force
    Copy-Item -LiteralPath $candidateManifest -Destination $destinationManifest -Force
    $result = & $resolver
}
catch {
    foreach ($path in $targets) {
        if ($previous.ContainsKey($path)) { Copy-Item -LiteralPath $previous[$path] -Destination $path -Force }
        elseif (Test-Path -LiteralPath $path -PathType Leaf) { Remove-Item -LiteralPath $path -Force }
    }
    throw
}
$result | Add-Member -NotePropertyName Rebuilt -NotePropertyValue $true -PassThru
