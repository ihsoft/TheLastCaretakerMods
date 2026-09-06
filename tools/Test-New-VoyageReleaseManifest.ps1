[CmdletBinding()]
param(
    [switch]$KeepArtifacts
)

$ErrorActionPreference = 'Stop'
Set-StrictMode -Version Latest

$repositoryRoot = (Resolve-Path -LiteralPath (Join-Path $PSScriptRoot '..')).Path
$testParent = Join-Path $repositoryRoot 'artifacts\tests\new-voyage-release-manifest'
$testRoot = Join-Path $testParent ([Guid]::NewGuid().ToString('N'))
$producer = Join-Path $PSScriptRoot 'New-VoyageReleaseManifest.ps1'
$steamApps = Join-Path $testRoot 'steamapps'
$gameRoot = Join-Path $steamApps 'common\Voyage'
$gameExecutable = Join-Path $gameRoot 'Voyage\Binaries\Win64\VoyageSteam-Win64-Shipping.exe'
$paksDirectory = Join-Path $gameRoot 'Voyage\Content\Paks'
$steamManifest = Join-Path $steamApps 'appmanifest_1783560.acf'
$releaseRoot = Join-Path $testRoot 'release'
$packageRoot = Join-Path $releaseRoot 'package'
$containerBase = 'SyntheticManifestMod_P'
$buildId = '77777777'
$checks = [Collections.Generic.List[object]]::new()
$dirtySourcePath = Join-Path $repositoryRoot (
    '.new-voyage-release-test-source-' + [Guid]::NewGuid().ToString('N') + '.txt')

function Add-Check {
    param(
        [Parameter(Mandatory = $true)]
        [string]$Name,

        [Parameter(Mandatory = $true)]
        [bool]$Passed,

        [string]$Evidence
    )

    $checks.Add([pscustomobject][ordered]@{
        name = $Name
        passed = $Passed
        evidence = $Evidence
    })
    if (-not $Passed) {
        throw "Check failed: $Name. $Evidence"
    }
}

function Assert-Rejected {
    param(
        [Parameter(Mandatory = $true)]
        [scriptblock]$Action,

        [Parameter(Mandatory = $true)]
        [string]$Pattern,

        [Parameter(Mandatory = $true)]
        [string]$Name
    )

    try {
        & $Action
    }
    catch {
        if ($_.Exception.Message -notmatch $Pattern) {
            throw
        }
        Add-Check -Name $Name -Passed $true -Evidence $_.Exception.Message
        return
    }
    throw "Expected rejection did not occur: $Pattern"
}

[IO.Directory]::CreateDirectory([IO.Path]::GetDirectoryName($gameExecutable)) | Out-Null
[IO.Directory]::CreateDirectory($paksDirectory) | Out-Null
[IO.Directory]::CreateDirectory($packageRoot) | Out-Null
[IO.File]::WriteAllText(
    $gameExecutable,
    'synthetic-voyage-executable',
    [Text.UTF8Encoding]::new($false))
[IO.File]::WriteAllText(
    $steamManifest,
    '"AppState" { "appid" "1783560" "buildid" "' + $buildId + '" }',
    [Text.UTF8Encoding]::new($false))

$payloadPaths = @()
foreach ($extension in @('.pak', '.ucas', '.utoc', '.autoload')) {
    $path = Join-Path $packageRoot ($containerBase + $extension)
    [IO.File]::WriteAllText(
        $path,
        "synthetic-$extension",
        [Text.UTF8Encoding]::new($false))
    $payloadPaths += $path
}
$archivePath = Join-Path $releaseRoot 'SyntheticManifestMod-test-v1.zip'
Compress-Archive -LiteralPath $payloadPaths -DestinationPath $archivePath

try {
    $result = & $producer -ReleaseRoot $releaseRoot `
        -Mod 'SyntheticManifestMod' -Version 'test-v1' `
        -Container (Join-Path $packageRoot ($containerBase + '.utoc')) `
        -Archive $archivePath -SourcePath (Join-Path $repositoryRoot 'AGENTS.md') `
        -GameRoot $gameRoot -ModVersion '1.2.3' `
        -InstalledArchiveName 'SyntheticManifestMod-test-v1.zip'
    Add-Check -Name 'manifest-created-and-validated' `
        -Passed ($result.status -ceq 'created-and-validated') `
        -Evidence ([string]$result.manifestPath)

    $manifest = Get-Content -LiteralPath $result.manifestPath -Raw | ConvertFrom-Json
    Add-Check -Name 'schema-and-fingerprint' `
        -Passed ($manifest.schemaVersion -eq 2 -and
            [string]$manifest.gameFingerprint.steamBuildId -ceq $buildId) `
        -Evidence ([string]$manifest.gameFingerprint.steamBuildId)
    Add-Check -Name 'exact-payload-records' `
        -Passed (@($manifest.payload).Count -eq 4 -and
            @($manifest.payload | Where-Object {
                $_.path -notmatch '^package/' -or $_.sha256 -notmatch '^[0-9A-F]{64}$'
            }).Count -eq 0) `
        -Evidence ((@($manifest.payload | ForEach-Object { $_.path })) -join ', ')
    Add-Check -Name 'optional-release-identity' `
        -Passed ([string]$manifest.modVersion -ceq '1.2.3' -and
            [string]$manifest.installedArchive.name -ceq
                'SyntheticManifestMod-test-v1.zip') `
        -Evidence (([string]$manifest.modVersion) + ' / ' +
            [string]$manifest.installedArchive.name)
    Add-Check -Name 'source-scope-recorded' `
        -Passed (@($manifest.sourcePaths).Count -eq 1 -and
            [string]$manifest.sourcePaths[0].path -ceq 'AGENTS.md') `
        -Evidence ($manifest.sourcePaths | ConvertTo-Json -Compress)

    [IO.File]::WriteAllText(
        $dirtySourcePath,
        'synthetic untracked source',
        [Text.UTF8Encoding]::new($false))
    $dirtyManifestPath = Join-Path $releaseRoot 'dirty-release-manifest.json'
    Assert-Rejected -Name 'dirty-source-needs-explicit-approval' `
        -Pattern 'declared release source has uncommitted changes' -Action {
        & $producer -ReleaseRoot $releaseRoot `
            -Mod 'SyntheticManifestMod' -Version 'dirty-test' `
            -Container (Join-Path $packageRoot ($containerBase + '.utoc')) `
            -Archive $archivePath -SourcePath $dirtySourcePath -GameRoot $gameRoot `
            -OutputPath $dirtyManifestPath
    }
    $dirtyResult = & $producer -ReleaseRoot $releaseRoot `
        -Mod 'SyntheticManifestMod' -Version 'dirty-test' `
        -Container (Join-Path $packageRoot ($containerBase + '.utoc')) `
        -Archive $archivePath -SourcePath $dirtySourcePath -GameRoot $gameRoot `
        -OutputPath $dirtyManifestPath -AllowDirtySource
    $dirtyManifest = Get-Content -LiteralPath $dirtyResult.manifestPath -Raw |
        ConvertFrom-Json
    Add-Check -Name 'dirty-test-candidate-recorded' `
        -Passed ([bool]$dirtyResult.dirtySource -and [bool]$dirtyManifest.dirtySource) `
        -Evidence ([string]$dirtyResult.manifestPath)

    $jsonManifestPath = Join-Path $releaseRoot 'json-result-manifest.json'
    $jsonText = & $producer -ReleaseRoot $releaseRoot `
        -Mod 'SyntheticManifestMod' -Version 'json-test' `
        -Container (Join-Path $packageRoot ($containerBase + '.utoc')) `
        -Archive $archivePath -SourcePath (Join-Path $repositoryRoot 'AGENTS.md') `
        -GameRoot $gameRoot -OutputPath $jsonManifestPath -AsJson
    $jsonResult = $jsonText | ConvertFrom-Json
    Add-Check -Name 'compact-json-result' `
        -Passed ([string]$jsonResult.status -ceq 'created-and-validated' -and
            [string]$jsonResult.manifestPath -ceq $jsonManifestPath) `
        -Evidence ([string]$jsonText)

    Assert-Rejected -Name 'immutable-output' -Pattern 'Refusing to overwrite' -Action {
        & $producer -ReleaseRoot $releaseRoot `
            -Mod 'SyntheticManifestMod' -Version 'test-v1' `
            -Container (Join-Path $packageRoot ($containerBase + '.utoc')) `
            -Archive $archivePath -SourcePath (Join-Path $repositoryRoot 'AGENTS.md') `
            -GameRoot $gameRoot
    }

    $badReleaseRoot = Join-Path $testRoot 'bad-release'
    $badPackageRoot = Join-Path $badReleaseRoot 'package'
    [IO.Directory]::CreateDirectory($badPackageRoot) | Out-Null
    $badPayload = @()
    foreach ($extension in @('.pak', '.ucas', '.utoc')) {
        $path = Join-Path $badPackageRoot ('BadArchiveMod_P' + $extension)
        [IO.File]::WriteAllText($path, "before-$extension")
        $badPayload += $path
    }
    $badArchive = Join-Path $badReleaseRoot 'BadArchiveMod.zip'
    Compress-Archive -LiteralPath $badPayload -DestinationPath $badArchive
    [IO.File]::WriteAllText($badPayload[0], 'after-archive')
    Assert-Rejected -Name 'zip-payload-mismatch-rejected' `
        -Pattern 'Release ZIP (does not match|content disagrees)' -Action {
        & $producer -ReleaseRoot $badReleaseRoot `
            -Mod 'BadArchiveMod' -Version 'test-v1' `
            -Container $badPayload[2] -Archive $badArchive `
            -SourcePath (Join-Path $repositoryRoot 'AGENTS.md') -GameRoot $gameRoot
    }
    Add-Check -Name 'failed-validation-left-no-manifest' `
        -Passed (-not (Test-Path -LiteralPath (
            Join-Path $badReleaseRoot 'release-manifest.json'))) `
        -Evidence $badReleaseRoot

    $outsideArchive = Join-Path $testRoot 'outside.zip'
    Copy-Item -LiteralPath $archivePath -Destination $outsideArchive
    $outsideOutput = Join-Path $releaseRoot 'outside-test.json'
    Assert-Rejected -Name 'archive-must-be-inside-release' `
        -Pattern 'Archive must be below' -Action {
        & $producer -ReleaseRoot $releaseRoot `
            -Mod 'SyntheticManifestMod' -Version 'outside-test' `
            -Container (Join-Path $packageRoot ($containerBase + '.utoc')) `
            -Archive $outsideArchive -SourcePath (Join-Path $repositoryRoot 'AGENTS.md') `
            -GameRoot $gameRoot -OutputPath $outsideOutput
    }

    $summary = [pscustomobject][ordered]@{
        status = 'passed'
        checkCount = $checks.Count
        testRoot = $testRoot
        gameFilesChanged = $false
        checks = $checks
    }
    $summaryPath = Join-Path $testRoot 'summary.json'
    [IO.File]::WriteAllText(
        $summaryPath,
        (($summary | ConvertTo-Json -Depth 8) + [Environment]::NewLine),
        [Text.UTF8Encoding]::new($false))
    $summary
}
finally {
    if (Test-Path -LiteralPath $dirtySourcePath -PathType Leaf) {
        Remove-Item -LiteralPath $dirtySourcePath -Force
    }
    if (-not $KeepArtifacts -and (Test-Path -LiteralPath $testRoot)) {
        Remove-Item -LiteralPath $testRoot -Recurse -Force
    }
}
