param(
    [string]$SourceRoot = (Join-Path $PSScriptRoot '..\.tools\UAssetAPI'),

    [string]$OutputRoot = (Join-Path $PSScriptRoot '..\.tools\bin\UAssetAPI')
)

$ErrorActionPreference = 'Stop'
Set-StrictMode -Version Latest

$expectedSourceCommit = '6b5ead37f213adc79d814689040a519be4e04a74'
$source = (Resolve-Path -LiteralPath $SourceRoot).Path
$sourceGitPath = $source.Replace('\', '/')
$project = (Resolve-Path -LiteralPath (Join-Path $source 'UAssetAPI\UAssetAPI.csproj')).Path
$destination = [IO.Path]::GetFullPath($OutputRoot)
$manifestFileName = 'publish-manifest.json'
$manifestPath = Join-Path $destination $manifestFileName

$actualCommit = (& git -c "safe.directory=$sourceGitPath" -C $source rev-parse HEAD).Trim()
if ($LASTEXITCODE -ne 0 -or $actualCommit -cne $expectedSourceCommit) {
    throw "UAssetAPI must be at the validated commit $expectedSourceCommit."
}
$sourceChanges = @(& git -c "safe.directory=$sourceGitPath" -C $source status --porcelain --untracked-files=no)
if ($LASTEXITCODE -ne 0 -or $sourceChanges.Count -ne 0) {
    throw 'Refusing to publish UAssetAPI from tracked source changes.'
}

function Test-PublishedBundle {
    if (-not (Test-Path -LiteralPath $manifestPath -PathType Leaf)) {
        return $false
    }
    try {
        $manifest = Get-Content -Raw -LiteralPath $manifestPath | ConvertFrom-Json
        if ([string]$manifest.kind -cne 'Voyage canonical UAssetAPI bundle' -or
            [string]$manifest.sourceCommit -cne $actualCommit) {
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
        return (Test-Path -LiteralPath (Join-Path $destination 'UAssetAPI.dll') -PathType Leaf)
    }
    catch {
        return $false
    }
}

if (Test-PublishedBundle) {
    $dll = Join-Path $destination 'UAssetAPI.dll'
    [pscustomobject]@{
        Path = (Resolve-Path -LiteralPath $dll).Path
        Length = (Get-Item -LiteralPath $dll).Length
        Sha256 = (Get-FileHash -Algorithm SHA256 -LiteralPath $dll).Hash
        SourceCommit = $actualCommit
        Rebuilt = $false
    }
    return
}

$repositoryRoot = (Resolve-Path -LiteralPath (Join-Path $PSScriptRoot '..')).Path
$publishParent = Join-Path $repositoryRoot 'artifacts\tools'
[IO.Directory]::CreateDirectory($publishParent) | Out-Null
$publishDirectory = Join-Path $publishParent ('uassetapi-publish-' + [Guid]::NewGuid().ToString('N'))
$destinationParent = [IO.Path]::GetDirectoryName($destination)
$candidate = $destination + '.new-' + [Guid]::NewGuid().ToString('N')
$backup = $destination + '.old-' + [Guid]::NewGuid().ToString('N')

try {
    & dotnet publish $project `
        -c Release `
        --no-self-contained `
        -p:GeneratePackageOnBuild=false `
        -o $publishDirectory
    if ($LASTEXITCODE -ne 0) {
        throw "UAssetAPI publish failed with exit code $LASTEXITCODE."
    }

    $publishedDll = Join-Path $publishDirectory 'UAssetAPI.dll'
    if (-not (Test-Path -LiteralPath $publishedDll -PathType Leaf)) {
        throw "Publish did not produce UAssetAPI.dll: $publishedDll"
    }
    $assemblyName = [Reflection.AssemblyName]::GetAssemblyName($publishedDll)
    if ($assemblyName.Name -cne 'UAssetAPI') {
        throw "Unexpected published assembly: $($assemblyName.FullName)"
    }

    [IO.Directory]::CreateDirectory($destinationParent) | Out-Null
    [IO.Directory]::CreateDirectory($candidate) | Out-Null
    foreach ($file in Get-ChildItem -LiteralPath $publishDirectory -File) {
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
        kind = 'Voyage canonical UAssetAPI bundle'
        sourceCommit = $actualCommit
        targetFramework = 'net10.0'
        assemblyVersion = $assemblyName.Version.ToString()
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
        throw 'Canonical UAssetAPI bundle failed post-install verification.'
    }
    if (Test-Path -LiteralPath $backup -PathType Container) {
        Remove-Item -LiteralPath $backup -Recurse -Force
    }

    $installedDll = Join-Path $destination 'UAssetAPI.dll'
    [pscustomobject]@{
        Path = $installedDll
        Length = (Get-Item -LiteralPath $installedDll).Length
        Sha256 = (Get-FileHash -Algorithm SHA256 -LiteralPath $installedDll).Hash
        SourceCommit = $actualCommit
        Rebuilt = $true
    }
}
finally {
    foreach ($temporaryDirectory in @($candidate, $publishDirectory)) {
        if (Test-Path -LiteralPath $temporaryDirectory -PathType Container) {
            $resolved = (Resolve-Path -LiteralPath $temporaryDirectory).Path
            $allowedPublishParent = [IO.Path]::GetFullPath($publishParent) + [IO.Path]::DirectorySeparatorChar
            $allowedDestinationParent = [IO.Path]::GetFullPath($destinationParent) + [IO.Path]::DirectorySeparatorChar
            if (-not $resolved.StartsWith($allowedPublishParent, [StringComparison]::OrdinalIgnoreCase) -and
                -not $resolved.StartsWith($allowedDestinationParent, [StringComparison]::OrdinalIgnoreCase)) {
                throw "Refusing to clean UAssetAPI output outside approved roots: $resolved"
            }
            Remove-Item -LiteralPath $resolved -Recurse -Force
        }
    }
}
