param(
    [string]$SourceRoot,

    [string]$OutputPath
)

$ErrorActionPreference = 'Stop'
if ([string]::IsNullOrWhiteSpace($SourceRoot)) { $SourceRoot = Join-Path $PSScriptRoot '..\.tools\UAssetGUI' }
if ([string]::IsNullOrWhiteSpace($OutputPath)) { $OutputPath = Join-Path $PSScriptRoot '..\.tools\bin\UAssetGUI.exe' }

$expectedGuiCommit = 'df18b5fd0d263d78fdb0cd5f49de1ee5cf6a9520'
$expectedApiCommit = '21c982fa8f04e12d5d216fdf330a2f206e81156f'
$source = (Resolve-Path -LiteralPath $SourceRoot).Path
$apiSource = (Resolve-Path -LiteralPath (Join-Path $source 'UAssetAPI')).Path
$project = (Resolve-Path -LiteralPath (
    Join-Path $source 'UAssetGUI\UAssetGUI.csproj')).Path
$destination = [IO.Path]::GetFullPath($OutputPath)
$destinationDirectory = [IO.Path]::GetDirectoryName($destination)

$actualGuiCommit = (& git -C $source rev-parse HEAD).Trim()
if ($LASTEXITCODE -ne 0 -or $actualGuiCommit -cne $expectedGuiCommit) {
    throw "UAssetGUI must be at the validated commit $expectedGuiCommit."
}
$actualApiCommit = (& git -C $apiSource rev-parse HEAD).Trim()
if ($LASTEXITCODE -ne 0 -or $actualApiCommit -cne $expectedApiCommit) {
    throw "UAssetAPI must be at the validated commit $expectedApiCommit."
}
$guiChanges = @(& git -C $source status --porcelain --untracked-files=no)
$apiChanges = @(& git -C $apiSource status --porcelain --untracked-files=no)
if ($LASTEXITCODE -ne 0 -or $guiChanges.Count -ne 0 -or $apiChanges.Count -ne 0) {
    throw 'Refusing to replace the stable binary from tracked source changes.'
}

$repositoryRoot = (Resolve-Path -LiteralPath (Join-Path $PSScriptRoot '..')).Path
$publishParent = Join-Path $repositoryRoot 'artifacts\tools'
[IO.Directory]::CreateDirectory($publishParent) | Out-Null
$publishDirectory = Join-Path $publishParent (
    'uassetgui-publish-' + [Guid]::NewGuid().ToString('N'))
$candidate = $destination + '.new'

try {
    & dotnet publish $project `
        -c Release `
        -r win-x64 `
        --self-contained false `
        -p:PublishSingleFile=true `
        -o $publishDirectory
    if ($LASTEXITCODE -ne 0) {
        throw "UAssetGUI publish failed with exit code $LASTEXITCODE."
    }

    $publishedExecutable = Join-Path $publishDirectory 'UAssetGUI.exe'
    if (-not (Test-Path -LiteralPath $publishedExecutable -PathType Leaf)) {
        throw "Publish did not produce UAssetGUI.exe: $publishedExecutable"
    }

    [IO.Directory]::CreateDirectory($destinationDirectory) | Out-Null
    Copy-Item -LiteralPath $publishedExecutable -Destination $candidate -Force
    $publishedHash = (Get-FileHash -Algorithm SHA256 -LiteralPath (
        $publishedExecutable)).Hash
    $candidateHash = (Get-FileHash -Algorithm SHA256 -LiteralPath $candidate).Hash
    if ($candidateHash -cne $publishedHash) {
        throw 'Stable-binary candidate failed hash verification.'
    }

    Move-Item -LiteralPath $candidate -Destination $destination -Force
    $installedHash = (Get-FileHash -Algorithm SHA256 -LiteralPath $destination).Hash
    if ($installedHash -cne $publishedHash) {
        throw 'Stable UAssetGUI binary failed post-install hash verification.'
    }

    $manifestPath = [IO.Path]::ChangeExtension($destination, '.manifest.json')
    $manifest = [ordered]@{
        kind = 'Voyage canonical UAssetGUI binary'
        guiCommit = $actualGuiCommit
        apiCommit = $actualApiCommit
        retocResourceSha256 = (Get-FileHash -LiteralPath (Join-Path $source 'UAssetGUI\retoc.exe.gz') -Algorithm SHA256).Hash
        executableSha256 = $installedHash
        executableLength = (Get-Item -LiteralPath $destination).Length
        publishedAtUtc = [DateTime]::UtcNow.ToString('o')
    }
    [IO.File]::WriteAllText($manifestPath, ($manifest | ConvertTo-Json))

    [pscustomobject]@{
        Path = $destination
        Length = (Get-Item -LiteralPath $destination).Length
        Sha256 = $installedHash
        UAssetGuiCommit = $actualGuiCommit
        UAssetApiCommit = $actualApiCommit
        SelfContained = $false
        RuntimeIdentifier = 'win-x64'
        ManifestPath = $manifestPath
    }
}
finally {
    if (Test-Path -LiteralPath $candidate -PathType Leaf) {
        Remove-Item -LiteralPath $candidate -Force
    }
    if (Test-Path -LiteralPath $publishDirectory -PathType Container) {
        $resolvedPublish = (Resolve-Path -LiteralPath $publishDirectory).Path
        $allowedParent = [IO.Path]::GetFullPath($publishParent) +
            [IO.Path]::DirectorySeparatorChar
        if (-not $resolvedPublish.StartsWith(
                $allowedParent,
                [StringComparison]::OrdinalIgnoreCase)) {
            throw "Refusing to clean publish output outside artifacts: $resolvedPublish"
        }
        Remove-Item -LiteralPath $resolvedPublish -Recurse -Force
    }
}
