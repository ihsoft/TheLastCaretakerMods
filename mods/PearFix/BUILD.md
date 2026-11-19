# PearFix build operations

Run Windows PowerShell scripts through the repository's
[shared launcher](../../tools/powershell-execution.md). Unreal operations use
the shared queue and must start outside the restricted sandbox.

## Candidate build and installation

```powershell
python -B tools/run_powershell.py mods/PearFix/Build-PearFix.ps1 -AllowDirtySource
```

Defaults: engine `P:/UnrealEngine/UE_5.8`, game
`P:/SteamLibrary/steamapps/common/Voyage`. Optional `-EngineRoot`, `-GameRoot`,
and fresh `-OutputRoot` select those locations. The producer checks the pinned
game build, prepares dependencies, cooks, verifies the exact package inventory,
and writes `PearFix-V1.zip`, `release-manifest.json`, and `build-result.json`
under a fresh `Tmp/PearFix/` run. It does not install or publish. Version `0`
remains unchanged until a numbered release is published.

Only `/Game/Mods/PearFix/ModActor` enters the container. Stock-path editor stubs
and the native editor support module are compilation dependencies, not payload.

For the chosen candidate, use the shared installer while the game is closed:

```powershell
python -B tools/run_powershell.py tools/Install-VoyageRelease.ps1 `
  -ReleaseManifest '<build result releaseManifestPath>' -ExpectedMod PearFix `
  -AllowDirtySource
```

DML must already be installed. The manifest selects the LogicMods destination.
Compilation and container verification do not establish in-game behavior.

## Blueprint authoring

```powershell
python -B tools/run_powershell.py mods/PearFix/New-PearFixProjectCopy.ps1 `
  -Destination Tmp/PearFix/authoring/Project
python -B tools/run_powershell.py tools/BlueprintText/Invoke-BlueprintWorkspace.ps1 `
  -Action Export -ProjectFile Tmp/PearFix/authoring/Project/Voyage.uproject `
  -WorkspaceRoot Tmp/BlueprintText/PearFix-edit -Asset /Game/Mods/PearFix/ModActor `
  -EngineRoot P:/UnrealEngine/UE_5.8
```

Follow the [shared BPT workflow](../../tools/blueprint-workspace.md) for editing,
Apply, Preview and selective source acceptance. JSON remains in the temporary
workspace; accept changed `Project/Content/<package>.uasset` files. Routine
builds use the saved assets.
Keep investigation evidence and game-test results in the ignored local backlog
until acceptance. No gameplay contract is established by editor-only stubs.

## Validation scope

For behavior changes, verify a broken full-size pear after save loading and after
arrival at a streamed location. Allow the five-second event batch, then check
collection and resumed tree growth independently. Confirm the short notice counts
repaired fruits, including multiple fruits on one tree, and healthy or repeated
checks remain silent. Preserve unrelated slot records and removal markers.

Editor fixtures can isolate registration guards, duplicate events, late
attachment, notification acceptance and teardown. They do not establish actual
streaming order or mesh readiness in the game. Documentation-only changes need
link and consistency checks, not a rebuilt gameplay package.
