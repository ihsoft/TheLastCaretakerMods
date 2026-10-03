[CmdletBinding()]
param([Parameter(Mandatory=$true)][string]$EngineRoot)

$ErrorActionPreference = 'Stop'
Set-StrictMode -Version Latest
$root = [IO.Path]::GetFullPath($EngineRoot)
$versionPath = Join-Path $root 'Build\Build.version'
$modulesPath = Join-Path $root 'Binaries\Win64\UnrealEditor.modules'
$editorPath = Join-Path $root 'Binaries\Win64\UnrealEditor-Cmd.exe'
foreach ($path in @($versionPath,$modulesPath,$editorPath)) {
    if (-not (Test-Path -LiteralPath $path -PathType Leaf)) {
        throw "Unreal Engine identity file is missing: $path"
    }
}
$version = Get-Content -LiteralPath $versionPath -Raw | ConvertFrom-Json
$modules = Get-Content -LiteralPath $modulesPath -Raw | ConvertFrom-Json
if ([int]$version.MajorVersion -ne 5 -or [int]$version.MinorVersion -ne 8 -or
    [string]$version.BranchName -cne '++UE5+Release-5.8' -or
    [string]::IsNullOrWhiteSpace([string]$modules.BuildId)) {
    throw 'AssetRegistry writer supports only the reviewed UE 5.8 editor profile.'
}
[pscustomobject]@{
    engineRoot = $root
    majorVersion = [int]$version.MajorVersion
    minorVersion = [int]$version.MinorVersion
    patchVersion = [int]$version.PatchVersion
    changelist = [int64]$version.Changelist
    compatibleChangelist = [int64]$version.CompatibleChangelist
    branchName = [string]$version.BranchName
    buildId = [string]$modules.BuildId
    buildVersionSha256 = (Get-FileHash -LiteralPath $versionPath -Algorithm SHA256).Hash
    editorModulesSha256 = (Get-FileHash -LiteralPath $modulesPath -Algorithm SHA256).Hash
    editorPath = $editorPath
    editorSha256 = (Get-FileHash -LiteralPath $editorPath -Algorithm SHA256).Hash
}
