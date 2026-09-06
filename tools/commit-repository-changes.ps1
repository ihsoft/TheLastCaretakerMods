param(
    [Parameter(Mandatory = $true)]
    [ValidateNotNullOrEmpty()]
    [string[]] $Path,

    [Parameter(Mandatory = $true)]
    [ValidateNotNullOrEmpty()]
    [string] $Message,

    [string] $Owner = "",
    [string] $ExpectedHead = "",

    [ValidateRange(0, 3600)]
    [int] $WaitSeconds = 300,

    [switch] $UseExistingIndex
)

$ErrorActionPreference = "Stop"
Set-StrictMode -Version Latest

$repoRoot = Split-Path -Parent $PSScriptRoot
$coordinationModule = Join-Path $PSScriptRoot "repository-coordination.psm1"
Import-Module $coordinationModule -Force

function Invoke-Git([string[]] $Arguments) {
    $previousPreference = $ErrorActionPreference
    try {
        $ErrorActionPreference = "Continue"
        $output = @(& git -C $repoRoot @Arguments 2>&1)
        $exitCode = $LASTEXITCODE
    } finally {
        $ErrorActionPreference = $previousPreference
    }

    if ($exitCode -ne 0) {
        $details = ($output | ForEach-Object { [string]$_ }) -join [System.Environment]::NewLine
        throw "git $($Arguments -join ' ') failed with exit code $exitCode.`n$details"
    }
    return $output
}

function Convert-ToRepositoryPath([string] $InputPath) {
    $absolutePath = if ([System.IO.Path]::IsPathRooted($InputPath)) {
        [System.IO.Path]::GetFullPath($InputPath)
    } else {
        [System.IO.Path]::GetFullPath((Join-Path $repoRoot $InputPath))
    }

    $rootWithSeparator = $repoRoot.TrimEnd('\', '/') + [System.IO.Path]::DirectorySeparatorChar
    if (-not $absolutePath.StartsWith(
            $rootWithSeparator,
            [System.StringComparison]::OrdinalIgnoreCase)) {
        throw "Commit path is outside the repository: $InputPath"
    }
    if (Test-Path -LiteralPath $absolutePath -PathType Container) {
        throw "Commit paths must name exact files, not directories: $InputPath"
    }

    return $absolutePath.Substring($rootWithSeparator.Length).Replace('\', '/')
}

function Assert-SamePathSet(
    [string[]] $Expected,
    [string[]] $Actual,
    [string] $Description) {
    $expectedSet = @($Expected | Sort-Object -Unique)
    $actualSet = @($Actual | Sort-Object -Unique)
    $difference = @(Compare-Object -ReferenceObject $expectedSet -DifferenceObject $actualSet)
    if ($difference.Count -gt 0) {
        throw "$Description path mismatch. Expected: [$($expectedSet -join ', ')]. Actual: [$($actualSet -join ', ')]."
    }
}

$inputPaths = @(
    $Path |
        ForEach-Object { $_ -split ',' } |
        ForEach-Object { $_.Trim() } |
        Where-Object { -not [string]::IsNullOrWhiteSpace($_) }
)
$relativePaths = @($inputPaths | ForEach-Object { Convert-ToRepositoryPath $_ } | Sort-Object -Unique)
if ($relativePaths.Count -eq 0) {
    throw "At least one exact file path is required."
}

$operation = "Commit exact repository paths: $Message"
Invoke-WithRepositoryLock -RepositoryRoot $repoRoot -Resource "git-transaction" `
    -Operation $operation -Owner $Owner -WaitSeconds $WaitSeconds -Action {
    $stagingAttempted = $false
    $commitCreated = $false
    try {
        $headBefore = [string](@(Invoke-Git @("rev-parse", "HEAD"))[0])
        if (-not [string]::IsNullOrWhiteSpace($ExpectedHead) -and
            $headBefore -ne $ExpectedHead) {
            throw "HEAD changed. Expected $ExpectedHead, actual $headBefore."
        }

        $preStaged = @(Invoke-Git @("diff", "--cached", "--no-renames", "--name-only", "--"))
        if ($UseExistingIndex) {
            Assert-SamePathSet $relativePaths $preStaged "Existing staged"
        } elseif ($preStaged.Count -gt 0) {
            throw "The index already contains staged paths: $($preStaged -join ', '). Another task may own them."
        } else {
            foreach ($relativePath in $relativePaths) {
                $status = @(Invoke-Git @(
                    "status",
                    "--porcelain=v1",
                    "--untracked-files=all",
                    "--",
                    $relativePath))
                if ($status.Count -eq 0) {
                    throw "Commit path has no current change: $relativePath"
                }
            }

            $stagingAttempted = $true
            Invoke-Git (@("add", "--") + $relativePaths) | Out-Null
        }

        $staged = @(Invoke-Git @("diff", "--cached", "--no-renames", "--name-only", "--"))
        Assert-SamePathSet $relativePaths $staged "Staged"
        Invoke-Git @("diff", "--cached", "--check") | Out-Null
        Invoke-Git @("diff", "--cached", "--stat", "--") | ForEach-Object { Write-Host $_ }

        Invoke-Git @("commit", "-m", $Message) | ForEach-Object { Write-Host $_ }
        $commitCreated = $true

        $headAfter = [string](@(Invoke-Git @("rev-parse", "HEAD"))[0])
        if ($headAfter -eq $headBefore) {
            throw "Git reported success but HEAD did not change."
        }

        $committedPaths = @(
            Invoke-Git @(
                "diff-tree",
                "--no-renames",
                "--no-commit-id",
                "--name-only",
                "-r",
                $headAfter)
        )
        Assert-SamePathSet $relativePaths $committedPaths "Committed"

        $remainingStaged = @(Invoke-Git @(
            "diff",
            "--cached",
            "--no-renames",
            "--name-only",
            "--"))
        if ($remainingStaged.Count -gt 0) {
            throw "Commit completed but staged paths remain: $($remainingStaged -join ', ')."
        }

        [pscustomobject]@{
            commit = $headAfter
            previousHead = $headBefore
            paths = $committedPaths
            lockResource = "git-transaction"
        }
    } catch {
        if (-not $commitCreated -and $stagingAttempted) {
            & git -C $repoRoot restore --staged -- $relativePaths 2>$null
        }
        throw
    }
}
