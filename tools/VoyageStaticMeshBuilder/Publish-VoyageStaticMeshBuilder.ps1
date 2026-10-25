[CmdletBinding()]
param([string]$EngineRoot = 'K:\Epic Games\UE_5.8\Engine')

$ErrorActionPreference = 'Stop'
Set-StrictMode -Version Latest
$publishMutex = New-Object Threading.Mutex($false,
    'Local\VoyageStaticMeshBuilderPublisher')
$mutexAcquired = $false
try {
$mutexAcquired = $publishMutex.WaitOne(0)
if (-not $mutexAcquired) {
    throw 'Another VoyageStaticMeshBuilder publication is already running.'
}
$repo = (Resolve-Path -LiteralPath (Join-Path $PSScriptRoot '..\..')).Path
$resolver = Join-Path $PSScriptRoot 'Get-VoyageStaticMeshBuilder.ps1'
$inputs = & $resolver -BuildInputsOnly -EngineRoot $EngineRoot
$project = Join-Path $PSScriptRoot 'VoyageStaticMeshBuilder.uproject'
$build = Join-Path $EngineRoot 'Build\BatchFiles\Build.bat'
if (-not (Test-Path -LiteralPath $build -PathType Leaf)) {
    throw "Unreal Build.bat is missing: $build"
}
$workspace = [IO.Path]::GetFullPath((Join-Path $repo (
    'Tmp\tools\VoyageStaticMeshBuilder\publish-' + [Guid]::NewGuid().ToString('N'))))
$allowedWorkspace = [IO.Path]::GetFullPath((Join-Path $repo 'Tmp\tools\VoyageStaticMeshBuilder')).TrimEnd('\') + '\'
if (-not $workspace.StartsWith($allowedWorkspace, [StringComparison]::OrdinalIgnoreCase)) {
    throw "Publication workspace escaped the owned Tmp root: $workspace"
}
$null = New-Item -ItemType Directory -Path $workspace -Force
$log = Join-Path $workspace 'build.log'
$savedPreference = $ErrorActionPreference
try {
    $ErrorActionPreference = 'Continue'
    & $build 'VoyageStaticMeshBuilderEditor' 'Win64' 'Development' `
        ('-Project=' + $project) '-WaitMutex' '-NoHotReloadFromIDE' '-NoUBA' `
        ('-Log=' + (Join-Path $workspace 'ubt.log')) *> $log
    $buildExit = $LASTEXITCODE
} finally { $ErrorActionPreference = $savedPreference }
if ($buildExit -ne 0) { throw "StaticMesh builder build failed ($buildExit). Log: $log" }
$inputsAfter = & $resolver -BuildInputsOnly -EngineRoot $EngineRoot
if ($inputsAfter.Fingerprint -cne $inputs.Fingerprint) {
    throw 'StaticMesh builder source inputs changed during publication.'
}

$candidate = Join-Path $workspace 'candidate'
$null = New-Item -ItemType Directory -Path (Join-Path $candidate 'Binaries\Win64') -Force
$null = New-Item -ItemType Directory -Path (Join-Path $candidate 'Config') -Force
Copy-Item -LiteralPath $project -Destination $candidate
Copy-Item -LiteralPath (Join-Path $PSScriptRoot 'Config\DefaultEngine.ini') `
    -Destination (Join-Path $candidate 'Config\DefaultEngine.ini')
$binaryRoot = Join-Path $PSScriptRoot 'Binaries\Win64'
foreach ($name in @('UnrealEditor-VoyageStaticMeshBuilder.dll','UnrealEditor.modules')) {
    $path = Join-Path $binaryRoot $name
    if (-not (Test-Path -LiteralPath $path -PathType Leaf)) {
        throw "Published builder binary is missing: $path"
    }
    Copy-Item -LiteralPath $path -Destination (Join-Path $candidate 'Binaries\Win64')
}
$candidateModules = Get-Content -LiteralPath `
    (Join-Path $candidate 'Binaries\Win64\UnrealEditor.modules') -Raw | ConvertFrom-Json
if ([string]$candidateModules.BuildId -cne [string]$inputs.Engine.buildId) {
    throw 'Published builder module BuildId does not match the selected Unreal Engine.'
}
$files = @(Get-ChildItem -LiteralPath $candidate -File -Recurse | ForEach-Object {
    [pscustomobject]@{
        path = $_.FullName.Substring($candidate.Length + 1).Replace('\', '/')
        length = $_.Length
    }
})
$manifest = [ordered]@{
    schemaVersion = 2
    kind = 'Voyage canonical StaticMesh builder'
    engineVersion = '5.8'
    engine = $inputs.Engine
    target = 'VoyageStaticMeshBuilderEditor Win64 Development'
    sourceStatus = @(& git -C $repo status --porcelain -- `
        'tools/New-VoyageStaticMeshAssets.ps1' 'tools/VoyageStaticMeshBuilder')
    publishedAtUtc = [DateTime]::UtcNow.ToString('o')
    files = $files
}
[IO.File]::WriteAllText((Join-Path $candidate 'publish-manifest.json'),
    ($manifest | ConvertTo-Json -Depth 8), (New-Object Text.UTF8Encoding($false)))
$destination = [IO.Path]::GetFullPath((Join-Path $repo '.tools\bin\VoyageStaticMeshBuilder'))
$expectedDestination = [IO.Path]::GetFullPath((Join-Path $repo '.tools\bin\VoyageStaticMeshBuilder'))
if ($destination -cne $expectedDestination) {
    throw "Unexpected publication destination: $destination"
}
$previous = $null
if (Test-Path -LiteralPath $destination -PathType Container) {
    $previous = [IO.Path]::GetFullPath((Join-Path $workspace 'previous'))
    if (-not $previous.StartsWith($workspace.TrimEnd('\') + '\',
            [StringComparison]::OrdinalIgnoreCase)) {
        throw "Previous-bundle path escaped the publication workspace: $previous"
    }
    Move-Item -LiteralPath $destination -Destination $previous
}
try {
    Copy-Item -LiteralPath $candidate -Destination $destination -Recurse
    $result = & $resolver -EngineRoot $EngineRoot
} catch {
    if (Test-Path -LiteralPath $destination) {
        if ([IO.Path]::GetFullPath($destination) -cne $expectedDestination) {
            throw "Refusing recovery removal outside the exact published bundle: $destination"
        }
        Remove-Item -LiteralPath $destination -Recurse -Force
    }
    if ($previous) { Move-Item -LiteralPath $previous -Destination $destination }
    throw
}
$result | Add-Member -NotePropertyName Rebuilt -NotePropertyValue $true -PassThru
} finally {
    if ($mutexAcquired) { $publishMutex.ReleaseMutex() }
    $publishMutex.Dispose()
}
