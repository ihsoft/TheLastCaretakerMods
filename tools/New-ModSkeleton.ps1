[CmdletBinding()]
param(
    [Parameter(Mandatory = $true)]
    [ValidateNotNullOrEmpty()]
    [string] $Name,

    [Parameter(Mandatory = $true)]
    [ValidateNotNullOrEmpty()]
    [string] $Summary
)

$ErrorActionPreference = 'Stop'
Set-StrictMode -Version Latest

if ($Name -cnotmatch '^[A-Za-z][A-Za-z0-9]{0,63}$' -or
    $Name -match '^(CON|PRN|AUX|NUL|COM[1-9]|LPT[1-9])$') {
    throw 'Name must start with a letter, contain only ASCII letters and digits, be at most 64 characters, and not be a Windows device name.'
}

$goal = $Summary.Trim()
if ($goal.Length -lt 3 -or $goal.Length -gt 240 -or $goal -match '[\r\n]') {
    throw 'Summary must be one line of 3 to 240 characters.'
}

$repositoryRoot = Split-Path -Parent $PSScriptRoot
$modsRoot = Join-Path $repositoryRoot 'mods'
$modRoot = Join-Path $modsRoot $Name
if (-not (Test-Path -LiteralPath $modsRoot -PathType Container)) {
    throw "Mods directory does not exist: $modsRoot"
}
if (Test-Path -LiteralPath $modRoot) {
    throw "Mod directory already exists; no files changed: $modRoot"
}

$index = @"
# $Name index

Start with the [repository index](../../AGENTS.md) for shared rules and
knowledge. For a new mod, follow its
[starting workflow](../../docs/new-mod-rule.md).

- **Read first for every mod task:** [Architecture](ARCHITECTURE.md) records the
  current design and known game contracts; the initial state is explicitly
  unimplemented.
- [Changelog](CHANGELOG.md): player-facing notes for the next Nexus release.
"@

$architecture = @"
# $Name architecture

## Current state

The mod is planned. No implementation, build, installation, or game validation
exists yet.

## Goal

$goal

## Established game contracts

None recorded yet. Identify the game object and its owning behavior before
selecting an implementation or pinning a Steam build ID.
"@

$readme = @"
# $Name

Planned mod: $goal

No candidate build command is available yet.
"@

$changelog = @"
# Changelog

## Next

<!-- Add concise player-facing changes for the next numbered release. -->
"@

$contents = [ordered]@{
    'AGENTS.md' = $index
    'ARCHITECTURE.md' = $architecture
    'README.md' = $readme
    'CHANGELOG.md' = $changelog
    'version.txt' = '0'
}

$encoding = New-Object System.Text.UTF8Encoding($false)
New-Item -ItemType Directory -Path $modRoot -ErrorAction Stop | Out-Null
try {
    foreach ($fileName in $contents.Keys) {
        $filePath = Join-Path $modRoot $fileName
        [System.IO.File]::WriteAllText($filePath, ([string]$contents[$fileName]).TrimEnd() + "`n", $encoding)
    }
} catch {
    throw "Skeleton creation stopped with incomplete files at $modRoot. Inspect that directory before retrying. $($_.Exception.Message)"
}

[pscustomobject]@{
    status = 'created'
    mod = $Name
    path = $modRoot
    files = @($contents.Keys)
} | ConvertTo-Json -Compress
