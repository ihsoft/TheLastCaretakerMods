# One read-only scan of the installed game executable; evidence stays ignored.
[CmdletBinding()]
param(
    [string]$GameRoot = 'P:\SteamLibrary\steamapps\common\Voyage'
)

$ErrorActionPreference = 'Stop'
Set-StrictMode -Version Latest

$repositoryRoot = (Resolve-Path -LiteralPath (Join-Path $PSScriptRoot '..')).Path
$testRoot = Join-Path $repositoryRoot (
    'artifacts\tests\executable-inspector-binary-' + [Guid]::NewGuid().ToString('N'))
[IO.Directory]::CreateDirectory($testRoot) | Out-Null
$checks = [Collections.Generic.List[string]]::new()

function Assert-InspectorTest {
    param(
        [Parameter(Mandatory = $true)]
        [bool]$Condition,

        [Parameter(Mandatory = $true)]
        [string]$Message
    )

    if (-not $Condition) {
        throw $Message
    }
}

function dotnet {
    throw 'Normal VoyageExecutableInspector operation attempted dotnet.'
}
function dotnet.exe {
    throw 'Normal VoyageExecutableInspector operation attempted dotnet.exe.'
}

foreach ($name in @(
    'Get-VoyageExecutableInspectorBinary.ps1',
    'Invoke-VoyageExecutableInspector.ps1',
    'Publish-VoyageExecutableInspectorBinary.ps1')) {
    $tokens = $null
    $parseErrors = $null
    [void][Management.Automation.Language.Parser]::ParseFile(
        (Join-Path $PSScriptRoot $name),
        [ref]$tokens,
        [ref]$parseErrors)
    Assert-InspectorTest ($parseErrors.Count -eq 0) "PowerShell parse failure: $name"
}
$checks.Add('public-script-parse')

$resolver = Join-Path $PSScriptRoot 'Get-VoyageExecutableInspectorBinary.ps1'
$publisher = Join-Path $PSScriptRoot 'Publish-VoyageExecutableInspectorBinary.ps1'
$binary = & $resolver
$beforeTime = (Get-Item -LiteralPath $binary.Path).LastWriteTimeUtc
$published = & $publisher
Assert-InspectorTest (-not [bool]$published.Rebuilt -and
    [string]$published.Sha256 -ceq [string]$binary.Sha256 -and
    (Get-Item -LiteralPath $binary.Path).LastWriteTimeUtc -eq $beforeTime) `
    'Publisher rebuilt a current VoyageExecutableInspector binary.'
$checks.Add('publisher-reuse-without-dotnet')

$savedPreference = $ErrorActionPreference
try {
    $ErrorActionPreference = 'Continue'
    $usage = (& $binary.Path 2>&1 | Out-String)
    $usageExitCode = $LASTEXITCODE
}
finally {
    $ErrorActionPreference = $savedPreference
}
Assert-InspectorTest ($usageExitCode -eq 2 -and
    $usage -match 'Usage: VoyageExecutableInspector') `
    'Canonical VoyageExecutableInspector usage smoke failed.'
$checks.Add('canonical-executable-smoke')

$existingOutput = Join-Path $testRoot 'existing.txt'
[IO.File]::WriteAllText($existingOutput, 'must-not-change')
$existingHash = (Get-FileHash -LiteralPath $existingOutput -Algorithm SHA256).Hash
$overwriteRejected = $false
try {
    & (Join-Path $PSScriptRoot 'Invoke-VoyageExecutableInspector.ps1') `
        -GameRoot $GameRoot -Query 'VoyageInputControlsComponent' `
        -OutputPath $existingOutput | Out-Null
}
catch {
    $overwriteRejected = $_.Exception.Message -like 'Refusing to overwrite*'
}
Assert-InspectorTest ($overwriteRejected -and
    (Get-FileHash -LiteralPath $existingOutput -Algorithm SHA256).Hash -ceq
        $existingHash) `
    'Existing inspector output was not protected before the executable scan.'
$checks.Add('existing-output-protected-before-scan')

$scanOutput = Join-Path $testRoot 'voyage-input-controls.txt'
$scanJson = & (Join-Path $PSScriptRoot 'Invoke-VoyageExecutableInspector.ps1') `
    -GameRoot $GameRoot -Query 'VoyageInputControlsComponent' `
    -WindowBytes 128 -OutputPath $scanOutput -RequireMatch -AsJson
$scan = $scanJson | ConvertFrom-Json
Assert-InspectorTest ($scan.status -ceq 'completed' -and
    $scan.matchCount -gt 0 -and
    $scan.inspectorBinarySha256 -ceq $binary.Sha256 -and
    (Test-Path -LiteralPath $scan.outputPath -PathType Leaf) -and
    (Get-FileHash -LiteralPath $scan.outputPath -Algorithm SHA256).Hash -ceq
        $scan.outputSha256) `
    'Current executable query did not return compact validated evidence.'
$checks.Add('current-executable-query-without-dotnet')

$fingerprint = (& (Join-Path $PSScriptRoot 'Get-VoyageBuildFingerprint.ps1') `
    -GameRoot $GameRoot | ConvertFrom-Json)
Assert-InspectorTest ($scan.executableSha256 -ceq
    [string]$fingerprint.executable.sha256 -and
    $scan.steamBuildId -ceq [string]$fingerprint.steam.buildId) `
    'Executable inspection did not preserve the current game identity.'
$checks.Add('current-game-identity')

$result = [pscustomobject][ordered]@{
    status = 'passed'
    checks = @($checks)
    checkCount = $checks.Count
    powerShellVersion = $PSVersionTable.PSVersion.ToString()
    inspectorBinaryPath = $binary.Path
    inspectorBinarySha256 = $binary.Sha256
    steamBuildId = [string]$fingerprint.steam.buildId
    executableSha256 = [string]$fingerprint.executable.sha256
    queryMatches = [int]$scan.matchCount
    reportPath = [string]$scan.outputPath
    reportSha256 = [string]$scan.outputSha256
    evidencePath = $testRoot
    gameFilesChanged = $false
}
[IO.File]::WriteAllText(
    (Join-Path $testRoot 'summary.json'),
    (($result | ConvertTo-Json -Depth 5) + [Environment]::NewLine),
    [Text.UTF8Encoding]::new($false))
$result
