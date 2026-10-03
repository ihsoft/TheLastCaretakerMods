[CmdletBinding()]
param([string]$EngineRoot = 'K:\Epic Games\UE_5.8\Engine')

$ErrorActionPreference = 'Stop'
Set-StrictMode -Version Latest
$repo = (Resolve-Path -LiteralPath (Join-Path $PSScriptRoot '..\..')).Path
$resolver = Join-Path $PSScriptRoot 'Get-VoyageAssetRegistryWriter.ps1'
$inputs = & $resolver -BuildInputsOnly -EngineRoot $EngineRoot
try {
    $existing = & $resolver -EngineRoot $EngineRoot
    $existing | Add-Member -NotePropertyName Rebuilt -NotePropertyValue $false -PassThru
    return
} catch { Write-Verbose $_.Exception.Message }

$projectRoot = $PSScriptRoot
$project = Join-Path $projectRoot 'VoyageAssetRegistryWriter.uproject'
$build = Join-Path $EngineRoot 'Build\BatchFiles\Build.bat'
if (-not (Test-Path -LiteralPath $build -PathType Leaf)) {
    throw "Unreal Build.bat is missing: $build"
}
$workspace = Join-Path $repo ('artifacts\tools\asset-registry-writer-publish-' +
    [Guid]::NewGuid().ToString('N'))
$null = New-Item -ItemType Directory -Path $workspace
$log = Join-Path $workspace 'build.log'
$savedPreference = $ErrorActionPreference
try {
    $ErrorActionPreference = 'Continue'
    & $build 'VoyageAssetRegistryWriterEditor' 'Win64' 'Development' `
        ('-Project=' + $project) '-WaitMutex' '-NoHotReloadFromIDE' `
        ('-Log=' + (Join-Path $workspace 'ubt.log')) *> $log
    $buildExit = $LASTEXITCODE
} finally { $ErrorActionPreference = $savedPreference }
if ($buildExit -ne 0) { throw "AssetRegistry writer build failed ($buildExit). Log: $log" }
$inputsAfter = & $resolver -BuildInputsOnly -EngineRoot $EngineRoot
if ($inputsAfter.Fingerprint -cne $inputs.Fingerprint) {
    throw 'AssetRegistry writer source inputs changed during publication.'
}

$candidate = Join-Path $workspace 'candidate'
$null = New-Item -ItemType Directory -Path (Join-Path $candidate 'Binaries\Win64') -Force
Copy-Item -LiteralPath $project -Destination $candidate
$binaryRoot = Join-Path $projectRoot 'Binaries\Win64'
$binaryNames = @('UnrealEditor-VoyageAssetRegistryWriter.dll',
    'UnrealEditor.modules')
foreach ($name in $binaryNames) {
    $path = Join-Path $binaryRoot $name
    if (-not (Test-Path -LiteralPath $path -PathType Leaf)) {
        throw "Published writer binary is missing: $path"
    }
    Copy-Item -LiteralPath $path -Destination (Join-Path $candidate 'Binaries\Win64')
}
$candidateModules = Get-Content -LiteralPath `
    (Join-Path $candidate 'Binaries\Win64\UnrealEditor.modules') -Raw |
    ConvertFrom-Json
if ([string]$candidateModules.BuildId -cne [string]$inputs.Engine.buildId) {
    throw 'Published writer module BuildId does not match the selected Unreal Engine.'
}
$files = @(Get-ChildItem -LiteralPath $candidate -File -Recurse | ForEach-Object {
    [pscustomobject]@{
        path = $_.FullName.Substring($candidate.Length + 1).Replace('\', '/')
        length = $_.Length
        sha256 = (Get-FileHash -LiteralPath $_.FullName -Algorithm SHA256).Hash
    }
})
$manifest = [ordered]@{
    schemaVersion = 1
    kind = 'Voyage canonical AssetRegistry writer'
    engineVersion = '5.8'
    engine = $inputs.Engine
    target = 'VoyageAssetRegistryWriterEditor Win64 Development'
    inputFingerprint = $inputs.Fingerprint
    sourceCommit = (& git -C $repo rev-parse HEAD).Trim()
    sourceStatus = @(& git -C $repo status --porcelain -- tools/VoyageAssetRegistryWriter)
    publishedAtUtc = [DateTime]::UtcNow.ToString('o')
    files = $files
}
[IO.File]::WriteAllText((Join-Path $candidate 'publish-manifest.json'),
    ($manifest | ConvertTo-Json -Depth 8), (New-Object Text.UTF8Encoding($false)))
$destination = Join-Path $repo '.tools\bin\VoyageAssetRegistryWriter'
$previous = $null
if (Test-Path -LiteralPath $destination -PathType Container) {
    $previous = Join-Path $workspace 'previous'
    Move-Item -LiteralPath $destination -Destination $previous
}
try {
    Copy-Item -LiteralPath $candidate -Destination $destination -Recurse
    $result = & $resolver -EngineRoot $EngineRoot
} catch {
    if (Test-Path -LiteralPath $destination) { Remove-Item -LiteralPath $destination -Recurse -Force }
    if ($previous) { Move-Item -LiteralPath $previous -Destination $destination }
    throw
}
$result | Add-Member -NotePropertyName Rebuilt -NotePropertyValue $true -PassThru
