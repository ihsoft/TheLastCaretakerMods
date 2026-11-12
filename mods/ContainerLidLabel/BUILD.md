# Build and validate ContainerLidLabel

The main implementation is [Project/Voyage.uproject](Project/Voyage.uproject).
[Build-ContainerLidLabel.ps1](Build-ContainerLidLabel.ps1) cooks a temporary copy
of its saved assets; it does not regenerate graphs, modify stock packages or
install the result.
Windows PowerShell 5.1 and PowerShell 7 are supported entry points.

## Source project and working copies

`Project/` holds maintained inputs: Content, optional Config, BlueprintText,
EditorSupport, Tests, the project descriptor and dependency declaration.
Its only ignored directory is Intermediate. Content and Config have no ignored
subtrees or files: necessary assets and settings belong in Git. Currently the
mod needs no authored Config files. Do not add editor-generated settings there
merely because a plugin wrote them during a run.

Builds and tests call
[New-ContainerLidLabelProjectCopy.ps1](New-ContainerLidLabelProjectCopy.ps1)
to copy those inputs to a fresh `Tmp/ContainerLidLabel/<run>/Project`.
The helper includes uncommitted source changes, excludes Intermediate and does
not generate assets or copy prepared plugins/caches. All Unreal preparation,
test fixtures, temporary INI files, cook output and incidental Config writes
stay in that copy. Successful builds/tests delete their project copy; reports,
logs and candidate packages remain. Failed runs retain the copy and diagnostics.
Each fresh copy prepares its own native dependencies, so it incurs a cold build.

For interactive Unreal/BPT authoring, create an explicit working copy first:

```powershell
./mods/ContainerLidLabel/New-ContainerLidLabelProjectCopy.ps1 `
  -Destination 'R:/Codex/TheLastCaretakerMods/Tmp/ContainerLidLabel/edit-label/Project'
```

Point the shared BPT workflow at the returned .uproject, or prepare that copy
before opening it through the Unreal queue. Do not launch the maintained project
directly: stock Unreal recreates its standard generated directories there.
Opening the Project folder in a file browser needs no preparation.

An authoring copy is not a second authority. Before accepting edits, check for
concurrent changes in the maintained source, review the exact asset/BPT diff,
and transfer only the intentionally changed saved assets and synchronized BPT
JSON back into Project. Transfer deliberately authored Config changes explicitly.
Never copy the whole working project back. Test/build copies never write back.

## Build

Run the whole producer through the shared Unreal queue from the repository root:

```powershell
$hostExe = if ($PSEdition -eq 'Core') { 'pwsh.exe' } else { 'powershell.exe' }
& ./tools/Invoke-UnrealQueued.ps1 -Operation 'Build ContainerLidLabel' `
  -FilePath (Join-Path $PSHOME $hostExe) `
  -ArgumentList @('-NoProfile','-ExecutionPolicy','Bypass','-File',
    'mods/ContainerLidLabel/Build-ContainerLidLabel.ps1',
    '-OutputRoot','R:/Codex/TheLastCaretakerMods/Tmp/ContainerLidLabel/candidate',
    '-AllowDirtySource')
```

OutputRoot must be fresh and below `Tmp/ContainerLidLabel`; omitting it selects
a build directory automatically. EngineRoot and GameRoot can override local
installations. AllowDirtySource records uncommitted source; omit it for a clean
release source. Version comes from version.txt under the shared release policy.

The builder prepares the copied project's editor dependencies through BPT Prepare
before cooking.
This includes both the Voyage API stand-ins and the shared BlueprintTextWorkspace
plugin required by authored optional-argument nodes. They are authoring/cooking
dependencies, not game payload. The exact four-package list excludes all editor
stand-ins and native DLLs. Shared tools validate the container, ZIP layout,
settings and release manifest. The ZIP deploys the triplet and INI directly to
`Voyage/Content/Paks/LogicMods`. DML must already be installed in the game.

For an installed-game build mismatch, follow the shared release rule and retry
with `-IgnorePinnedGameVersion`; changed game contracts still require validation.
Do not regenerate project assets merely to build a package.

## Editor checks

Run the owning test entry point through the queue. It creates and prepares its
own copy; do not execute ValidateEditor.py against the maintained project:

```powershell
$hostExe = if ($PSEdition -eq 'Core') { 'pwsh.exe' } else { 'powershell.exe' }
& ./tools/Invoke-UnrealQueued.ps1 -Operation 'Test ContainerLidLabel' `
  -FilePath (Join-Path $PSHOME $hostExe) `
  -ArgumentList @('-NoProfile','-ExecutionPolicy','Bypass','-File',
    'mods/ContainerLidLabel/Test-ContainerLidLabel.ps1')
```

The runner returns the report path under `Tmp/ContainerLidLabel/test-editor-*/`
and requires `status: passed`; Unreal's process exit alone does not establish
Python assertions passed. EngineRoot is configurable. An explicit OutputRoot
must be a fresh direct child of Tmp/ContainerLidLabel.
The suite exercises existing/event-created supported containers, excluded heirs,
capacity snapshots/fallbacks, names/icons, asynchronous changes, attachment and
cleanup. It uses synthetic editor actors and does not prove visual appearance,
save serialization, event timing or relative performance. BPT edits additionally
use strict Apply and Preview through the shared workflow.

Test source is maintained in Project/Tests and the native editor stand-ins;
it belongs in the repository. The generated actors/materials/child Blueprints,
temporary INI and execution evidence are per-run output, not test source.
Any future deliberately authored reusable test assets also belong in source,
with an explicit test-only package boundary.

## Install and upgrade

```powershell
& ./mods/ContainerLidLabel/Install-ContainerLidLabel.ps1 `
  -ReleaseManifest 'R:/Codex/TheLastCaretakerMods/Tmp/ContainerLidLabel/candidate/release-manifest.json'
```

The owning installer validates the manifest, rejects a running game, preserves
player settings, installs and reads back the payload. It refuses coexistence
with legacy `ContainerLidLabel_P` overrides. Uninstall that old package through
the mod manager before upgrading; preserve its INI for the migration described
in [configuration](CONFIGURATION.md). Do not use the generic container installer
alone for a settings-preserving update.

Keep the exact game-accepted ZIP and manifest selected for publication; a new
verification build does not replace that selection automatically. Publication,
version advancement and commits follow the shared rules and require authorization.
