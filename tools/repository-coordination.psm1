Set-StrictMode -Version Latest

function Resolve-CoordinationRepositoryRoot([string] $RepositoryRoot) {
    $resolvedRoot = (Resolve-Path -LiteralPath $RepositoryRoot).Path
    return [System.IO.Path]::GetFullPath($resolvedRoot).TrimEnd('\', '/')
}

function Get-CoordinationMutexName([string] $RepositoryRoot, [string] $Resource) {
    $identityRoot = $RepositoryRoot
    if ($env:OS -eq "Windows_NT") {
        $identityRoot = $identityRoot.ToUpperInvariant()
    }

    $identity = "$identityRoot`n$Resource"
    $sha256 = [System.Security.Cryptography.SHA256]::Create()
    try {
        $bytes = [System.Text.Encoding]::UTF8.GetBytes($identity)
        $hash = $sha256.ComputeHash($bytes)
    } finally {
        $sha256.Dispose()
    }

    $hashText = [System.BitConverter]::ToString($hash).Replace("-", "")
    return "TheLastCaretakerMods.RepositoryLock.$hashText"
}

function Get-CoordinationRecordPath([string] $RepositoryRoot, [string] $Resource) {
    $lockRoot = Join-Path $RepositoryRoot ".tools/repository-locks"
    return Join-Path $lockRoot "$Resource.json"
}

function Read-CoordinationRecord([string] $RecordPath) {
    if (-not (Test-Path -LiteralPath $RecordPath)) {
        return $null
    }

    try {
        return Get-Content -Raw -LiteralPath $RecordPath | ConvertFrom-Json
    } catch {
        return $null
    }
}

function Write-CoordinationRecord([string] $RecordPath, [object] $Record) {
    $recordRoot = Split-Path -Parent $RecordPath
    New-Item -ItemType Directory -Path $recordRoot -Force | Out-Null
    $json = $Record | ConvertTo-Json -Depth 4
    [System.IO.File]::WriteAllText(
        $RecordPath,
        $json,
        [System.Text.UTF8Encoding]::new($false))
}

function Format-CoordinationOwner([object] $Record) {
    if ($null -eq $Record) {
        return "an unknown owner (the diagnostic record is missing or unreadable)"
    }

    $owner = if ([string]::IsNullOrWhiteSpace([string]$Record.owner)) {
        "unknown owner"
    } else {
        [string]$Record.owner
    }
    $operation = if ([string]::IsNullOrWhiteSpace([string]$Record.operation)) {
        "unknown operation"
    } else {
        [string]$Record.operation
    }
    $hostName = if ([string]::IsNullOrWhiteSpace([string]$Record.hostName)) {
        "unknown host"
    } else {
        [string]$Record.hostName
    }
    $processId = if ($null -eq $Record.processId) {
        "unknown pid"
    } else {
        "pid $($Record.processId)"
    }
    $acquiredAt = if ([string]::IsNullOrWhiteSpace([string]$Record.acquiredAtUtc)) {
        "unknown time"
    } else {
        [string]$Record.acquiredAtUtc
    }
    return "$owner running '$operation' on $hostName ($processId), acquired at $acquiredAt"
}

function Get-RepositoryLockRecord {
    [CmdletBinding()]
    param(
        [Parameter(Mandatory = $true)]
        [string] $RepositoryRoot,

        [Parameter(Mandatory = $true)]
        [ValidatePattern('^[A-Za-z0-9][A-Za-z0-9._-]*$')]
        [string] $Resource
    )

    $resolvedRoot = Resolve-CoordinationRepositoryRoot $RepositoryRoot
    $recordPath = Get-CoordinationRecordPath $resolvedRoot $Resource
    return Read-CoordinationRecord $recordPath
}

function Invoke-WithRepositoryLock {
    [CmdletBinding()]
    param(
        [Parameter(Mandatory = $true)]
        [string] $RepositoryRoot,

        [Parameter(Mandatory = $true)]
        [ValidatePattern('^[A-Za-z0-9][A-Za-z0-9._-]*$')]
        [string] $Resource,

        [Parameter(Mandatory = $true)]
        [ValidateNotNullOrEmpty()]
        [string] $Operation,

        [string] $Owner = "",

        [ValidateRange(0, 3600)]
        [int] $WaitSeconds = 300,

        [Parameter(Mandatory = $true)]
        [scriptblock] $Action
    )

    $resolvedRoot = Resolve-CoordinationRepositoryRoot $RepositoryRoot
    $recordPath = Get-CoordinationRecordPath $resolvedRoot $Resource
    $mutexName = Get-CoordinationMutexName $resolvedRoot $Resource
    $mutex = [System.Threading.Mutex]::new($false, $mutexName)
    $acquired = $false
    $abandoned = $false
    $token = [System.Guid]::NewGuid().ToString("N")

    if ([string]::IsNullOrWhiteSpace($Owner)) {
        $Owner = "process-$PID"
    }

    try {
        try {
            $acquired = $mutex.WaitOne([System.TimeSpan]::FromSeconds($WaitSeconds))
        } catch [System.Threading.AbandonedMutexException] {
            $acquired = $true
            $abandoned = $true
        }

        if (-not $acquired) {
            $existingRecord = Read-CoordinationRecord $recordPath
            $ownerDescription = Format-CoordinationOwner $existingRecord
            throw "Repository resource '$Resource' is locked by $ownerDescription."
        }

        $previousRecord = Read-CoordinationRecord $recordPath
        if ($abandoned) {
            Write-Warning "Recovered abandoned lock '$Resource'. Previous record: $(Format-CoordinationOwner $previousRecord)."
        } elseif ($null -ne $previousRecord) {
            Write-Warning "Replacing an orphaned diagnostic record for '$Resource'."
        }

        $record = [ordered]@{
            schemaVersion = 1
            resource = $Resource
            repositoryRoot = $resolvedRoot
            owner = $Owner
            operation = $Operation
            hostName = [System.Environment]::MachineName
            processId = $PID
            token = $token
            acquiredAtUtc = [System.DateTimeOffset]::UtcNow.ToString("O")
            recoveredAbandonedLock = $abandoned
        }
        Write-CoordinationRecord $recordPath $record
        return & $Action
    } finally {
        if ($acquired) {
            $currentRecord = Read-CoordinationRecord $recordPath
            if ($null -ne $currentRecord -and [string]$currentRecord.token -eq $token) {
                Remove-Item -LiteralPath $recordPath -Force -ErrorAction SilentlyContinue
            }
            $mutex.ReleaseMutex()
        }
        $mutex.Dispose()
    }
}

Export-ModuleMember -Function Get-RepositoryLockRecord, Invoke-WithRepositoryLock
