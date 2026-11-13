param([string]$OutputRoot)

$ErrorActionPreference = 'Stop'
Set-StrictMode -Version Latest
$repo = [IO.Path]::GetFullPath((Join-Path $PSScriptRoot '../..'))
$temporary = [IO.Path]::GetFullPath((Join-Path $repo 'Tmp')) + [IO.Path]::DirectorySeparatorChar
if (-not $OutputRoot) {
    $OutputRoot = Join-Path $temporary ('tests/material-output-paths/' + [guid]::NewGuid().ToString('N'))
}
$run = [IO.Path]::GetFullPath($OutputRoot)
if (-not $run.StartsWith($temporary, [StringComparison]::OrdinalIgnoreCase) -or (Test-Path -LiteralPath $run)) {
    throw 'OutputRoot must be a fresh directory below repository Tmp.'
}

# Exercise the unmodified public entry points in an isolated repository layout.
# Stop at game identification: this checks path gates without exporting game data.
$fixture = Join-Path $run 'repository'
$fixtureTools = Join-Path $fixture 'tools'
$bundle = Join-Path $fixture '.tools/bin/VoyageMaterialLibrary'
[void](New-Item -ItemType Directory -Path $fixtureTools -Force)
[void](New-Item -ItemType Directory -Path $bundle -Force)
[IO.File]::WriteAllText((Join-Path $bundle 'publish-manifest.json'), '{"kind":"VoyageMaterialLibrary","files":[]}')
[IO.File]::WriteAllText((Join-Path $fixtureTools 'Get-VoyageBuildFingerprint.ps1'), "throw 'OUTPUT_PATH_ACCEPTED'")
$blender = Join-Path $fixture 'unused-blender.exe'
[IO.File]::WriteAllText($blender, 'Never executed: the fingerprint stub stops the call first.')
$wrappers = @(
    @{ Script = 'Export-VoyageMaterialsGlb.ps1'; Name = 'sample.glb'; Multi = $true; Blender = $false },
    @{ Script = 'Export-VoyageMaterialPreview.ps1'; Name = 'sample.png'; Multi = $true; Blender = $true },
    @{ Script = 'Export-VoyageMaterialPack.ps1'; Name = 'MI_Sample.materialpack.zip'; Multi = $false; Blender = $true },
    @{ Script = 'Export-VoyageMaterialBundle.ps1'; Name = 'Samples.materialbundle.zip'; Multi = $true; Blender = $true }
)
foreach ($wrapper in $wrappers) {
    Copy-Item -LiteralPath (Join-Path $repo ('tools/' + $wrapper.Script)) -Destination $fixtureTools
}
$checks = 0
$previousDirectory = [Environment]::CurrentDirectory
Push-Location $fixture
[Environment]::CurrentDirectory = $fixture
try {
    foreach ($wrapper in $wrappers) {
        $parameters = @{ MaterialMode = 'PbrApproximation' }
        if ($wrapper.Multi) { $parameters.Materials = @('/Game/Test/MI_Sample') }
        else { $parameters.Material = '/Game/Test/MI_Sample' }
        if ($wrapper.Blender) { $parameters.BlenderPath = $blender }
        if ($wrapper.Script -eq 'Export-VoyageMaterialBundle.ps1') { $parameters.BundleName = 'Samples' }
        $owner = [IO.Path]::GetFileNameWithoutExtension($wrapper.Script)
        $cases = @(
            @{ Directory = "Tmp/$owner/relative"; Expected = 'OUTPUT_PATH_ACCEPTED' },
            @{ Directory = (Join-Path $fixture "Tmp/$owner/absolute"); Expected = 'OUTPUT_PATH_ACCEPTED' },
            @{ Directory = "artifacts/$owner/retained"; Expected = 'OUTPUT_PATH_ACCEPTED' },
            @{ Directory = "TmpSibling/$owner"; Expected = 'Game-derived*must be a fresh*' },
            @{ Directory = "artifactsSibling/$owner"; Expected = 'Game-derived*must be a fresh*' },
            @{ Directory = "Tmp/../outside/$owner"; Expected = 'Game-derived*must be a fresh*' },
            @{ Directory = "Tmp/$owner/wrong-name"; Name = 'wrong.txt'; Expected = 'Game-derived*must be a fresh*' },
            @{ Directory = "Tmp/$owner/existing-output"; Existing = 'output'; Expected = 'Output/evidence already exists*' },
            @{ Directory = "Tmp/$owner/existing-evidence"; Existing = 'evidence'; Expected = 'Output/evidence already exists*' }
        )
        foreach ($case in $cases) {
            $name = if ($case.ContainsKey('Name')) { $case.Name } else { $wrapper.Name }
            $parameters.OutputPath = Join-Path $case.Directory $name
            $output = [IO.Path]::GetFullPath($parameters.OutputPath)
            $sentinel = $null
            if ($case.ContainsKey('Existing')) {
                $sentinel = if ($case.Existing -eq 'output') { $output } else { Join-Path ($output + '.evidence') 'keep.txt' }
                [void](New-Item -ItemType Directory -Path (Split-Path $sentinel) -Force)
                [IO.File]::WriteAllText($sentinel, 'preserve existing data')
            }
            $failure = $null
            try { & (Join-Path $fixtureTools $wrapper.Script) @parameters | Out-Null }
            catch { $failure = $_.Exception.Message }
            if (-not $failure -or $failure -notlike $case.Expected) {
                throw "$($wrapper.Script) [$($case.Directory)]: expected '$($case.Expected)', got '$failure'."
            }
            if ($sentinel -and [IO.File]::ReadAllText($sentinel) -cne 'preserve existing data') {
                throw "Existing data changed: $sentinel"
            }
            if ($case.Expected -like 'Game-derived*' -and
                ((Test-Path -LiteralPath $output) -or (Test-Path -LiteralPath ($output + '.evidence')))) {
                throw "Rejected path was written: $output"
            }
            $checks++
        }
    }
} finally {
    [Environment]::CurrentDirectory = $previousDirectory
    Pop-Location
}
[pscustomobject]@{ status = 'passed'; checks = $checks; evidence = $run } | ConvertTo-Json -Compress
