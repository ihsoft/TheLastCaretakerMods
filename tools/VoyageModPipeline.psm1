Set-StrictMode -Version Latest
$ErrorActionPreference = 'Stop'

function Resolve-ModPath([string]$Path) {
    [IO.Path]::GetFullPath($ExecutionContext.SessionState.Path.GetUnresolvedProviderPathFromPSPath($Path))
}

function Assert-ModUnlinked([string]$Path, [switch]$Recurse) {
    for ($cursor = $Path; $cursor; $cursor = Split-Path -Parent $cursor) {
        if ((Test-Path -LiteralPath $cursor) -and ((Get-Item -LiteralPath $cursor -Force).Attributes -band [IO.FileAttributes]::ReparsePoint)) {
            throw "Linked path is unsupported: $cursor"
        }
    }
    if ($Recurse -and (Test-Path -LiteralPath $Path -PathType Container)) {
        foreach ($entry in Get-ChildItem -LiteralPath $Path -Force) {
            Assert-ModUnlinked $entry.FullName -Recurse
        }
    }
}

function Assert-ModChild([string]$Path, [string]$Parent) {
    if (-not $Path.StartsWith($Parent.TrimEnd('\','/') + [IO.Path]::DirectorySeparatorChar, [StringComparison]::OrdinalIgnoreCase)) {
        throw "Path must stay below ${Parent}: $Path"
    }
    Assert-ModUnlinked $Path
}

function Get-ModContext([string]$ModRoot) {
    $repo = Resolve-ModPath (Join-Path $PSScriptRoot '..')
    $mod = Resolve-ModPath $ModRoot
    if ((Split-Path -Parent $mod) -ne (Join-Path $repo 'mods') -or -not (Test-Path -LiteralPath $mod -PathType Container)) {
        throw "Expected an existing direct child of repository mods: $mod"
    }
    Assert-ModUnlinked $mod
    [pscustomobject]@{repo=$repo;modRoot=$mod;name=(Split-Path -Leaf $mod);owner=(Join-Path $repo ('Tmp/' + (Split-Path -Leaf $mod)))}
}

function New-VoyageModRun {
    [CmdletBinding()]
    param([Parameter(Mandatory)][string]$ModRoot, [string]$OutputRoot, [ValidatePattern('^[a-zA-Z0-9][a-zA-Z0-9._-]*$')][string]$Kind='build')
    $context = Get-ModContext $ModRoot
    if (-not $OutputRoot) { $OutputRoot = Join-Path $context.owner ($Kind + '-' + [guid]::NewGuid().ToString('N')) }
    $output = Resolve-ModPath $OutputRoot
    Assert-ModChild $output $context.owner
    if (Test-Path -LiteralPath $output) { throw "Use a fresh output directory: $output" }
    [IO.Directory]::CreateDirectory($output) | Out-Null
    $output
}

function New-VoyageModProjectCopy {
    [CmdletBinding()]
    param([Parameter(Mandatory)][string]$ModRoot, [Parameter(Mandatory)][string]$Destination)
    $context = Get-ModContext $ModRoot
    $target = Resolve-ModPath $Destination
    Assert-ModChild $target $context.owner
    if (Test-Path -LiteralPath $target) { throw "Use a fresh project destination: $target" }
    $source = Join-Path $context.modRoot 'Project'
    Assert-ModUnlinked $source
    $files = @('Voyage.uproject','EditorSupport.json')
    $directories = @('Content','Config','BlueprintText','EditorSupport','Tests')
    foreach ($required in @('Voyage.uproject','Content')) {
        if (-not (Test-Path -LiteralPath (Join-Path $source $required))) { throw "Missing maintained project input: $required" }
    }
    $entries = @(Get-ChildItem -LiteralPath $source -Force | Where-Object { $_.Name -notin @('.gitignore','Intermediate') })
    foreach ($entry in $entries) {
        if (($entry.PSIsContainer -and $entry.Name -notin $directories) -or (-not $entry.PSIsContainer -and $entry.Name -notin $files)) {
            throw "Unexpected project input: $($entry.FullName)"
        }
        Assert-ModUnlinked $entry.FullName -Recurse
    }
    [IO.Directory]::CreateDirectory($target) | Out-Null
    foreach ($entry in $entries) { Copy-Item -LiteralPath $entry.FullName -Destination $target -Recurse }
    Join-Path $target 'Voyage.uproject'
}

function Remove-VoyageModScratch {
    [CmdletBinding()]
    param([Parameter(Mandatory)][string]$ModRoot, [Parameter(Mandatory)][string]$RunRoot, [Parameter(Mandatory)][string]$Path)
    $context = Get-ModContext $ModRoot
    $run = Resolve-ModPath $RunRoot
    $target = Resolve-ModPath $Path
    Assert-ModChild $run $context.owner
    Assert-ModChild $target $run
    Assert-ModUnlinked $target -Recurse
    if (Test-Path -LiteralPath $target) { Remove-Item -LiteralPath $target -Recurse -Force }
}

function Invoke-VoyageModEditorTest {
    [CmdletBinding()]
    param(
        [Parameter(Mandatory)][string]$ModRoot, [Parameter(Mandatory)][string]$EngineRoot,
        [string]$OutputRoot, [Parameter(Mandatory)][scriptblock]$Run,
        [ValidatePattern('^[a-zA-Z0-9._-]+\.json$')][string]$ReportName='test-report.json',
        [int]$ExpectedSuiteCount=0
    )
    $owner = Get-ModContext $ModRoot
    $output = New-VoyageModRun -ModRoot $ModRoot -OutputRoot $OutputRoot -Kind test
    $workspace = Join-Path $owner.repo ('Tmp/BlueprintText/' + $owner.name + '-test-' + [guid]::NewGuid().ToString('N'))
    try {
        $project = New-VoyageModProjectCopy -ModRoot $ModRoot -Destination (Join-Path $output 'Project')
        & (Join-Path $owner.repo 'tools/BlueprintText/Invoke-BlueprintWorkspace.ps1') -Action Prepare -ProjectFile $project -EngineRoot $EngineRoot -WorkspaceRoot $workspace > (Join-Path $output 'prepare.log')
        $reportPath = Join-Path $output $ReportName
        $context = [pscustomobject]@{repo=$owner.repo;modRoot=$owner.modRoot;output=$output;project=$project;projectRoot=(Split-Path -Parent $project);reportPath=$reportPath;engineRoot=$EngineRoot}
        & $Run $context > (Join-Path $output 'test-console.log')
        if (-not (Test-Path -LiteralPath $reportPath -PathType Leaf)) { throw "Test report missing: $reportPath" }
        $report = Get-Content -LiteralPath $reportPath -Raw | ConvertFrom-Json
        if ($report.status -cne 'passed') { throw "Tests failed: $reportPath" }
        if ($ExpectedSuiteCount -gt 0 -and @($report.tests).Count -ne $ExpectedSuiteCount) { throw "Incomplete suite report: $reportPath" }
        Remove-VoyageModScratch -ModRoot $ModRoot -RunRoot $output -Path $context.projectRoot
        [pscustomobject]@{status='passed';reportPath=$reportPath;evidence=$output;bptWorkspace=$workspace}
    } catch {
        Write-Warning "Test failed; project and diagnostics remain at $output (BPT workspace: $workspace)"
        throw
    }
}

function Invoke-VoyageModCook {
    [CmdletBinding()]
    param(
        [Parameter(Mandatory)][string]$ModRoot, [Parameter(Mandatory)][string]$ProjectFile,
        [Parameter(Mandatory)][string]$EngineRoot, [Parameter(Mandatory)][string]$OutputRoot,
        [Parameter(Mandatory)][string[]]$Packages, [Parameter(Mandatory)][string]$ParserVersion,
        [string[]]$AdditionalCookArguments=@()
    )
    $context = Get-ModContext $ModRoot
    $output = Resolve-ModPath $OutputRoot
    Assert-ModChild $output $context.owner
    $project = Resolve-ModPath $ProjectFile
    Assert-ModChild $project $output
    $projectRoot = Split-Path -Parent $project
    if (-not $Packages.Count -or @($Packages | Select-Object -Unique).Count -ne $Packages.Count) { throw 'Expected a nonempty, unique package inventory' }
    foreach ($package in $Packages) {
        if ($package -cnotmatch '^/Game/(?:[A-Za-z0-9_]+/)*[A-Za-z0-9_]+$') { throw "Unsupported package identity: $package" }
        $source = Join-Path $projectRoot ('Content/' + $package.Substring(6) + '.uasset')
        if (-not (Test-Path -LiteralPath $source -PathType Leaf)) { throw "Maintained asset missing: $source" }
    }
    $log = Join-Path $output 'cook.log'
    $arguments = @($project,'-run=cook','-targetplatform=Windows','-unversioned','-SkipZenStore',('-Package=' + ($Packages -join '+')),'-CookSinglePackageNoRefs','-unattended','-nop4','-nosplash','-nullrhi','-ddc=NoZenLocalFallback') + $AdditionalCookArguments + @('-abslog=' + $log)
    & (Join-Path $context.repo 'tools/Invoke-UnrealQueued.ps1') -Operation ('Cook ' + $context.name) -FilePath (Join-Path $EngineRoot 'Engine/Binaries/Win64/UnrealEditor-Cmd.exe') -ArgumentList $arguments > (Join-Path $output 'cook-console.log')
    if (-not (Test-Path -LiteralPath $log -PathType Leaf)) { throw "Cook log missing: $log" }
    if (Select-String -LiteralPath $log -Pattern 'LogBlueprint: Error:|LogLinker: Error:|LogCook: Error:' -Quiet) { throw "Cook errors: $log" }
    $stage = Join-Path $output 'stage'
    $payload = Join-Path $output 'package'
    foreach ($directory in @($stage,$payload)) {
        Assert-ModChild $directory $output
        if (Test-Path -LiteralPath $directory) { throw "Cook output already exists: $directory" }
        [IO.Directory]::CreateDirectory($directory) | Out-Null
    }
    foreach ($package in $Packages) {
        $relative = 'Voyage/Content/' + $package.Substring(6)
        foreach ($extension in @('.uasset','.uexp','.ubulk','.uptnl')) {
            $source = Join-Path $projectRoot ('Saved/Cooked/Windows/' + $relative + $extension)
            if ($extension -in @('.ubulk','.uptnl') -and -not (Test-Path -LiteralPath $source)) { continue }
            $destination = Join-Path $stage ($relative + $extension)
            [IO.Directory]::CreateDirectory((Split-Path -Parent $destination)) | Out-Null
            Copy-Item -LiteralPath $source -Destination $destination
        }
    }
    $container = Join-Path $payload ($context.name + '.utoc')
    & (Join-Path $context.repo '.tools/bin/retoc.exe') to-zen --version $ParserVersion $stage $container > (Join-Path $output 'retoc.log')
    if ($LASTEXITCODE -ne 0) { throw 'retoc conversion failed; see retoc.log' }
    $expected = Join-Path $output 'expected-packages.txt'
    @($Packages | ForEach-Object { '../../../Voyage/Content/' + $_.Substring(6) + '.uasset' }) | Set-Content -LiteralPath $expected -Encoding ASCII
    $validation = & (Join-Path $context.repo 'tools/Test-VoyageContainer.ps1') -Container $container -ExpectedPackageList $expected -OutputRoot (Join-Path $output 'container-check')
    [pscustomobject]@{container=$container;stage=$stage;validation=$validation}
}

Export-ModuleMember -Function New-VoyageModRun,New-VoyageModProjectCopy,Remove-VoyageModScratch,Invoke-VoyageModEditorTest,Invoke-VoyageModCook
