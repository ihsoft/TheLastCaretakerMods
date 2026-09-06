# Restores the exact predecessor recorded by Install-VoyageRelease.ps1.
# Files that did not exist before installation are removed only after exact hash checks.

[CmdletBinding()]
param(
    [Parameter(Mandatory = $true, Position = 0)]
    [string]$InstallManifest,

    [switch]$ValidateOnly
)

$ErrorActionPreference = 'Stop'
Set-StrictMode -Version Latest

$sha256Pattern = '^[0-9A-F]{64}$'

function Get-OptionalPropertyValue {
    param(
        [Parameter(Mandatory = $true)]
        [object]$Object,

        [Parameter(Mandatory = $true)]
        [string]$Name
    )

    $property = $Object.PSObject.Properties[$Name]
    if ($null -eq $property) {
        return $null
    }
    $property.Value
}

function Assert-SafeLeafName {
    param(
        [Parameter(Mandatory = $true)]
        [string]$Name
    )

    if ([string]::IsNullOrWhiteSpace($Name) -or
        [IO.Path]::GetFileName($Name) -cne $Name -or
        $Name.IndexOfAny([IO.Path]::GetInvalidFileNameChars()) -ge 0) {
        throw "Installation record has an unsafe filename: $Name"
    }
}

function Assert-GameClosed {
    $running = @(
        Get-Process -Name 'VoyageSteam-Win64-Shipping', 'Voyage' `
            -ErrorAction SilentlyContinue
    )
    if ($running.Count -gt 0) {
        $processes = ($running | ForEach-Object {
            "$($_.ProcessName):$($_.Id)"
        }) -join ', '
        throw "Voyage must be closed before restoring an installation: $processes"
    }
}

function Write-JsonFile {
    param(
        [Parameter(Mandatory = $true)]
        [string]$Path,

        [Parameter(Mandatory = $true)]
        [object]$Value
    )

    [IO.File]::WriteAllText(
        $Path,
        (($Value | ConvertTo-Json -Depth 10) + [Environment]::NewLine),
        [Text.UTF8Encoding]::new($false))
}

$manifestPath = (Resolve-Path -LiteralPath $InstallManifest).Path
$installationEvidence = Split-Path -Parent $manifestPath
$manifestSha256 = (Get-FileHash -Algorithm SHA256 -LiteralPath $manifestPath).Hash
$manifest = Get-Content -LiteralPath $manifestPath -Raw | ConvertFrom-Json
if ([int](Get-OptionalPropertyValue -Object $manifest -Name 'schemaVersion') -ne 1 -or
    [string](Get-OptionalPropertyValue -Object $manifest -Name 'kind') -cne
        'Voyage release installation' -or
    [string](Get-OptionalPropertyValue -Object $manifest -Name 'status') -cne
        'installed') {
    throw 'Only a completed Install-VoyageRelease installation manifest is accepted.'
}

$mod = [string](Get-OptionalPropertyValue -Object $manifest -Name 'mod')
$artifactVersion = [string](Get-OptionalPropertyValue -Object $manifest -Name 'artifactVersion')
$paksValue = [string](Get-OptionalPropertyValue -Object $manifest -Name 'paksDirectory')
$records = @(Get-OptionalPropertyValue -Object $manifest -Name 'files')
if ([string]::IsNullOrWhiteSpace($mod) -or
    [string]::IsNullOrWhiteSpace($artifactVersion) -or
    $records.Count -lt 1) {
    throw 'Installation manifest is missing its identity or file records.'
}
$paksDirectory = (Resolve-Path -LiteralPath $paksValue).Path
$gameRootValue = [string](Get-OptionalPropertyValue -Object $manifest -Name 'gameRoot')
$gameRoot = (Resolve-Path -LiteralPath $gameRootValue).Path
$expectedPaksDirectory = [IO.Path]::GetFullPath((Join-Path $gameRoot 'Voyage\Content\Paks'))
if (-not $paksDirectory.Equals($expectedPaksDirectory, [StringComparison]::OrdinalIgnoreCase) -or
    -not (Test-Path -LiteralPath (Join-Path $gameRoot `
        'Voyage\Binaries\Win64\VoyageSteam-Win64-Shipping.exe') -PathType Leaf)) {
    throw "Installation manifest does not identify its Voyage Paks directory: $paksDirectory"
}
$evidencePrefix = $installationEvidence.TrimEnd('\') + '\'

$names = @{}
$restoreItems = @(
    foreach ($record in $records) {
        $name = [string](Get-OptionalPropertyValue -Object $record -Name 'name')
        Assert-SafeLeafName -Name $name
        if ($names.ContainsKey($name)) {
            throw "Installation manifest contains a duplicate filename: $name"
        }
        $names[$name] = $true

        $destinationPath = [string](
            Get-OptionalPropertyValue -Object $record -Name 'destinationPath')
        $expectedDestination = Join-Path $paksDirectory $name
        if ([IO.Path]::GetFullPath($destinationPath) -cne
            [IO.Path]::GetFullPath($expectedDestination)) {
            throw "Installation destination does not match its Paks filename: $name"
        }

        $installedSha256 = [string](
            Get-OptionalPropertyValue -Object $record -Name 'installedSha256')
        if ($installedSha256 -cnotmatch $sha256Pattern) {
            throw "Installation record has an invalid installed SHA-256: $name"
        }
        if (-not (Test-Path -LiteralPath $expectedDestination -PathType Leaf)) {
            throw "Installed file is missing: $expectedDestination"
        }
        $target = Get-Item -LiteralPath $expectedDestination -Force
        if ($target.Attributes -band [IO.FileAttributes]::ReparsePoint) {
            throw "Refusing a reparse-point restoration target: $expectedDestination"
        }
        $currentSha256 = (
            Get-FileHash -Algorithm SHA256 -LiteralPath $expectedDestination).Hash
        if ($currentSha256 -cne $installedSha256) {
            throw "Refusing to restore over a changed installed file: $expectedDestination"
        }

        $hadExistingValue = Get-OptionalPropertyValue -Object $record -Name 'hadExisting'
        if ($null -eq $hadExistingValue -or $hadExistingValue -isnot [bool]) {
            throw "Installation record must explicitly declare boolean hadExisting: $name"
        }
        $hadExisting = [bool]$hadExistingValue
        $previousSha256 = $null
        $backupPath = $null
        if ($hadExisting) {
            $previousSha256 = [string](
                Get-OptionalPropertyValue -Object $record -Name 'previousSha256')
            $backupValue = [string](
                Get-OptionalPropertyValue -Object $record -Name 'backupPath')
            if ($previousSha256 -cnotmatch $sha256Pattern -or
                [string]::IsNullOrWhiteSpace($backupValue)) {
                throw "Previous-file evidence is incomplete: $name"
            }
            $backupPath = (Resolve-Path -LiteralPath $backupValue).Path
            if (-not $backupPath.StartsWith(
                $evidencePrefix,
                [StringComparison]::OrdinalIgnoreCase)) {
                throw "Backup is outside its installation evidence: $backupPath"
            }
            $backupSha256 = (
                Get-FileHash -Algorithm SHA256 -LiteralPath $backupPath).Hash
            if ($backupSha256 -cne $previousSha256) {
                throw "Previous-file backup hash mismatch: $backupPath"
            }
        }

        [pscustomobject][ordered]@{
            name = $name
            destinationPath = $expectedDestination
            installedSha256 = $installedSha256
            hadExisting = $hadExisting
            previousSha256 = $previousSha256
            backupPath = $backupPath
            currentStagingPath = $null
            restoreStagingPath = $null
            restoredSha256 = $null
        }
    }
)

$containerItems = @($restoreItems | Where-Object {
    @('.pak', '.ucas', '.utoc') -contains [IO.Path]::GetExtension($_.name).ToLowerInvariant()
})
$archiveItems = @($restoreItems | Where-Object {
    [IO.Path]::GetExtension($_.name).ToLowerInvariant() -ceq '.zip'
})
$sidecarItems = @($restoreItems | Where-Object {
    [IO.Path]::GetExtension($_.name).ToLowerInvariant() -ceq '.autoload'
})
$bases = @($containerItems | ForEach-Object {
    [IO.Path]::GetFileNameWithoutExtension($_.name)
} | Sort-Object -Unique)
$extensions = @($containerItems | ForEach-Object {
    [IO.Path]::GetExtension($_.name).ToLowerInvariant()
} | Sort-Object -Unique)
if ($restoreItems.Count -ne (4 + $sidecarItems.Count) -or $sidecarItems.Count -gt 1 -or $containerItems.Count -ne 3 -or
    $archiveItems.Count -ne 1 -or $bases.Count -ne 1 -or $extensions.Count -ne 3 -or
    $bases[0] -match '^(?i:global$|pakchunk)') {
    throw 'Restoration requires one standalone mod triplet and its release ZIP; stock containers are forbidden.'
}
if ($sidecarItems.Count -eq 1 -and $sidecarItems[0].name -cne ($bases[0] + '.autoload')) {
    throw 'Restoration autoload sidecar does not match the exact container basename.'
}

if ($ValidateOnly) {
    Write-Host "Voyage installation restore validated without mutation: $mod $artifactVersion"
    [pscustomobject][ordered]@{
        validated = $true
        mod = $mod
        artifactVersion = $artifactVersion
        installManifestPath = $manifestPath
        installManifestSha256 = $manifestSha256
        paksDirectory = $paksDirectory
        restoreCount = @($restoreItems | Where-Object { $_.hadExisting }).Count
        removeCount = @($restoreItems | Where-Object { -not $_.hadExisting }).Count
    }
    return
}

$token = [Guid]::NewGuid().ToString('N')
$timestamp = [DateTime]::UtcNow.ToString('yyyyMMdd-HHmmss')
$transactionPath = Join-Path $installationEvidence (
    "restore-transaction-$timestamp-$($token.Substring(0, 8)).json")
$restoreManifestPath = Join-Path $installationEvidence (
    "restore-manifest-$timestamp-$($token.Substring(0, 8)).json")
foreach ($item in $restoreItems) {
    $item.currentStagingPath = Join-Path $paksDirectory (
        '.' + $item.name + ".removing-$token")
    if ($item.hadExisting) {
        $item.restoreStagingPath = Join-Path $paksDirectory (
            '.' + $item.name + ".restoring-$token")
    }
}

$transaction = [ordered]@{
    schemaVersion = 1
    kind = 'Voyage release restoration transaction'
    status = 'preparing'
    installManifestPath = $manifestPath
    installManifestSha256 = $manifestSha256
    mod = $mod
    artifactVersion = $artifactVersion
    paksDirectory = $paksDirectory
    createdAtUtc = [DateTime]::UtcNow.ToString('o')
    files = $restoreItems
}
Write-JsonFile -Path $transactionPath -Value $transaction

$mutationStarted = $false
try {
    Assert-GameClosed

    foreach ($item in $restoreItems) {
        Copy-Item -LiteralPath $item.destinationPath `
            -Destination $item.currentStagingPath
        $stagedCurrentHash = (
            Get-FileHash -Algorithm SHA256 -LiteralPath $item.currentStagingPath).Hash
        if ($stagedCurrentHash -cne $item.installedSha256) {
            throw "Current-file staging hash mismatch: $($item.name)"
        }
        if ($item.hadExisting) {
            Copy-Item -LiteralPath $item.backupPath `
                -Destination $item.restoreStagingPath
            $stagedRestoreHash = (
                Get-FileHash -Algorithm SHA256 -LiteralPath $item.restoreStagingPath).Hash
            if ($stagedRestoreHash -cne $item.previousSha256) {
                throw "Restore staging hash mismatch: $($item.name)"
            }
        }
    }

    $transaction.status = 'prepared'
    $transaction.preparedAtUtc = [DateTime]::UtcNow.ToString('o')
    Write-JsonFile -Path $transactionPath -Value $transaction

    Assert-GameClosed
    foreach ($item in $restoreItems) {
        $currentHash = (Get-FileHash -Algorithm SHA256 -LiteralPath $item.destinationPath).Hash
        if ($currentHash -cne $item.installedSha256) {
            throw "Restoration target changed after staging: $($item.name)"
        }
    }
    $mutationStarted = $true
    foreach ($item in $restoreItems) {
        if ($item.hadExisting) {
            Move-Item -LiteralPath $item.restoreStagingPath `
                -Destination $item.destinationPath -Force
            $item.restoredSha256 = (
                Get-FileHash -Algorithm SHA256 -LiteralPath $item.destinationPath).Hash
            if ($item.restoredSha256 -cne $item.previousSha256) {
                throw "Restored file hash mismatch: $($item.name)"
            }
        }
        else {
            Remove-Item -LiteralPath $item.destinationPath -Force
            if (Test-Path -LiteralPath $item.destinationPath) {
                throw "Newly installed file still exists after removal: $($item.name)"
            }
        }
    }

    $restoredAtUtc = [DateTime]::UtcNow.ToString('o')
    $restoreManifest = [ordered]@{
        schemaVersion = 1
        kind = 'Voyage release installation restoration'
        status = 'restored'
        installManifestPath = $manifestPath
        installManifestSha256 = $manifestSha256
        mod = $mod
        artifactVersion = $artifactVersion
        paksDirectory = $paksDirectory
        restoredAtUtc = $restoredAtUtc
        files = $restoreItems
    }
    Write-JsonFile -Path $restoreManifestPath -Value $restoreManifest
    $transaction.status = 'restored'
    $transaction.completedAtUtc = $restoredAtUtc
    $transaction.restoreManifestPath = $restoreManifestPath
    Write-JsonFile -Path $transactionPath -Value $transaction
}
catch {
    $originalError = $_
    $recoveryErrors = @()
    if ($mutationStarted) {
        foreach ($item in $restoreItems) {
            try {
                if (-not (Test-Path -LiteralPath $item.currentStagingPath -PathType Leaf)) {
                    throw "Installed-state staging file is missing: $($item.currentStagingPath)"
                }
                $stagedCurrentHash = (
                    Get-FileHash -Algorithm SHA256 -LiteralPath $item.currentStagingPath).Hash
                if ($stagedCurrentHash -cne $item.installedSha256) {
                    throw "Installed-state staging file changed: $($item.currentStagingPath)"
                }
                Copy-Item -LiteralPath $item.currentStagingPath `
                    -Destination $item.destinationPath -Force
                $rolledBackHash = (
                    Get-FileHash -Algorithm SHA256 -LiteralPath $item.destinationPath).Hash
                if ($rolledBackHash -cne $item.installedSha256) {
                    throw "Restoration rollback hash mismatch: $($item.name)"
                }
            }
            catch {
                $recoveryErrors += $_.Exception.Message
            }
        }
    }
    $transaction.status = if ($recoveryErrors.Count -eq 0) { 'rolled-back' } else { 'recovery-failed' }
    if (Test-Path -LiteralPath $restoreManifestPath -PathType Leaf) {
        Remove-Item -LiteralPath $restoreManifestPath -Force
    }
    $transaction.failedAtUtc = [DateTime]::UtcNow.ToString('o')
    $transaction.error = $originalError.Exception.Message
    $transaction.recoveryErrors = $recoveryErrors
    Write-JsonFile -Path $transactionPath -Value $transaction
    if ($recoveryErrors.Count -gt 0) {
        throw ($originalError.Exception.Message + ' Recovery also failed: ' +
            ($recoveryErrors -join ' | ') + ". Evidence: $transactionPath")
    }
    throw ($originalError.Exception.Message +
        ". Restoration was rolled back. Evidence: $transactionPath")
}
finally {
    if ($transaction.status -cne 'recovery-failed') {
        foreach ($item in $restoreItems) {
            foreach ($stagingPath in @($item.currentStagingPath, $item.restoreStagingPath)) {
                if (-not [string]::IsNullOrWhiteSpace($stagingPath) -and
                    (Test-Path -LiteralPath $stagingPath -PathType Leaf)) {
                    Remove-Item -LiteralPath $stagingPath -Force
                }
            }
        }
    }
}

Write-Host "Previous Voyage installation restored: $mod $artifactVersion"
Write-Host "Restoration evidence: $restoreManifestPath"
[pscustomobject][ordered]@{
    mod = $mod
    artifactVersion = $artifactVersion
    restoreManifestPath = $restoreManifestPath
    paksDirectory = $paksDirectory
    restoredCount = @($restoreItems | Where-Object { $_.hadExisting }).Count
    removedCount = @($restoreItems | Where-Object { -not $_.hadExisting }).Count
}
