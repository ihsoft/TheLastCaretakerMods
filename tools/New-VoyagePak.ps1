# Package explicitly mapped extra files; does not build an IoStore container or install.
[CmdletBinding()]
param([Parameter(Mandatory)][string]$Manifest, [Parameter(Mandatory)][string]$OutputRoot,
    [string]$EngineRoot='P:/UnrealEngine/UE_5.8')
$ErrorActionPreference='Stop'
Set-StrictMode -Version Latest
$python=Join-Path $env:USERPROFILE '.cache/codex-runtimes/codex-primary-runtime/dependencies/python/python.exe'
$worker=Join-Path $PSScriptRoot 'AssetPackaging/pak_files.py'
$unrealPak=Join-Path $EngineRoot 'Engine/Binaries/Win64/UnrealPak.exe'
if (-not (Test-Path -LiteralPath $unrealPak -PathType Leaf)) { throw "UnrealPak is missing: $unrealPak" }
& $python -B $worker prepare --manifest $Manifest --output $OutputRoot | Out-Null
if ($LASTEXITCODE) { throw 'PAK request rejected; no archive was created.' }
$output=(Resolve-Path -LiteralPath $OutputRoot).Path
$request=Get-Content -LiteralPath (Join-Path $output 'request.json') -Raw | ConvertFrom-Json
function Invoke-Pak([string[]]$Arguments) {
    & (Join-Path $PSScriptRoot 'Invoke-VoyageBoundedTool.ps1') -Executable $unrealPak `
        -Arguments $Arguments -OutputRoot (Join-Path $output 'tool-runs') -MemoryLimitMB 2048 -TimeoutSeconds 120
}
$null=Invoke-Pak @($request.pakPath,('-Create='+(Join-Path $output 'response.txt')),'-Dest=../../../')
$list=Invoke-Pak @('-List',$request.pakPath)
& $python -B $worker check-list --output $output --log $list.stdoutPath | Out-Null
if ($LASTEXITCODE) { throw "PAK mount/file list rejected; inspect $output" }
$readback=Join-Path $output 'readback'
$null=New-Item -ItemType Directory -Path $readback
$null=Invoke-Pak @($request.pakPath,'-Extract',$readback)
& $python -B $worker verify --output $output --log $list.stdoutPath | Out-Null
if ($LASTEXITCODE) { throw "PAK readback rejected; inspect $output" }
Get-Content -LiteralPath (Join-Path $output 'result.json') -Raw | ConvertFrom-Json
