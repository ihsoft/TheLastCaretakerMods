[CmdletBinding()]
param(
    [switch]$KeepArtifacts,
    [switch]$WithAutoLoadSidecar,
    [switch]$WithContentPluginDescriptor,
    [switch]$RetainArchiveOnRestore
)

$ErrorActionPreference = 'Stop'
Set-StrictMode -Version Latest

$repositoryRoot = (Resolve-Path -LiteralPath (Join-Path $PSScriptRoot '..')).Path
$testParent = Join-Path $repositoryRoot 'Tmp\tests\install-voyage-release'
$testRoot = Join-Path $testParent ([Guid]::NewGuid().ToString('N'))
$installer = Join-Path $PSScriptRoot 'Install-VoyageRelease.ps1'
$steamApps = Join-Path $testRoot 'steamapps'
$gameRoot = Join-Path $steamApps 'common\Voyage'
$gameExecutable = Join-Path $gameRoot 'Voyage\Binaries\Win64\VoyageSteam-Win64-Shipping.exe'
$paksDirectory = Join-Path $gameRoot 'Voyage\Content\Paks'
$steamManifest = Join-Path $steamApps 'appmanifest_1783560.acf'
$evidenceParent = Join-Path $testRoot 'evidence'
$restorer = Join-Path $PSScriptRoot 'Restore-VoyageReleaseInstallation.ps1'
$buildId = '99999999'
$modName = 'SyntheticVoyageMod'
$containerName = 'SyntheticVoyageMod_P'
$fixtureExtensions = @('.pak', '.ucas', '.utoc')
if ($WithAutoLoadSidecar) { $fixtureExtensions += '.autoload' }
$expectedInstalledCount = $fixtureExtensions.Count + 1 +
    $(if ($WithContentPluginDescriptor) { 1 } else { 0 })
$pluginName = 'SyntheticCatalogue'
$pluginInstallPath = Join-Path $gameRoot "Voyage\Mods\$pluginName\$pluginName.uplugin"
$sourceCommit = (& git -C $repositoryRoot rev-parse HEAD).Trim()
if ($LASTEXITCODE -ne 0 -or $sourceCommit -notmatch '^[0-9a-f]{40}$') {
    throw 'Could not resolve the repository HEAD for the synthetic manifest.'
}

function Write-Utf8Json {
    param(
        [Parameter(Mandatory = $true)]
        [string]$Path,

        [Parameter(Mandatory = $true)]
        [object]$Value
    )

    [IO.File]::WriteAllText(
        $Path,
        (($Value | ConvertTo-Json -Depth 8) + [Environment]::NewLine),
        [Text.UTF8Encoding]::new($false))
}

function New-SyntheticRelease {
    param(
        [Parameter(Mandatory = $true)]
        [string]$Version,

        [Parameter(Mandatory = $true)]
        [string]$ContentMarker,

        [Parameter(Mandatory = $true)]
        [string]$ExecutableSha256
    )

    $releaseRoot = Join-Path $testRoot "release-$Version"
    $packageRoot = Join-Path $releaseRoot 'container\package'
    [IO.Directory]::CreateDirectory($packageRoot) | Out-Null
    $records = @()
    foreach ($extension in $fixtureExtensions) {
        $name = $containerName + $extension
        $path = Join-Path $packageRoot $name
        [IO.File]::WriteAllText(
            $path,
            $(if ($extension -ceq '.autoload') { '' } else { "$ContentMarker-$extension" }),
            [Text.UTF8Encoding]::new($false))
        $item = Get-Item -LiteralPath $path
        $records += [pscustomobject][ordered]@{
            name = $name
            size = $item.Length
            sha256 = (Get-FileHash -Algorithm SHA256 -LiteralPath $path).Hash
        }
    }

    $contentPlugin = $null
    if ($WithContentPluginDescriptor) {
        $descriptorDirectory = Join-Path $packageRoot "Mods\$pluginName"
        [IO.Directory]::CreateDirectory($descriptorDirectory) | Out-Null
        $descriptorPath = Join-Path $descriptorDirectory ($pluginName + '.uplugin')
        Write-Utf8Json -Path $descriptorPath -Value ([ordered]@{
            FileVersion = 3
            CanContainContent = $true
            EnabledByDefault = $true
            ExplicitlyLoaded = $false
            Modules = @()
        })
        $descriptor = Get-Item -LiteralPath $descriptorPath
        $contentPlugin = [ordered]@{
            type = 'content-plugin-descriptor'
            pluginName = $pluginName
            artifact = [ordered]@{
                name = $descriptor.Name
                path = "container/package/Mods/$pluginName/$pluginName.uplugin"
                size = $descriptor.Length
                sha256 = (Get-FileHash -Algorithm SHA256 -LiteralPath $descriptorPath).Hash
            }
            archivePath = "Mods/$pluginName/$pluginName.uplugin"
            installRelativePath = "Voyage/Mods/$pluginName/$pluginName.uplugin"
        }
    }
    $archivePath = Join-Path $releaseRoot "$modName-$Version.zip"
    Compress-Archive -Path (Join-Path $packageRoot '*') -DestinationPath $archivePath
    $archive = Get-Item -LiteralPath $archivePath
    $manifest = [ordered]@{
        schemaVersion = $(if ($WithContentPluginDescriptor) { 3 } else { 2 })
        mod = $modName
        version = $Version
        modVersion = 'test'
        sourceCommit = $sourceCommit
        dirtySource = $false
        gameFingerprint = [ordered]@{
            steamBuildId = $buildId
            executableSha256 = $ExecutableSha256
        }
        payload = $records
        archive = [ordered]@{
            name = $archive.Name
            size = $archive.Length
            sha256 = (Get-FileHash -Algorithm SHA256 -LiteralPath $archivePath).Hash
        }
    }
    if ($WithContentPluginDescriptor) {
        $manifest['contentPlugin'] = $contentPlugin
    }
    $manifestPath = Join-Path $releaseRoot 'release-manifest.json'
    Write-Utf8Json -Path $manifestPath -Value $manifest
    [pscustomobject]@{
        root = $releaseRoot
        manifestPath = $manifestPath
        records = $records
        archive = $manifest.archive
        contentPlugin = $contentPlugin
    }
}

function Assert-Hash {
    param(
        [Parameter(Mandatory = $true)]
        [string]$Path,

        [Parameter(Mandatory = $true)]
        [string]$Expected,

        [Parameter(Mandatory = $true)]
        [string]$Label
    )

    if (-not (Test-Path -LiteralPath $Path -PathType Leaf)) {
        throw "$Label is missing: $Path"
    }
    $actual = (Get-FileHash -Algorithm SHA256 -LiteralPath $Path).Hash
    if ($actual -cne $Expected) {
        throw "$Label hash mismatch: $actual != $Expected"
    }
}

function Assert-Rejected {
    param(
        [Parameter(Mandatory = $true)]
        [scriptblock]$Action,

        [Parameter(Mandatory = $true)]
        [string]$Pattern
    )
    try { & $Action }
    catch {
        if ($_.Exception.Message -notmatch $Pattern) { throw }
        return
    }
    throw "Expected rejection did not occur: $Pattern"
}

[IO.Directory]::CreateDirectory([IO.Path]::GetDirectoryName($gameExecutable)) | Out-Null
[IO.Directory]::CreateDirectory($paksDirectory) | Out-Null
[IO.File]::WriteAllText(
    $gameExecutable,
    'synthetic-voyage-executable',
    [Text.UTF8Encoding]::new($false))
[IO.File]::WriteAllText(
    $steamManifest,
    '"AppState" { "appid" "1783560" "buildid" "' + $buildId + '" }',
    [Text.UTF8Encoding]::new($false))
$executableSha256 = (Get-FileHash -Algorithm SHA256 -LiteralPath $gameExecutable).Hash

try {
    $previousHashes = @{}
    foreach ($extension in $fixtureExtensions) {
        $name = $containerName + $extension
        $path = Join-Path $paksDirectory $name
        [IO.File]::WriteAllText(
            $path,
            "previous-installed-$extension",
            [Text.UTF8Encoding]::new($false))
        $previousHashes[$name] = (Get-FileHash -Algorithm SHA256 -LiteralPath $path).Hash
    }

    $firstRelease = New-SyntheticRelease -Version 'test-v1' `
        -ContentMarker 'first' -ExecutableSha256 $executableSha256
    $validationEvidence = Join-Path $testRoot 'validation-must-not-exist'
    $validationResult = & $installer -ReleaseManifest $firstRelease.manifestPath `
        -GameRoot $gameRoot -EvidenceRoot $validationEvidence `
        -RetainArchiveOnRestore:$RetainArchiveOnRestore -ValidateOnly
    if (-not [bool]$validationResult.validated -or
        @($validationResult.files).Count -ne $expectedInstalledCount -or
        (Test-Path -LiteralPath $validationEvidence)) {
        throw 'ValidateOnly did not return a clean, mutation-free installation plan.'
    }

    $wrongFingerprintRelease = New-SyntheticRelease -Version 'wrong-fingerprint' `
        -ContentMarker 'wrong-fingerprint' -ExecutableSha256 ('A' * 64)
    Assert-Rejected -Pattern 'fingerprint does not match' -Action {
        & $installer -ReleaseManifest $wrongFingerprintRelease.manifestPath `
            -GameRoot $gameRoot -ValidateOnly
    }
    if ($WithAutoLoadSidecar) {
        $wrongSidecarRelease = New-SyntheticRelease -Version 'wrong-sidecar' `
            -ContentMarker 'wrong-sidecar' -ExecutableSha256 $executableSha256
        $wrongSidecarManifest = Get-Content -LiteralPath $wrongSidecarRelease.manifestPath -Raw | ConvertFrom-Json
        $wrongSidecarManifest.payload[-1].name = 'DifferentMod.autoload'
        Write-Utf8Json -Path $wrongSidecarRelease.manifestPath -Value $wrongSidecarManifest
        Assert-Rejected -Pattern 'autoload sidecar must use the exact container basename' -Action {
            & $installer -ReleaseManifest $wrongSidecarRelease.manifestPath -GameRoot $gameRoot -ValidateOnly
        }
    }
    $wrongArchiveRelease = New-SyntheticRelease -Version 'wrong-archive' `
        -ContentMarker 'archived-content' -ExecutableSha256 $executableSha256
    $wrongArchivePayloadPath = Join-Path $wrongArchiveRelease.root (
        'container\package\' + $containerName + '.pak')
    [IO.File]::WriteAllText($wrongArchivePayloadPath, 'different-payload')
    $wrongArchiveManifest = Get-Content -LiteralPath $wrongArchiveRelease.manifestPath `
        -Raw | ConvertFrom-Json
    $wrongArchiveManifest.payload[0].size = (Get-Item -LiteralPath $wrongArchivePayloadPath).Length
    $wrongArchiveManifest.payload[0].sha256 = (
        Get-FileHash -Algorithm SHA256 -LiteralPath $wrongArchivePayloadPath).Hash
    Write-Utf8Json -Path $wrongArchiveRelease.manifestPath -Value $wrongArchiveManifest
    Assert-Rejected -Pattern 'Release ZIP (does not match|content disagrees)' -Action {
        & $installer -ReleaseManifest $wrongArchiveRelease.manifestPath `
            -GameRoot $gameRoot -ValidateOnly
    }
    if ($WithContentPluginDescriptor) {
        $unsafePluginRelease = New-SyntheticRelease -Version 'unsafe-plugin-path' `
            -ContentMarker 'unsafe-plugin' -ExecutableSha256 $executableSha256
        $unsafePluginManifest = Get-Content -LiteralPath `
            $unsafePluginRelease.manifestPath -Raw | ConvertFrom-Json
        $unsafePluginManifest.contentPlugin.installRelativePath =
            'Voyage/Mods/../SyntheticCatalogue/SyntheticCatalogue.uplugin'
        Write-Utf8Json -Path $unsafePluginRelease.manifestPath `
            -Value $unsafePluginManifest
        Assert-Rejected -Pattern 'paths do not match' -Action {
            & $installer -ReleaseManifest $unsafePluginRelease.manifestPath `
                -GameRoot $gameRoot -ValidateOnly
        }
    }

    $directoryTargetPath = Join-Path $paksDirectory ($containerName + '.pak')
    $directoryTargetOriginal = [IO.File]::ReadAllBytes($directoryTargetPath)
    [IO.File]::Delete($directoryTargetPath)
    [IO.Directory]::CreateDirectory($directoryTargetPath) | Out-Null
    try {
        Assert-Rejected -Pattern 'directory or reparse-point installation target' -Action {
            & $installer -ReleaseManifest $firstRelease.manifestPath `
                -GameRoot $gameRoot -ValidateOnly
        }
    }
    finally {
        [IO.Directory]::Delete($directoryTargetPath)
        [IO.File]::WriteAllBytes($directoryTargetPath, $directoryTargetOriginal)
    }
    $installResult = & $installer -ReleaseManifest $firstRelease.manifestPath `
        -GameRoot $gameRoot -EvidenceRoot $evidenceParent `
        -RetainArchiveOnRestore:$RetainArchiveOnRestore

    foreach ($record in $firstRelease.records) {
        Assert-Hash -Path (Join-Path $paksDirectory $record.name) `
            -Expected $record.sha256 -Label "Installed $($record.name)"
    }
    if ($WithContentPluginDescriptor) {
        Assert-Hash -Path $pluginInstallPath `
            -Expected $firstRelease.contentPlugin.artifact.sha256 `
            -Label 'Installed content plugin descriptor'
    }
    $firstArchiveName = $modName + '_test-v1.zip'
    Assert-Hash -Path (Join-Path $paksDirectory $firstArchiveName) `
        -Expected $firstRelease.archive.sha256 -Label 'Installed release archive'

    $installManifest = Get-Content -LiteralPath $installResult.installManifestPath `
        -Raw | ConvertFrom-Json
    if ([string]$installManifest.status -cne 'installed' -or
        @($installManifest.files).Count -ne $expectedInstalledCount) {
        throw 'Successful installation evidence is incomplete.'
    }
    $archiveEvidence = @($installManifest.files | Where-Object {
        [string]$_.kind -ceq 'archive'
    })
    if ($archiveEvidence.Count -ne 1 -or
        [bool]$archiveEvidence[0].retainOnRestore -ne
            [bool]$RetainArchiveOnRestore) {
        throw 'Release archive restore-retention evidence is incorrect.'
    }
    $installEvidenceDirectory = Split-Path -Parent $installResult.installManifestPath
    $legacyRestoreManifestPath = Join-Path $installEvidenceDirectory `
        'legacy-restore-manifest.json'
    $legacyRestoreManifest = Get-Content -LiteralPath `
        $installResult.installManifestPath -Raw | ConvertFrom-Json
    $legacyArchiveEvidence = @($legacyRestoreManifest.files | Where-Object {
        [string]$_.kind -ceq 'archive'
    })
    $legacyArchiveEvidence[0].PSObject.Properties.Remove('retainOnRestore')
    Write-Utf8Json -Path $legacyRestoreManifestPath -Value $legacyRestoreManifest
    $legacyRestoreValidation = & $restorer `
        -InstallManifest $legacyRestoreManifestPath -ValidateOnly
    if (-not [bool]$legacyRestoreValidation.validated -or
        [int]$legacyRestoreValidation.retainedCount -ne 0) {
        throw 'A legacy install manifest without archive retention was not accepted.'
    }

    $invalidRetentionTypePath = Join-Path $installEvidenceDirectory `
        'invalid-retention-type-manifest.json'
    $invalidRetentionTypeManifest = Get-Content -LiteralPath `
        $installResult.installManifestPath -Raw | ConvertFrom-Json
    $invalidTypeArchive = @($invalidRetentionTypeManifest.files | Where-Object {
        [string]$_.kind -ceq 'archive'
    })
    $invalidTypeArchive[0].retainOnRestore = 'true'
    Write-Utf8Json -Path $invalidRetentionTypePath -Value $invalidRetentionTypeManifest
    Assert-Rejected -Pattern 'retainOnRestore must be boolean' -Action {
        & $restorer -InstallManifest $invalidRetentionTypePath -ValidateOnly
    }

    $invalidRetentionKindPath = Join-Path $installEvidenceDirectory `
        'invalid-retention-kind-manifest.json'
    $invalidRetentionKindManifest = Get-Content -LiteralPath `
        $installResult.installManifestPath -Raw | ConvertFrom-Json
    $invalidKindPak = @($invalidRetentionKindManifest.files | Where-Object {
        [IO.Path]::GetExtension([string]$_.name).ToLowerInvariant() -ceq '.pak'
    })
    $invalidKindPak[0].kind = 'archive'
    $invalidKindPak[0] | Add-Member -NotePropertyName retainOnRestore `
        -NotePropertyValue $true -Force
    Write-Utf8Json -Path $invalidRetentionKindPath -Value $invalidRetentionKindManifest
    Assert-Rejected -Pattern 'Only a release archive may be retained' -Action {
        & $restorer -InstallManifest $invalidRetentionKindPath -ValidateOnly
    }
    $pakEvidence = @($installManifest.files | Where-Object {
        [string]$_.name -ceq ($containerName + '.pak')
    })
    if ($pakEvidence.Count -ne 1 -or -not [bool]$pakEvidence[0].hadExisting -or
        [string]$pakEvidence[0].previousSha256 -cne
            $previousHashes[$containerName + '.pak']) {
        throw 'Previous-file backup evidence is incorrect.'
    }
    Assert-Hash -Path ([string]$pakEvidence[0].backupPath) `
        -Expected $previousHashes[$containerName + '.pak'] `
        -Label 'Previous-file backup'
    if ($WithContentPluginDescriptor) {
        $unsafeRestoreManifestPath = Join-Path `
            (Split-Path -Parent $installResult.installManifestPath) `
            'unsafe-restore-manifest.json'
        $unsafeRestoreManifest = Get-Content -LiteralPath `
            $installResult.installManifestPath -Raw | ConvertFrom-Json
        $descriptorEvidence = @($unsafeRestoreManifest.files | Where-Object {
            [string]$_.kind -ceq 'content-plugin-descriptor'
        })
        if ($descriptorEvidence.Count -ne 1) {
            throw 'Content plugin installation evidence is missing or duplicated.'
        }
        $descriptorEvidence[0].createdDirectories = @(
            (Join-Path $gameRoot 'Voyage\Content'))
        Write-Utf8Json -Path $unsafeRestoreManifestPath -Value $unsafeRestoreManifest
        Assert-Rejected -Pattern 'unowned created directory' -Action {
            & $restorer -InstallManifest $unsafeRestoreManifestPath -ValidateOnly
        }
    }

    $installedBeforeFailure = @{}
    foreach ($extension in @('.pak', '.ucas', '.utoc')) {
        $name = $containerName + $extension
        $installedBeforeFailure[$name] = (
            Get-FileHash -Algorithm SHA256 -LiteralPath (
                Join-Path $paksDirectory $name)).Hash
    }

    $secondEvidenceParent = Join-Path $testRoot 'failure-evidence'
    $secondRelease = New-SyntheticRelease -Version 'test-v2' `
        -ContentMarker 'second' -ExecutableSha256 $executableSha256
    $global:VoyageInstallerTestMoveCount = 0
    function Move-Item {
        [CmdletBinding()]
        param(
            [Parameter(Mandatory = $true)]
            [string]$LiteralPath,

            [Parameter(Mandatory = $true)]
            [string]$Destination,

            [switch]$Force
        )

        if ($LiteralPath -like '*.installing-*') {
            $global:VoyageInstallerTestMoveCount++
            if ($global:VoyageInstallerTestMoveCount -eq 2) {
                throw 'Synthetic mid-install failure'
            }
        }
        Microsoft.PowerShell.Management\Move-Item @PSBoundParameters
    }

    $failureObserved = $false
    try {
        & $installer -ReleaseManifest $secondRelease.manifestPath `
            -GameRoot $gameRoot -EvidenceRoot $secondEvidenceParent `
            -RetainArchiveOnRestore:$RetainArchiveOnRestore
    }
    catch {
        if ($_.Exception.Message -notmatch 'Synthetic mid-install failure' -or
            $_.Exception.Message -notmatch 'rolled back') {
            throw
        }
        $failureObserved = $true
    }
    finally {
        Remove-Item -Path Function:\Move-Item
        Remove-Variable -Name VoyageInstallerTestMoveCount -Scope Global `
            -ErrorAction SilentlyContinue
    }
    if (-not $failureObserved) {
        throw 'Synthetic mid-install failure was not observed.'
    }

    foreach ($name in $installedBeforeFailure.Keys) {
        Assert-Hash -Path (Join-Path $paksDirectory $name) `
            -Expected $installedBeforeFailure[$name] -Label "Rolled-back $name"
    }
    if (Test-Path -LiteralPath (
        Join-Path $paksDirectory ($modName + '_test-v2.zip')) -PathType Leaf) {
        throw 'Failed installation left its release archive installed.'
    }
    $failureTransactionPath = @(
        Get-ChildItem -LiteralPath $secondEvidenceParent -Recurse -File `
            -Filter 'install-transaction.json'
    )
    if ($failureTransactionPath.Count -ne 1) {
        throw 'Failed installation did not produce one transaction record.'
    }
    $failureTransaction = Get-Content -LiteralPath $failureTransactionPath[0].FullName `
        -Raw | ConvertFrom-Json
    if ([string]$failureTransaction.status -cne 'rolled-back') {
        throw "Unexpected failure transaction status: $($failureTransaction.status)"
    }

    $installedPakPath = Join-Path $paksDirectory ($containerName + '.pak')
    $installedPakBytes = [IO.File]::ReadAllBytes($installedPakPath)
    $changedFileRejected = $false
    try {
        [IO.File]::WriteAllText($installedPakPath, 'external-change')
        try {
            & $restorer -InstallManifest $installResult.installManifestPath -ValidateOnly
        }
        catch {
            if ($_.Exception.Message -notmatch 'changed installed file') { throw }
            $changedFileRejected = $true
        }
    }
    finally {
        [IO.File]::WriteAllBytes($installedPakPath, $installedPakBytes)
    }
    if (-not $changedFileRejected) { throw 'Changed installed file was not rejected.' }

    $backupPakPath = [string]$pakEvidence[0].backupPath
    $backupPakBytes = [IO.File]::ReadAllBytes($backupPakPath)
    $changedBackupRejected = $false
    try {
        [IO.File]::WriteAllText($backupPakPath, 'damaged-backup')
        try {
            & $restorer -InstallManifest $installResult.installManifestPath -ValidateOnly
        }
        catch {
            if ($_.Exception.Message -notmatch 'backup hash mismatch') { throw }
            $changedBackupRejected = $true
        }
    }
    finally {
        [IO.File]::WriteAllBytes($backupPakPath, $backupPakBytes)
    }
    if (-not $changedBackupRejected) { throw 'Damaged backup was not rejected.' }

    $restoreValidation = & $restorer `
        -InstallManifest $installResult.installManifestPath -ValidateOnly
    if (-not [bool]$restoreValidation.validated -or
        [int]$restoreValidation.restoreCount -ne $fixtureExtensions.Count -or
        [int]$restoreValidation.removeCount -ne
            ($(if ($RetainArchiveOnRestore) { 0 } else { 1 }) +
                $(if ($WithContentPluginDescriptor) { 1 } else { 0 })) -or
        [int]$restoreValidation.retainedCount -ne
            $(if ($RetainArchiveOnRestore) { 1 } else { 0 })) {
        throw 'Restore ValidateOnly returned an unexpected plan.'
    }
    $restoreResult = & $restorer -InstallManifest $installResult.installManifestPath
    foreach ($name in $previousHashes.Keys) {
        Assert-Hash -Path (Join-Path $paksDirectory $name) `
            -Expected $previousHashes[$name] -Label "Restored $name"
    }
    $installedArchivePath = Join-Path $paksDirectory $firstArchiveName
    if ($RetainArchiveOnRestore) {
        Assert-Hash -Path $installedArchivePath -Expected $firstRelease.archive.sha256 `
            -Label 'Retained release archive'
    }
    elseif (Test-Path -LiteralPath $installedArchivePath) {
        throw 'Successful restore left a non-retained release archive behind.'
    }
    if ($WithContentPluginDescriptor -and
        (Test-Path -LiteralPath $pluginInstallPath)) {
        throw 'Successful restore left the content plugin descriptor behind.'
    }

    $reinstallEvidence = Join-Path $testRoot 'reinstall-evidence'
    $reinstallResult = & $installer -ReleaseManifest $firstRelease.manifestPath `
        -GameRoot $gameRoot -EvidenceRoot $reinstallEvidence `
        -RetainArchiveOnRestore:$RetainArchiveOnRestore
    $installedBeforeRestoreFailure = @{}
    foreach ($name in @(
        ($containerName + '.pak'),
        ($containerName + '.ucas'),
        ($containerName + '.utoc'),
        $firstArchiveName
    )) {
        $installedBeforeRestoreFailure[$name] = (
            Get-FileHash -Algorithm SHA256 -LiteralPath (
                Join-Path $paksDirectory $name)).Hash
    }

    $global:VoyageRestoreTestMoveCount = 0
    function Move-Item {
        [CmdletBinding()]
        param(
            [Parameter(Mandatory = $true)]
            [string]$LiteralPath,

            [Parameter(Mandatory = $true)]
            [string]$Destination,

            [switch]$Force
        )

        if ($LiteralPath -like '*.restoring-*') {
            $global:VoyageRestoreTestMoveCount++
            if ($global:VoyageRestoreTestMoveCount -eq 2) {
                throw 'Synthetic mid-restore failure'
            }
        }
        Microsoft.PowerShell.Management\Move-Item @PSBoundParameters
    }

    $restoreFailureObserved = $false
    try {
        & $restorer -InstallManifest $reinstallResult.installManifestPath
    }
    catch {
        if ($_.Exception.Message -notmatch 'Synthetic mid-restore failure' -or
            $_.Exception.Message -notmatch 'rolled back') {
            throw
        }
        $restoreFailureObserved = $true
    }
    finally {
        Remove-Item -Path Function:\Move-Item
        Remove-Variable -Name VoyageRestoreTestMoveCount -Scope Global `
            -ErrorAction SilentlyContinue
    }
    if (-not $restoreFailureObserved) {
        throw 'Synthetic mid-restore failure was not observed.'
    }
    foreach ($name in $installedBeforeRestoreFailure.Keys) {
        Assert-Hash -Path (Join-Path $paksDirectory $name) `
            -Expected $installedBeforeRestoreFailure[$name] `
            -Label "Restore rollback $name"
    }
    $restoreTransactions = @(
        Get-ChildItem -LiteralPath (
            Split-Path -Parent $reinstallResult.installManifestPath) -File `
            -Filter 'restore-transaction-*.json'
    )
    if ($restoreTransactions.Count -ne 1) {
        throw 'Failed restore did not produce one transaction record.'
    }
    $restoreTransaction = Get-Content -LiteralPath $restoreTransactions[0].FullName `
        -Raw | ConvertFrom-Json
    if ([string]$restoreTransaction.status -cne 'rolled-back') {
        throw "Unexpected restore transaction status: $($restoreTransaction.status)"
    }

    $global:VoyageRestoreRecoveryTestMoveCount = 0
    function Move-Item {
        [CmdletBinding()]
        param([string]$LiteralPath, [string]$Destination, [switch]$Force)
        if ($LiteralPath -like '*.restoring-*') {
            $global:VoyageRestoreRecoveryTestMoveCount++
            if ($global:VoyageRestoreRecoveryTestMoveCount -eq 2) {
                throw 'Synthetic restore failure before recovery failure'
            }
        }
        Microsoft.PowerShell.Management\Move-Item @PSBoundParameters
    }
    function Copy-Item {
        [CmdletBinding()]
        param([string]$LiteralPath, [string]$Destination, [switch]$Force)
        if ($LiteralPath -like '*.removing-*') {
            throw 'Synthetic recovery copy failure'
        }
        Microsoft.PowerShell.Management\Copy-Item @PSBoundParameters
    }
    $recoveryFailureObserved = $false
    try {
        & $restorer -InstallManifest $reinstallResult.installManifestPath
    }
    catch {
        if ($_.Exception.Message -notmatch 'Recovery also failed' -or
            $_.Exception.Message -notmatch 'Synthetic recovery copy failure') { throw }
        $recoveryFailureObserved = $true
    }
    finally {
        Remove-Item -Path Function:\Move-Item, Function:\Copy-Item
        Remove-Variable -Name VoyageRestoreRecoveryTestMoveCount -Scope Global `
            -ErrorAction SilentlyContinue
    }
    if (-not $recoveryFailureObserved) { throw 'Recovery failure was not observed.' }
    $recoveryTransactions = @(
        Get-ChildItem -LiteralPath (
            Split-Path -Parent $reinstallResult.installManifestPath) -File `
            -Filter 'restore-transaction-*.json' | ForEach-Object {
                Get-Content -LiteralPath $_.FullName -Raw | ConvertFrom-Json
            } | Where-Object { [string]$_.status -ceq 'recovery-failed' }
    )
    if ($recoveryTransactions.Count -ne 1) {
        throw 'Recovery failure evidence is missing.'
    }
    foreach ($record in @($recoveryTransactions[0].files | Where-Object {
        [string]$_.restoreAction -cne 'keep'
    })) {
        Assert-Hash -Path ([string]$record.currentStagingPath) `
            -Expected ([string]$record.installedSha256) `
            -Label "Preserved recovery copy $($record.name)"
    }

    Write-Host 'Voyage release install/restore synthetic regression: PASS'
    [pscustomobject][ordered]@{
        passed = $true
        autoLoadSidecar = [bool]$WithAutoLoadSidecar
        contentPluginDescriptor = [bool]$WithContentPluginDescriptor
        retainArchiveOnRestore = [bool]$RetainArchiveOnRestore
        checks = @('validate-only', 'fingerprint-rejection', 'archive-payload-rejection',
            'content-plugin-path-rejection',
            'directory-target-rejection', 'install-readback', 'predecessor-backup',
            'legacy-retention-default', 'retention-type-rejection',
            'retention-kind-rejection',
            'install-rollback', 'changed-file-rejection', 'damaged-backup-rejection',
            'restore-remove-or-retain', 'restore-rollback',
            'failed-recovery-copy-retention')
        artifactsRetained = [bool]$KeepArtifacts
        artifacts = if ($KeepArtifacts) { $testRoot } else { $null }
    }
}
finally {
    if (-not $KeepArtifacts -and (Test-Path -LiteralPath $testRoot)) {
        $resolvedTestRoot = (Resolve-Path -LiteralPath $testRoot).Path
        $resolvedTestParent = (Resolve-Path -LiteralPath $testParent).Path
        $requiredPrefix = $resolvedTestParent.TrimEnd('\') + '\'
        if (-not $resolvedTestRoot.StartsWith(
            $requiredPrefix,
            [StringComparison]::OrdinalIgnoreCase)) {
            throw "Refusing to remove test path outside the test root: $resolvedTestRoot"
        }
        Remove-Item -LiteralPath $resolvedTestRoot -Recurse -Force
    }
}
