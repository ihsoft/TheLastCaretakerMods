[CmdletBinding()]
param()

$ErrorActionPreference = 'Stop'
Set-StrictMode -Version Latest

$repositoryRoot = (Resolve-Path -LiteralPath (Join-Path $PSScriptRoot '..')).Path
$resolver = Join-Path $PSScriptRoot 'Get-VoyageAssetPatcherBinary.ps1'
$sourceRoot = Join-Path $PSScriptRoot 'VoyageAssetPatcher'
$project = Join-Path $sourceRoot 'VoyageAssetPatcher.csproj'
$inputs = & $resolver -BuildInputsOnly

$sourceInputPaths = @($inputs.Files | Where-Object {
    [string]$_.path -like 'source/*'
} | ForEach-Object {
    'tools/VoyageAssetPatcher/' + ([string]$_.path).Substring('source/'.Length)
})
$sourceChanges = @(& git -C $repositoryRoot status --porcelain `
    --untracked-files=all -- $sourceInputPaths)
if ($LASTEXITCODE -ne 0 -or $sourceChanges.Count -ne 0) {
    throw 'Commit and validate VoyageAssetPatcher source changes before publication.'
}
$sourceCommit = [string]$inputs.SourceCommit

$existing = $null
try {
    $existing = & $resolver
}
catch {
    Write-Verbose $_.Exception.Message
}
if ($null -ne $existing) {
    $existing | Add-Member -NotePropertyName Rebuilt -NotePropertyValue $false -PassThru
    return
}

$workspace = Join-Path $repositoryRoot (
    'artifacts\tools\asset-patcher-publish-' + [Guid]::NewGuid().ToString('N'))
$publishDirectory = Join-Path $workspace 'publish'
$publishLog = Join-Path $workspace 'publish.log'
[IO.Directory]::CreateDirectory($workspace) | Out-Null

$savedPreference = $ErrorActionPreference
try {
    $ErrorActionPreference = 'Continue'
    & dotnet publish $project -c Release -r win-x64 --self-contained false `
        -p:PublishSingleFile=true -o $publishDirectory *> $publishLog
    $publishExitCode = $LASTEXITCODE
}
finally {
    $ErrorActionPreference = $savedPreference
}
if ($publishExitCode -ne 0) {
    throw "VoyageAssetPatcher publish failed ($publishExitCode). Log: $publishLog"
}

$candidate = Join-Path $publishDirectory 'VoyageAssetPatcher.exe'
if (-not (Test-Path -LiteralPath $candidate -PathType Leaf)) {
    throw "No published VoyageAssetPatcher executable. Log: $publishLog"
}
$sidecars = @(Get-ChildItem -LiteralPath $publishDirectory -File | Where-Object {
    $_.Name -cne 'VoyageAssetPatcher.exe' -and
    $_.Extension -notin @('.pdb', '.xml')
})
if ($sidecars.Count -ne 0) {
    throw ('Single-file publish left required sidecars: ' +
        (($sidecars | ForEach-Object Name) -join ', ') + ". Log: $publishLog")
}

$savedPreference = $ErrorActionPreference
try {
    $ErrorActionPreference = 'Continue'
    $usage = (& $candidate 2>&1 | Out-String)
    $smokeExitCode = $LASTEXITCODE
}
finally {
    $ErrorActionPreference = $savedPreference
}
if ($smokeExitCode -ne 2 -or $usage -notmatch 'Usage: VoyageAssetPatcher') {
    throw "Published VoyageAssetPatcher smoke failed ($smokeExitCode): $usage"
}

$inputsAfterBuild = & $resolver -BuildInputsOnly
if ($inputsAfterBuild.Fingerprint -cne $inputs.Fingerprint) {
    throw 'VoyageAssetPatcher inputs changed during publication.'
}
$manifest = [ordered]@{
    schemaVersion = 1
    kind = 'Voyage canonical AssetPatcher'
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
[IO.File]::WriteAllText(
    $candidateManifest,
    (($manifest | ConvertTo-Json -Depth 7) + [Environment]::NewLine),
    [Text.UTF8Encoding]::new($false))

$destination = Join-Path $repositoryRoot '.tools\bin\VoyageAssetPatcher.exe'
$destinationManifest = Join-Path $repositoryRoot (
    '.tools\bin\VoyageAssetPatcher.publish-manifest.json')
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
        if ($previous.ContainsKey($path)) {
            Copy-Item -LiteralPath $previous[$path] -Destination $path -Force
        }
        elseif (Test-Path -LiteralPath $path -PathType Leaf) {
            Remove-Item -LiteralPath $path -Force
        }
    }
    throw
}
$result | Add-Member -NotePropertyName Rebuilt -NotePropertyValue $true -PassThru
