[CmdletBinding()]
param()
$ErrorActionPreference = 'Stop'
Set-StrictMode -Version Latest

# Exercise the actual producer completion blocks with a fake installer. No UE,
# game files, release manifests or installed containers are changed by this test.
$cases = 0
foreach ($relative in @('Build-Shell.ps1', 'AutoloadProbe/Build-Probe.ps1')) {
    $path = Join-Path $PSScriptRoot $relative
    $source = Get-Content -LiteralPath $path -Raw
    $tokens = $null; $errors = $null
    $ast = [Management.Automation.Language.Parser]::ParseFile($path, [ref]$tokens, [ref]$errors)
    if ($errors.Count) { throw ($errors | Out-String) }
    if ('Install' -notin @($ast.ParamBlock.Parameters | ForEach-Object { $_.Name.VariablePath.UserPath })) { throw 'Missing Install switch.' }
    $marker = '$installation = $null'
    $offset = $source.LastIndexOf($marker, [StringComparison]::Ordinal)
    if ($offset -lt $source.IndexOf("'Manifest producer did not publish the candidate.'")) { throw 'Install must follow manifest validation.' }
    $completion = [scriptblock]::Create($source.Substring($offset))
    foreach ($mode in @('prepare', 'install', 'failure')) {
        & {
            $Install = $mode -ne 'prepare'
            $StationPrototype = $false
            $repo = 'test-repository'; $manifestPath = 'test-manifest'
            $archivePath = 'test-archive'; $verify = @{reportPath='test-verification'}
            $script:shortcutInstallCalls = 0
            function Join-Path { param($Path, $ChildPath)
                if ($ChildPath -ne 'tools/Install-VoyageRelease.ps1') { throw 'Unexpected installer path.' }
                'Invoke-TestInstaller'
            }
            function Invoke-TestInstaller { param($ReleaseManifest, [switch]$AllowDirtySource)
                $script:shortcutInstallCalls++
                if ($ReleaseManifest -ne 'test-manifest' -or -not $AllowDirtySource) { throw 'Incorrect installer arguments.' }
                if ($mode -eq 'failure') { throw 'Expected installer rejection.' }
                [pscustomobject]@{installManifestPath='test-receipt';fileCount=4}
            }
            $failed = $false
            try { $result = (& $completion) | ConvertFrom-Json }
            catch {
                if ($mode -ne 'failure' -or $_.Exception.Message -ne 'Expected installer rejection.') { throw }
                $failed = $true
            }
            if ($mode -eq 'failure') {
                if (-not $failed -or $script:shortcutInstallCalls -ne 1) { throw 'Installer failure was swallowed.' }
            } elseif ($mode -eq 'install') {
                if ($script:shortcutInstallCalls -ne 1 -or $result.status -ne 'installed' -or $result.installation.installManifestPath -ne 'test-receipt') { throw 'Install result mismatch.' }
            } elseif ($script:shortcutInstallCalls -ne 0 -or $result.status -ne 'prepared-not-installed' -or $null -ne $result.installation) {
                throw 'Preparation unexpectedly installs.'
            }
        }
        $cases++
    }
}
$shell = Get-Content -LiteralPath (Join-Path $PSScriptRoot 'Build-Shell.ps1') -Raw
if ($shell.Contains('$model.source.sha256') -or $shell.Contains('$model.source.byteLength')) { throw 'Stale model revision gate remains.' }
if (-not $shell.Contains('Source content changed during preparation.')) { throw 'Mid-build source stability gate missing.' }
$cases++
$settingsTemplate = Join-Path $PSScriptRoot 'Assets/HarpoonCannon.ini'
if (-not (Test-Path -LiteralPath $settingsTemplate -PathType Leaf)) { throw 'Distribution settings template missing.' }
$station = Get-Content -LiteralPath (Join-Path $PSScriptRoot 'AutoloadProbe/Build-Probe.ps1') -Raw
if (-not $station.Contains("mods/HarpoonCannon/Assets/HarpoonCannon.ini") -or
    -not $station.Contains("../Assets/HarpoonCannon.ini")) { throw 'Station producer does not own the distribution settings template.' }
$cases++
[pscustomobject]@{status='passed';cases=$cases;boundary='mocked installer; no game mutation or Unreal build'} | ConvertTo-Json -Compress
