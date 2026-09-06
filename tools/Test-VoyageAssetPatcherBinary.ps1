# Read-only against the installed game; diagnostics stay under artifacts/tests.
[CmdletBinding()]
param(
    [string]$GameRoot = 'P:\SteamLibrary\steamapps\common\Voyage'
)

$ErrorActionPreference = 'Stop'
Set-StrictMode -Version Latest

$repositoryRoot = (Resolve-Path -LiteralPath (Join-Path $PSScriptRoot '..')).Path
$testRoot = Join-Path $repositoryRoot (
    'artifacts\tests\asset-patcher-binary-' + [Guid]::NewGuid().ToString('N'))
[IO.Directory]::CreateDirectory($testRoot) | Out-Null
$checks = [Collections.Generic.List[string]]::new()

function Assert-PatcherTest {
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

function Assert-Rejected {
    param(
        [Parameter(Mandatory = $true)]
        [scriptblock]$Action,

        [Parameter(Mandatory = $true)]
        [string]$Pattern,

        [Parameter(Mandatory = $true)]
        [string]$Check
    )

    try {
        & $Action
    }
    catch {
        if ($_.Exception.Message -notmatch $Pattern) {
            throw
        }
        $checks.Add($Check)
        return
    }
    throw "Expected rejection did not occur: $Pattern"
}

# Any regression to source-project launch in a normal path fails immediately.
function dotnet {
    throw 'Normal VoyageAssetPatcher operation attempted dotnet.'
}
function dotnet.exe {
    throw 'Normal VoyageAssetPatcher operation attempted dotnet.exe.'
}

foreach ($name in @(
    'Get-VoyageAssetPatcherBinary.ps1',
    'Invoke-VoyageAssetPatcher.ps1',
    'Publish-VoyageAssetPatcherBinary.ps1')) {
    $tokens = $null
    $parseErrors = $null
    [void][Management.Automation.Language.Parser]::ParseFile(
        (Join-Path $PSScriptRoot $name),
        [ref]$tokens,
        [ref]$parseErrors)
    Assert-PatcherTest ($parseErrors.Count -eq 0) "PowerShell parse failure: $name"
}
$checks.Add('public-script-parse')

$resolver = Join-Path $PSScriptRoot 'Get-VoyageAssetPatcherBinary.ps1'
$publisher = Join-Path $PSScriptRoot 'Publish-VoyageAssetPatcherBinary.ps1'
$binary = & $resolver
$beforeTime = (Get-Item -LiteralPath $binary.Path).LastWriteTimeUtc
$published = & $publisher
Assert-PatcherTest (-not [bool]$published.Rebuilt -and
    [string]$published.Sha256 -ceq [string]$binary.Sha256 -and
    (Get-Item -LiteralPath $binary.Path).LastWriteTimeUtc -eq $beforeTime) `
    'Publisher rebuilt a current VoyageAssetPatcher binary.'
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
Assert-PatcherTest ($usageExitCode -eq 2 -and
    $usage -match 'Usage: VoyageAssetPatcher') `
    'Canonical VoyageAssetPatcher usage smoke failed.'
$checks.Add('canonical-executable-smoke')

$fingerprintScript = Join-Path $PSScriptRoot 'Get-VoyageBuildFingerprint.ps1'
$beforeFingerprint = & $fingerprintScript -GameRoot $GameRoot | ConvertFrom-Json
$mapping = & (Join-Path $PSScriptRoot 'Get-VoyageMappings.ps1') -GameRoot $GameRoot
$extraction = & (Join-Path $PSScriptRoot 'Extract-VoyagePackage.ps1') `
    -GameRoot $GameRoot -Filter 'Blueprints/BP_VoyageCableUpdater' `
    -OutputRoot (Join-Path $testRoot 'extracted') 6>$null
$inputAssets = @(Get-ChildItem -LiteralPath $extraction.outputPath -Recurse -File `
    -Filter 'BP_VoyageCableUpdater.uasset')
Assert-PatcherTest ($inputAssets.Count -eq 1) `
    'Exact BP_VoyageCableUpdater extraction did not return one asset.'
$checks.Add('public-current-asset-inputs')

$roundtripOutput = Join-Path $testRoot 'roundtrip\BP_VoyageCableUpdater.uasset'
$roundtrip = & (Join-Path $PSScriptRoot 'Invoke-VoyageAssetPatcher.ps1') `
    -Operation roundtrip-unchanged -InputAsset $inputAssets[0].FullName `
    -OutputAsset $roundtripOutput -GameRoot $GameRoot `
    -EvidenceRoot (Join-Path $testRoot 'tool-runs')
$roundtripCompanion = [IO.Path]::ChangeExtension($roundtripOutput, '.uexp')
Assert-PatcherTest ($roundtrip.status -ceq 'completed' -and
    $roundtrip.mappingSource -ceq 'reviewed-current-game' -and
    @($roundtrip.inputs).Count -ge 2 -and
    -not [string]::IsNullOrWhiteSpace([string]$roundtrip.mappingManifestPath) -and
    $roundtrip.patcherBinarySha256 -ceq $binary.Sha256 -and
    (Test-Path -LiteralPath $roundtripOutput -PathType Leaf) -and
    (Test-Path -LiteralPath $roundtripCompanion -PathType Leaf)) `
    "Canonical unchanged roundtrip failed. Log: $($roundtrip.logPath)"
$checks.Add('current-asset-roundtrip-without-dotnet')

$jsonOutput = Join-Path $testRoot 'json\BP_VoyageCableUpdater.json'
$jsonText = & (Join-Path $PSScriptRoot 'Invoke-VoyageAssetPatcher.ps1') `
    -Operation export-json -InputAsset $inputAssets[0].FullName `
    -OutputAsset $jsonOutput -GameRoot $GameRoot `
    -EvidenceRoot (Join-Path $testRoot 'tool-runs') -AsJson
$jsonResult = $jsonText | ConvertFrom-Json
Assert-PatcherTest ($jsonResult.status -ceq 'completed' -and
    $jsonResult.mappingSource -ceq 'reviewed-current-game' -and
    @($jsonResult.outputs).Count -eq 1 -and
    (Test-Path -LiteralPath $jsonOutput -PathType Leaf)) `
    'Compact JSON patcher result is incomplete.'
$checks.Add('compact-json-result')

$protectedOutput = Join-Path $testRoot 'protected\BP_VoyageCableUpdater.uasset'
[IO.Directory]::CreateDirectory([IO.Path]::GetDirectoryName($protectedOutput)) | Out-Null
$protectedCompanion = [IO.Path]::ChangeExtension($protectedOutput, '.uexp')
[IO.File]::WriteAllText($protectedCompanion, 'must-not-change')
$protectedHash = (Get-FileHash -LiteralPath $protectedCompanion -Algorithm SHA256).Hash
Assert-Rejected -Check 'existing-companion-protected' `
    -Pattern 'Refusing to overwrite an existing output or companion' -Action {
    & (Join-Path $PSScriptRoot 'Invoke-VoyageAssetPatcher.ps1') `
        -Operation roundtrip-unchanged -InputAsset $inputAssets[0].FullName `
        -OutputAsset $protectedOutput -GameRoot $GameRoot
}
Assert-PatcherTest ((Get-FileHash -LiteralPath $protectedCompanion `
    -Algorithm SHA256).Hash -ceq $protectedHash) `
    'Rejected output companion changed.'

Assert-Rejected -Check 'legacy-mapping-must-be-explicit' `
    -Pattern 'Legacy UE5_7 operations require' -Action {
    & (Join-Path $PSScriptRoot 'Invoke-VoyageAssetPatcher.ps1') `
        -Operation export-json -InputAsset $inputAssets[0].FullName `
        -OutputAsset (Join-Path $testRoot 'legacy.json') -EngineVersion UE5_7 `
        -GameRoot $GameRoot
}

$afterFingerprint = & $fingerprintScript -GameRoot $GameRoot | ConvertFrom-Json
$beforeContainers = @($beforeFingerprint.containers | Sort-Object name |
    ForEach-Object { [string]$_.name + '=' + [string]$_.sha256 })
$afterContainers = @($afterFingerprint.containers | Sort-Object name |
    ForEach-Object { [string]$_.name + '=' + [string]$_.sha256 })
Assert-PatcherTest (
    [string]$beforeFingerprint.executable.sha256 -ceq
        [string]$afterFingerprint.executable.sha256 -and
    @(Compare-Object $beforeContainers $afterContainers).Count -eq 0) `
    'Installed Voyage files changed during the read-only patcher regression.'
$checks.Add('installed-game-unchanged')

$result = [pscustomobject][ordered]@{
    status = 'passed'
    checks = @($checks)
    checkCount = $checks.Count
    powerShellVersion = $PSVersionTable.PSVersion.ToString()
    patcherBinaryPath = $binary.Path
    patcherBinarySha256 = $binary.Sha256
    mappingSha256 = $mapping.sha256
    steamBuildId = [string]$beforeFingerprint.steam.buildId
    executableSha256 = [string]$beforeFingerprint.executable.sha256
    roundtripOutput = $roundtripOutput
    evidencePath = $testRoot
    gameFilesChanged = $false
}
[IO.File]::WriteAllText(
    (Join-Path $testRoot 'summary.json'),
    (($result | ConvertTo-Json -Depth 5) + [Environment]::NewLine),
    [Text.UTF8Encoding]::new($false))
$result
