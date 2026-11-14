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

# One inventory drives cook, staging and final container verification.
function Assert-ModPackageInventory([string[]]$Packages) {
    if (-not $Packages.Count -or @($Packages | Sort-Object -Unique).Count -ne $Packages.Count) {
        throw 'Expected a nonempty, unique package inventory'
    }
    foreach ($package in $Packages) {
        if ($package -cnotmatch '^/Game/(?:[A-Za-z0-9_]+/)*[A-Za-z0-9_]+$') { throw "Unsupported package identity: $package" }
    }
}

function Invoke-VoyageModCookAssets {
    [CmdletBinding()]
    param(
        [Parameter(Mandatory)][string]$ModRoot, [Parameter(Mandatory)][string]$ProjectFile,
        [Parameter(Mandatory)][string]$EngineRoot, [Parameter(Mandatory)][string]$OutputRoot,
        [Parameter(Mandatory)][string[]]$Packages, [string[]]$TaggedPackages=@(),
        [string[]]$AdditionalCookArguments=@()
    )
    $context = Get-ModContext $ModRoot
    $output = Resolve-ModPath $OutputRoot
    Assert-ModChild $output $context.owner
    $project = Resolve-ModPath $ProjectFile
    Assert-ModChild $project $output
    $projectRoot = Split-Path -Parent $project
    Assert-ModPackageInventory $Packages
    if (@($TaggedPackages | Sort-Object -Unique).Count -ne $TaggedPackages.Count) { throw 'Duplicated tagged package' }
    foreach ($package in $TaggedPackages) {
        if ($Packages -cnotcontains $package) { throw "Tagged package is outside the release inventory: $package" }
    }
    foreach ($argument in $AdditionalCookArguments) {
        if ($argument -match '^-(?:run|RunAsCookCommandlet|Package|PackageManifest|targetplatform|abslog|unversioned|CookDir|CookAll|Map|CookSinglePackageNoRefs|SkipZenStore)(?:=|$)') {
            throw "Cook selection and serialization are owned by the shared pipeline: $argument"
        }
    }
    foreach ($package in $Packages) {
        $source = Join-Path $projectRoot ('Content/' + $package.Substring(6) + '.uasset')
        Assert-ModUnlinked $source
        if (-not (Test-Path -LiteralPath $source -PathType Leaf)) { throw "Maintained asset missing: $source" }
    }
    $stage = Join-Path $output 'stage'
    Assert-ModChild $stage $output
    if (Test-Path -LiteralPath $stage) { throw "Cook output already exists: $stage" }
    [IO.Directory]::CreateDirectory($stage) | Out-Null
    & (Join-Path $context.repo 'tools/Prepare-UnrealEditorSupport.ps1') -ProjectFile $project `
        -PluginSource (Join-Path $PSScriptRoot 'UnrealCookSupport/UnrealCookSupport.uplugin') `
        -EngineRoot $EngineRoot -OutputRoot (Join-Path $output 'cook-support') > (Join-Path $output 'cook-support.log')
    $groups = @(
        @{name='unversioned';packages=@($Packages | Where-Object { $TaggedPackages -cnotcontains $_ });arguments=@('-unversioned')},
        @{name='tagged';packages=@($TaggedPackages);arguments=@()}
    )
    $projectName = [IO.Path]::GetFileNameWithoutExtension($project)
    $cookedRoot = Join-Path $projectRoot ('Saved/Cooked/Windows/' + $projectName + '/Content')
    foreach ($group in $groups) {
        if (-not $group.packages.Count) { continue }
        $manifest = Join-Path $output ('cook-' + $group.name + '-packages.txt')
        [IO.File]::WriteAllLines($manifest, [string[]]$group.packages, (New-Object Text.UTF8Encoding($false)))
        $log = Join-Path $output ('cook-' + $group.name + '.log')
        $arguments = @($project,'-run=CookVoyagePackages','-RunAsCookCommandlet',('-PackageManifest=' + $manifest),
            '-targetplatform=Windows','-SkipZenStore','-CookSinglePackageNoRefs',
            '-unattended','-nop4','-nosplash','-nullrhi','-ddc=NoZenLocalFallback') +
            $group.arguments + $AdditionalCookArguments + @('-abslog=' + $log)
        & (Join-Path $context.repo 'tools/Invoke-UnrealQueued.ps1') -Operation ('Cook ' + $context.name) `
            -FilePath (Join-Path $EngineRoot 'Engine/Binaries/Win64/UnrealEditor-Cmd.exe') `
            -ArgumentList $arguments > (Join-Path $output ('cook-' + $group.name + '-console.log'))
        if (-not (Test-Path -LiteralPath $log -PathType Leaf)) { throw "Cook log missing: $log" }
        if (Select-String -LiteralPath $log -Pattern 'LogBlueprint: Error:|LogLinker: Error:|LogCook: Error:|LogStaticMesh: Error: Bad MeshDescription' -Quiet) {
            throw "Cook errors: $log"
        }
        # The next cook can clear Saved/Cooked; stage each completed group first.
        foreach ($package in $group.packages) {
            $relative = $package.Substring(6)
            foreach ($extension in @('.uasset','.uexp','.ubulk','.uptnl','.m.ubulk')) {
                $source = Join-Path $cookedRoot ($relative + $extension)
                if ($extension -notin @('.uasset','.uexp') -and -not (Test-Path -LiteralPath $source)) { continue }
                $destination = Join-Path $stage ('Voyage/Content/' + $relative + $extension)
                [IO.Directory]::CreateDirectory((Split-Path -Parent $destination)) | Out-Null
                Copy-Item -LiteralPath $source -Destination $destination
            }
        }
    }
    [pscustomobject]@{stage=$stage;packages=$Packages;taggedPackages=$TaggedPackages}
}

function New-VoyageModContainer {
    [CmdletBinding()]
    param(
        [Parameter(Mandatory)][string]$ModRoot, [Parameter(Mandatory)][string]$Stage,
        [Parameter(Mandatory)][string]$OutputRoot, [Parameter(Mandatory)][string[]]$Packages,
        [Parameter(Mandatory)][string]$ParserVersion, [string]$GameRoot,
        [string]$ContainerName,
        [string]$Retoc, [string]$CompanionPak
    )
    $context = Get-ModContext $ModRoot
    $output = Resolve-ModPath $OutputRoot
    $stageRoot = Resolve-ModPath $Stage
    Assert-ModChild $output $context.owner
    Assert-ModChild $stageRoot $context.owner
    Assert-ModUnlinked $stageRoot -Recurse
    if ($output -eq $stageRoot -or $output.StartsWith($stageRoot + '\', [StringComparison]::OrdinalIgnoreCase) -or
        $stageRoot.StartsWith($output + '\', [StringComparison]::OrdinalIgnoreCase)) { throw 'Container output and stage must be disjoint' }
    if (Test-Path -LiteralPath $output) { throw "Container output already exists: $output" }
    if (-not (Test-Path -LiteralPath $stageRoot -PathType Container)) { throw "Stage missing: $stageRoot" }
    Assert-ModPackageInventory $Packages
    foreach ($package in $Packages) {
        foreach ($extension in @('.uasset','.uexp')) {
            $path = Join-Path $stageRoot ('Voyage/Content/' + $package.Substring(6) + $extension)
            if (-not (Test-Path -LiteralPath $path -PathType Leaf)) { throw "Staged package companion missing: $path" }
        }
    }
    if (-not $ContainerName) { $ContainerName = $context.name }
    if ($ContainerName -cnotmatch '^[A-Za-z0-9_]+$') { throw "Invalid container name: $ContainerName" }
    if (-not $Retoc) { $Retoc = Join-Path $context.repo '.tools/bin/retoc.exe' }
    if ($CompanionPak) {
        $CompanionPak = (Resolve-Path -LiteralPath $CompanionPak).Path
        if ([IO.Path]::GetExtension($CompanionPak) -ine '.pak' -or
            -not (Test-Path -LiteralPath $CompanionPak -PathType Leaf)) { throw 'CompanionPak must be a PAK file' }
    }
    [IO.Directory]::CreateDirectory($output) | Out-Null
    $runner = Join-Path $context.repo 'tools/Invoke-VoyageBoundedTool.ps1'
    $bounded = Join-Path $output 'bounded-tool-runs'
    $metadata = $null
    if ($GameRoot) {
        $global = Join-Path (Resolve-ModPath $GameRoot) 'Voyage/Content/Paks/global.utoc'
        $globalData = [IO.Path]::ChangeExtension($global, '.ucas')
        foreach ($inputPath in @($global,$globalData)) {
            if (-not (Test-Path -LiteralPath $inputPath -PathType Leaf)) { throw "Current-game IoStore metadata missing: $inputPath" }
        }
        $scriptObjects = Join-Path $stageRoot 'scriptobjects.bin'
        if (Test-Path -LiteralPath $scriptObjects) { throw 'Stage already contains scriptobjects.bin; use a fresh stage' }
        $metadataRoot = Join-Path $output 'scriptobjects'
        [IO.Directory]::CreateDirectory($metadataRoot) | Out-Null
        $run = & $runner -Executable $Retoc -Arguments @('to-legacy','--version',$ParserVersion,'--no-assets','--no-shaders',$global,$metadataRoot) `
            -OutputRoot $bounded -TimeoutSeconds 60 -MemoryLimitMB 1024
        if ($run.status -cne 'passed') { throw "Script-object extraction failed: $($run.reportPath)" }
        $files = @(Get-ChildItem -LiteralPath $metadataRoot -File -Recurse)
        if ($files.Count -ne 1 -or $files[0].Name -cne 'scriptobjects.bin' -or $files[0].Length -eq 0) {
            throw "Metadata-only extraction did not produce one nonempty scriptobjects.bin: $metadataRoot"
        }
        Copy-Item -LiteralPath $files[0].FullName -Destination $scriptObjects
        $metadata = [pscustomobject]@{status='passed';inputs=@($global,$globalData);reportPath=$run.reportPath}
        Assert-ModChild $metadataRoot $output
        Remove-Item -LiteralPath $metadataRoot -Recurse -Force
    }
    $container = Join-Path $output ($ContainerName + '.utoc')
    $pack = & $runner -Executable $Retoc -Arguments @('to-zen','--version',$ParserVersion,$stageRoot,$container) `
        -OutputRoot $bounded -TimeoutSeconds 60 -MemoryLimitMB 1024
    if ($pack.status -cne 'passed') { throw "Container conversion failed: $($pack.reportPath)" }
    # A caller-owned PAK may supply additional registry/files beside the IoStore assets.
    if ($CompanionPak) { Copy-Item -LiteralPath $CompanionPak -Destination ([IO.Path]::ChangeExtension($container,'.pak')) -Force }
    $expected = Join-Path $output 'expected-packages.txt'
    @($Packages | ForEach-Object { '../../../Voyage/Content/' + $_.Substring(6) + '.uasset' }) |
        Set-Content -LiteralPath $expected -Encoding ASCII
    $validation = & (Join-Path $context.repo 'tools/Test-VoyageContainer.ps1') -Container $container `
        -ExpectedPackageList $expected -Retoc $Retoc -OutputRoot (Join-Path $output 'container-check')
    if ($validation.status -cne 'passed' -or -not $validation.packageSetMatches) {
        throw "Container verification failed: $($validation.reportPath)"
    }
    [pscustomobject]@{status='passed';container=$container;stage=$stageRoot;validation=$validation;scriptObjects=$metadata}
}

function Invoke-VoyageModCook {
    [CmdletBinding()]
    param(
        [Parameter(Mandatory)][string]$ModRoot, [Parameter(Mandatory)][string]$ProjectFile,
        [Parameter(Mandatory)][string]$EngineRoot, [Parameter(Mandatory)][string]$OutputRoot,
        [Parameter(Mandatory)][string[]]$Packages, [Parameter(Mandatory)][string]$ParserVersion,
        [string[]]$TaggedPackages=@(), [string[]]$AdditionalCookArguments=@(),
        [string]$GameRoot, [string]$ContainerName, [string]$Retoc
    )
    $cook = Invoke-VoyageModCookAssets -ModRoot $ModRoot -ProjectFile $ProjectFile -EngineRoot $EngineRoot `
        -OutputRoot $OutputRoot -Packages $Packages -TaggedPackages $TaggedPackages -AdditionalCookArguments $AdditionalCookArguments
    New-VoyageModContainer -ModRoot $ModRoot -Stage $cook.stage -OutputRoot (Join-Path $OutputRoot 'package') `
        -Packages $Packages -ParserVersion $ParserVersion -GameRoot $GameRoot -ContainerName $ContainerName -Retoc $Retoc
}

Export-ModuleMember -Function New-VoyageModRun,New-VoyageModProjectCopy,Remove-VoyageModScratch,Invoke-VoyageModEditorTest,Invoke-VoyageModCookAssets,New-VoyageModContainer,Invoke-VoyageModCook
