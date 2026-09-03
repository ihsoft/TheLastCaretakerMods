param(
    [string]$SourceRoot = (Join-Path $PSScriptRoot '..\.tools\CUE4Parse'),

    [string]$OutputRoot = (Join-Path $PSScriptRoot '..\.tools\bin\CUE4Parse')
)

$ErrorActionPreference = 'Stop'
Set-StrictMode -Version Latest

$expectedSourceCommit = 'ec6595e46448a817ac21ea9bde01caa48f80a420'
$microsoftBclMemoryVersion = '10.0.11'
$source = (Resolve-Path -LiteralPath $SourceRoot).Path
$sourceGitPath = $source.Replace('\', '/')
$project = (Resolve-Path -LiteralPath (Join-Path $source 'CUE4Parse\CUE4Parse.csproj')).Path
$destination = [IO.Path]::GetFullPath($OutputRoot)
$manifestFileName = 'publish-manifest.json'
$manifestPath = Join-Path $destination $manifestFileName

$actualCommit = (& git -c "safe.directory=$sourceGitPath" -C $source rev-parse HEAD).Trim()
if ($LASTEXITCODE -ne 0 -or $actualCommit -cne $expectedSourceCommit) {
    throw "CUE4Parse must be at the validated commit $expectedSourceCommit."
}
$sourceChanges = @(& git -c "safe.directory=$sourceGitPath" -C $source status --porcelain --untracked-files=no)
if ($LASTEXITCODE -ne 0 -or $sourceChanges.Count -ne 0) {
    throw 'Refusing to publish CUE4Parse from tracked source changes.'
}

function Test-PublishedBundle {
    if (-not (Test-Path -LiteralPath $manifestPath -PathType Leaf)) {
        return $false
    }
    try {
        $manifest = Get-Content -Raw -LiteralPath $manifestPath | ConvertFrom-Json
        if ([string]$manifest.kind -cne 'Voyage canonical CUE4Parse bundle' -or
            [string]$manifest.sourceCommit -cne $actualCommit -or
            [string]$manifest.microsoftBclMemoryVersion -cne $microsoftBclMemoryVersion -or
            [bool]$manifest.nativeBackend) {
            return $false
        }
        $manifestFiles = @($manifest.files)
        $expectedNames = @($manifestFiles | ForEach-Object { [string]$_.path } | Sort-Object)
        $actualNames = @(
            Get-ChildItem -LiteralPath $destination -File |
                Where-Object Name -cne $manifestFileName |
                ForEach-Object Name |
                Sort-Object
        )
        if (@(Compare-Object -ReferenceObject $expectedNames -DifferenceObject $actualNames).Count -ne 0) {
            return $false
        }
        foreach ($file in $manifestFiles) {
            $path = Join-Path $destination ([string]$file.path)
            if (-not (Test-Path -LiteralPath $path -PathType Leaf) -or
                (Get-Item -LiteralPath $path).Length -ne [long]$file.length -or
                (Get-FileHash -Algorithm SHA256 -LiteralPath $path).Hash -cne [string]$file.sha256) {
                return $false
            }
        }
        $bclMemoryPath = Join-Path $destination 'Microsoft.Bcl.Memory.dll'
        if (-not (Test-Path -LiteralPath $bclMemoryPath -PathType Leaf) -or
            -not (Get-Item -LiteralPath $bclMemoryPath).VersionInfo.ProductVersion.StartsWith(
                "$microsoftBclMemoryVersion+",
                [StringComparison]::Ordinal)) {
            return $false
        }
        return (Test-Path -LiteralPath (Join-Path $destination 'CUE4Parse.dll') -PathType Leaf)
    }
    catch {
        return $false
    }
}

if (Test-PublishedBundle) {
    $dll = Join-Path $destination 'CUE4Parse.dll'
    [pscustomobject]@{
        Path = (Resolve-Path -LiteralPath $dll).Path
        Length = (Get-Item -LiteralPath $dll).Length
        Sha256 = (Get-FileHash -Algorithm SHA256 -LiteralPath $dll).Hash
        SourceCommit = $actualCommit
        NativeBackend = $false
        Rebuilt = $false
    }
    return
}

$repositoryRoot = (Resolve-Path -LiteralPath (Join-Path $PSScriptRoot '..')).Path
$publishParent = Join-Path $repositoryRoot 'artifacts\tools'
[IO.Directory]::CreateDirectory($publishParent) | Out-Null
$publishWorkspace = Join-Path $publishParent ('cue4parse-publish-' + [Guid]::NewGuid().ToString('N'))
$publishDirectory = Join-Path $publishWorkspace 'publish'
$hostProject = Join-Path $publishWorkspace 'Cue4ParsePublishHost.csproj'
$destinationParent = [IO.Path]::GetDirectoryName($destination)
$candidate = $destination + '.new-' + [Guid]::NewGuid().ToString('N')
$backup = $destination + '.old-' + [Guid]::NewGuid().ToString('N')

try {
    [IO.Directory]::CreateDirectory($publishWorkspace) | Out-Null
    $escapedProject = [Security.SecurityElement]::Escape($project)
    $hostProjectText = @"
<Project Sdk="Microsoft.NET.Sdk">
  <PropertyGroup>
    <TargetFramework>net10.0</TargetFramework>
  </PropertyGroup>
  <ItemGroup>
    <ProjectReference Include="$escapedProject" />
    <PackageReference Include="Microsoft.Bcl.Memory" Version="$microsoftBclMemoryVersion" />
  </ItemGroup>
</Project>
"@
    [IO.File]::WriteAllText($hostProject, $hostProjectText)

    & dotnet publish $hostProject `
        -c Release `
        --no-self-contained `
        -p:CUE4PARSE_SKIP_NATIVE=true `
        -p:GeneratePackageOnBuild=false `
        -o $publishDirectory
    if ($LASTEXITCODE -ne 0) {
        throw "CUE4Parse publish failed with exit code $LASTEXITCODE."
    }

    $publishedDll = Join-Path $publishDirectory 'CUE4Parse.dll'
    if (-not (Test-Path -LiteralPath $publishedDll -PathType Leaf)) {
        throw "Publish did not produce CUE4Parse.dll: $publishedDll"
    }
    if (Test-Path -LiteralPath (Join-Path $publishDirectory 'CUE4Parse-Natives.dll') -PathType Leaf) {
        throw 'Managed-only CUE4Parse publish unexpectedly contains CUE4Parse-Natives.dll.'
    }
    $assemblyName = [Reflection.AssemblyName]::GetAssemblyName($publishedDll)
    if ($assemblyName.Name -cne 'CUE4Parse') {
        throw "Unexpected published assembly: $($assemblyName.FullName)"
    }

    [IO.Directory]::CreateDirectory($destinationParent) | Out-Null
    [IO.Directory]::CreateDirectory($candidate) | Out-Null
    foreach ($file in Get-ChildItem -LiteralPath $publishDirectory -File |
        Where-Object Name -NotLike 'Cue4ParsePublishHost.*') {
        Copy-Item -LiteralPath $file.FullName -Destination (Join-Path $candidate $file.Name)
    }
    $files = @(
        Get-ChildItem -LiteralPath $candidate -File |
            Sort-Object Name |
            ForEach-Object {
                [ordered]@{
                    path = $_.Name
                    length = $_.Length
                    sha256 = (Get-FileHash -Algorithm SHA256 -LiteralPath $_.FullName).Hash
                }
            }
    )
    $stableManifest = [ordered]@{
        kind = 'Voyage canonical CUE4Parse bundle'
        sourceCommit = $actualCommit
        targetFramework = 'net10.0'
        assemblyVersion = $assemblyName.Version.ToString()
        nativeBackend = $false
        microsoftBclMemoryVersion = $microsoftBclMemoryVersion
        files = $files
        publishedAtUtc = [DateTime]::UtcNow.ToString('o')
    }
    [IO.File]::WriteAllText(
        (Join-Path $candidate $manifestFileName),
        (($stableManifest | ConvertTo-Json -Depth 5) + [Environment]::NewLine))

    if (Test-Path -LiteralPath $destination -PathType Container) {
        Move-Item -LiteralPath $destination -Destination $backup
    }
    try {
        Move-Item -LiteralPath $candidate -Destination $destination
    }
    catch {
        if (Test-Path -LiteralPath $backup -PathType Container) {
            Move-Item -LiteralPath $backup -Destination $destination
        }
        throw
    }
    if (-not (Test-PublishedBundle)) {
        Remove-Item -LiteralPath $destination -Recurse -Force
        if (Test-Path -LiteralPath $backup -PathType Container) {
            Move-Item -LiteralPath $backup -Destination $destination
        }
        throw 'Canonical CUE4Parse bundle failed post-install verification.'
    }
    if (Test-Path -LiteralPath $backup -PathType Container) {
        Remove-Item -LiteralPath $backup -Recurse -Force
    }

    $installedDll = Join-Path $destination 'CUE4Parse.dll'
    [pscustomobject]@{
        Path = $installedDll
        Length = (Get-Item -LiteralPath $installedDll).Length
        Sha256 = (Get-FileHash -Algorithm SHA256 -LiteralPath $installedDll).Hash
        SourceCommit = $actualCommit
        NativeBackend = $false
        Rebuilt = $true
    }
}
finally {
    foreach ($temporaryDirectory in @($candidate, $publishWorkspace)) {
        if (Test-Path -LiteralPath $temporaryDirectory -PathType Container) {
            $resolved = (Resolve-Path -LiteralPath $temporaryDirectory).Path
            $allowedPublishParent = [IO.Path]::GetFullPath($publishParent) + [IO.Path]::DirectorySeparatorChar
            $allowedDestinationParent = [IO.Path]::GetFullPath($destinationParent) + [IO.Path]::DirectorySeparatorChar
            if (-not $resolved.StartsWith($allowedPublishParent, [StringComparison]::OrdinalIgnoreCase) -and
                -not $resolved.StartsWith($allowedDestinationParent, [StringComparison]::OrdinalIgnoreCase)) {
                throw "Refusing to clean CUE4Parse output outside approved roots: $resolved"
            }
            Remove-Item -LiteralPath $resolved -Recurse -Force
        }
    }
}
