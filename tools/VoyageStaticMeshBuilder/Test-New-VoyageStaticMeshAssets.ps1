[CmdletBinding()]
param(
    [string]$EngineRoot = 'K:\Epic Games\UE_5.8\Engine',
    [string]$OutputRoot
)

$ErrorActionPreference = 'Stop'
Set-StrictMode -Version Latest
$repo = (Resolve-Path -LiteralPath (Join-Path $PSScriptRoot '..\..')).Path
if ([string]::IsNullOrWhiteSpace($OutputRoot)) {
    $OutputRoot = Join-Path $repo ('Tmp\tools\VoyageStaticMeshBuilder\test-' +
        [DateTime]::UtcNow.ToString('yyyyMMdd-HHmmss'))
}
$root = [IO.Path]::GetFullPath($OutputRoot)
if (Test-Path -LiteralPath $root) { throw "OutputRoot already exists: $root" }
$null = New-Item -ItemType Directory -Path $root

function Write-Json([string]$Path, $Value) {
    [IO.File]::WriteAllText($Path, ($Value | ConvertTo-Json -Depth 20),
        (New-Object Text.UTF8Encoding($false)))
}
function Invoke-Case([string]$Name, [string]$Manifest, [bool]$ExpectSuccess) {
    $stdout = Join-Path $root "$Name.stdout.log"
    $stderr = Join-Path $root "$Name.stderr.log"
    $evidence = Join-Path $root "$Name-evidence"
    $output = Join-Path $root "$Name-output"
    $arguments = @('-NoProfile','-ExecutionPolicy','Bypass','-File',
        ('"{0}"' -f (Join-Path $repo 'tools\New-VoyageStaticMeshAssets.ps1')),
        '-Manifest',('"{0}"' -f $Manifest),'-OutputRoot',('"{0}"' -f $output),
        '-EvidenceRoot',('"{0}"' -f $evidence),'-EngineRoot',('"{0}"' -f $EngineRoot))
    $process = Start-Process -FilePath 'powershell.exe' -ArgumentList $arguments `
        -Wait -PassThru -WindowStyle Hidden -RedirectStandardOutput $stdout `
        -RedirectStandardError $stderr
    if ($ExpectSuccess -and $process.ExitCode -ne 0) {
        throw "$Name unexpectedly failed ($($process.ExitCode)); logs: $stdout, $stderr"
    }
    if (-not $ExpectSuccess -and $process.ExitCode -eq 0) {
        throw "$Name unexpectedly passed."
    }
    [pscustomobject]@{Name=$Name;ExitCode=$process.ExitCode;Evidence=$evidence;Output=$output;Stdout=$stdout;Stderr=$stderr}
}

$python = Join-Path $EngineRoot 'Binaries\ThirdParty\Python3\Win64\python.exe'
if (-not (Test-Path -LiteralPath $python -PathType Leaf)) {
    throw "Bundled Unreal Python is missing: $python"
}
$fixtures = Join-Path $root 'fixtures'
& $python (Join-Path $PSScriptRoot 'Build-TestStaticMeshFixtures.py') --output $fixtures
if ($LASTEXITCODE -ne 0) { throw 'Fixture generation failed.' }

$invalidManifest = Join-Path $root 'invalid.json'
Write-Json $invalidManifest ([ordered]@{schemaVersion=1;unexpected=$true;assets=@()})
$invalid = Invoke-Case 'invalid' $invalidManifest $false
$invalidFailure = Get-Content -LiteralPath (Join-Path $invalid.Evidence 'failure.json') -Raw |
    ConvertFrom-Json
if ($invalidFailure.message -cnotmatch "Unknown field 'unexpected'" -or
    (Test-Path -LiteralPath (Join-Path $invalid.Evidence 'generate.log'))) {
    throw 'Strict invalid-manifest preflight evidence is missing or Unreal launched.'
}

$ambiguousManifest = Join-Path $root 'ambiguous.json'
Write-Json $ambiguousManifest ([ordered]@{schemaVersion=1;assets=@([ordered]@{
    id='ambiguous';source='fixtures/multi.glb';destination=[ordered]@{
        packagePath='/Game/Mods/BuilderTest/SM_Ambiguous';objectName='SM_Ambiguous'
    }
})})
$ambiguous = Invoke-Case 'ambiguous' $ambiguousManifest $false
$selection = Get-Content -LiteralPath (Join-Path $ambiguous.Evidence 'editor-summary.json') -Raw |
    ConvertFrom-Json
if ($selection.errorCode -cne 'mesh-selection' -or
    (@($selection.discoveredMeshes) -join ',') -cne 'Alpha,Beta') {
    throw 'Multi-mesh ambiguity did not return the exact discovered selector names.'
}

$batchManifest = Join-Path $root 'batch.json'
$countAssertions = [ordered]@{
    sourceTriangles=[ordered]@{exact=1}
    naniteInputTriangles=[ordered]@{exact=1}
    fallbackTriangles=[ordered]@{exact=1}
}
Write-Json $batchManifest ([ordered]@{schemaVersion=1;assets=@(
    [ordered]@{id='single-default';source='fixtures/single.glb';destination=[ordered]@{packagePath='/Game/Mods/BuilderTest/SM_SingleDefault';objectName='SM_SingleDefault'};assertions=$countAssertions},
    [ordered]@{id='single-reference';source='fixtures/single.glb';destination=[ordered]@{packagePath='/Game/Mods/BuilderTest/SM_SingleReference';objectName='SM_SingleReference'};materialMode='ExternalReferences';materialBindings=@([ordered]@{slot='SlotA';objectPath='/Game/Mods/BuilderTest/Materials/M_External.M_External';classPath='/Script/Engine.MaterialInstanceConstant'});assertions=$countAssertions},
    [ordered]@{id='multi-alpha';source='fixtures/multi.glb';sourceMesh='Alpha';destination=[ordered]@{packagePath='/Game/Mods/BuilderTest/SM_Alpha';objectName='SM_Alpha'}},
    [ordered]@{id='multi-beta';source='fixtures/multi.glb';sourceMesh='Beta';destination=[ordered]@{packagePath='/Game/Mods/BuilderTest/SM_Beta';objectName='SM_Beta'}},
    [ordered]@{id='nanite-disabled';source='fixtures/single.glb';destination=[ordered]@{packagePath='/Game/Mods/BuilderTest/SM_NaniteDisabled';objectName='SM_NaniteDisabled'};nanite=[ordered]@{enabled=$false};rayTracing=[ordered]@{support=$true};assertions=[ordered]@{sourceTriangles=[ordered]@{exact=1};naniteInputTriangles=[ordered]@{exact=0};fallbackTriangles=[ordered]@{exact=1}}}
)})
$batch = Invoke-Case 'batch' $batchManifest $true
$resultPath = Join-Path $batch.Output 'build-result.json'
$result = Get-Content -LiteralPath $resultPath -Raw | ConvertFrom-Json
if ($result.status -cne 'passed' -or @($result.assets).Count -ne 5) {
    throw 'Batch result is incomplete.'
}
$byId = @{}
foreach ($asset in $result.assets) { $byId[[string]$asset.id] = $asset }
if ($byId['single-default'].materialMode -cne 'DefaultEngine' -or
    @($byId['single-default'].resolvedExternalMaterialReferences).Count -ne 0 -or
    $byId['single-reference'].materialMode -cne 'ExternalReferences' -or
    (@($byId['single-reference'].resolvedExternalMaterialReferences) -join ',') -cne
        '/Game/Mods/BuilderTest/Materials/M_External.M_External' -or
    $byId['multi-alpha'].selectedSourceMesh -cne 'Alpha' -or
    $byId['multi-beta'].selectedSourceMesh -cne 'Beta' -or
    $byId['nanite-disabled'].naniteInputTriangles -ne 0 -or
    $byId['nanite-disabled'].naniteRootPages -ne 0 -or
    $byId['nanite-disabled'].fallbackTriangles -ne 1 -or
    $byId['nanite-disabled'].rayTracingProxyLods -ne 1 -or
    $byId['nanite-disabled'].rayTracingUsesRenderingLods -ne $true) {
    throw 'Material mode/reference or multi-mesh selector result differs from the contract.'
}
foreach ($asset in @($result.assets | Where-Object id -ne 'nanite-disabled')) {
    if ($asset.sourceTriangles -ne 1 -or $asset.sourceUvChannels -ne 1 -or
        $asset.sourceHasNonWhiteVertexColors -ne $true -or
        $asset.naniteInputTriangles -ne 1 -or $asset.naniteRootPages -le 0 -or
        $asset.rayTracingProxyLods -le 0) {
        throw "Source/build attribute contract failed for $($asset.id)."
    }
}
$contentFiles = @(Get-ChildItem -LiteralPath (Join-Path $batch.Output 'Content') -File -Recurse)
if ($contentFiles.Count -ne 15) { throw "Expected 15 cooked companions; found $($contentFiles.Count)." }

$report = [ordered]@{
    schemaVersion=1;status='passed';createdAtUtc=[DateTime]::UtcNow.ToString('o')
    invalidManifestFailure=Join-Path $invalid.Evidence 'failure.json'
    ambiguousSelectionSummary=Join-Path $ambiguous.Evidence 'editor-summary.json'
    batchResult=$resultPath
    outputManifest=Join-Path $batch.Output 'output-manifest.json'
    fixtures=@(Get-ChildItem -LiteralPath $fixtures -File | Sort-Object Name | ForEach-Object {
        [ordered]@{name=$_.Name;length=$_.Length;sha256=(Get-FileHash -LiteralPath $_.FullName -Algorithm SHA256).Hash}
    })
    limits=@('This builder test verifies generated and cooked structure plus producer-reported material references. Independent cooked-package inspection remains a separate validator responsibility.')
}
$reportPath = Join-Path $root 'test-report.json'
Write-Json $reportPath $report
[pscustomobject]@{status='passed';reportPath=$reportPath;batchOutput=$batch.Output} |
    ConvertTo-Json -Compress
