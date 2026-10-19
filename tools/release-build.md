# Release producers and container verification

## Release producers

Build/cook/package contracts remain with their owning mods. Use these routes
before searching for scripts or assembling Unreal/retoc commands manually.
Every listed producer except ScopeFix launches Unreal build, generation, or
cook. Start those producers outside the restricted sandbox from the first
invocation, including a retry with `-IgnorePinnedGameVersion`; their Unreal
stage may be hidden inside the PowerShell script. ScopeFix does not launch
Unreal. See the [execution rule](../docs/game-derived-data-rule.md)
for the failure boundary.

For a requested candidate, run the owning producer first. If it reports only a
pinned Steam build mismatch, retry with `-IgnorePinnedGameVersion`; the
producer still checks current inputs and records the actual build. Resolve a
reported mapping gap through the [mapping guide](mappings.md), then retry.
After a successful build, use the owning installation entry point and request
an in-game test. Mods with a coupled user INI must preserve that file through
their mod-specific installation path; a container-only installation is
insufficient for them.
Keep the same ZIP and manifest for release selection; no post-test rebuild is
needed for compatibility metadata. The [release rule](../docs/release-and-installation-rule.md)
owns candidate and publication status.

| Mod | Candidate producer | Mod index |
| --- | --- | --- |
| BoatHUDTotalResources | [Build-BoatHUDTotalResources.ps1](../mods/BoatHUDTotalResources/Build-BoatHUDTotalResources.ps1) | [Index](../mods/BoatHUDTotalResources/AGENTS.md) |
| ContainerLidLabel | [Build-ContainerLidLabel.ps1](../mods/ContainerLidLabel/Build-ContainerLidLabel.ps1) | [Index](../mods/ContainerLidLabel/AGENTS.md) |
| DonkLiftKeyboardControl | [Build-DonkLiftRelease.ps1](../mods/DonkLiftKeyboardControl/Build-DonkLiftRelease.ps1) | [Index](../mods/DonkLiftKeyboardControl/AGENTS.md) |
| ElectrifiedBoat | [Build-ElectrifiedBoat.ps1](../mods/ElectrifiedBoat/Build-ElectrifiedBoat.ps1) | [Index](../mods/ElectrifiedBoat/AGENTS.md) |
| MooringCable60m | [Build-MooringCable60m.ps1](../mods/MooringCable60m/Build-MooringCable60m.ps1) | [Index](../mods/MooringCable60m/AGENTS.md) |
| PalletContainerSnap | [Build-PalletContainerSnap.ps1](../mods/PalletContainerSnap/Build-PalletContainerSnap.ps1) | [Index](../mods/PalletContainerSnap/AGENTS.md) |
| Railgun | [Build-Railgun.ps1](../mods/Railgun/Build-Railgun.ps1) | [Index](../mods/Railgun/AGENTS.md) |
| ScopeFix | [Build-ScopeFix.ps1](../mods/ScopeFix/Build-ScopeFix.ps1) | [Index](../mods/ScopeFix/AGENTS.md) |
| StableGyro | [Build-StableGyro.ps1](../mods/StableGyro/Build-StableGyro.ps1) | [Index](../mods/StableGyro/AGENTS.md) |
| VoyageAutoLoader | [Build-VoyageAutoLoader.ps1](../mods/VoyageAutoLoader/Build-VoyageAutoLoader.ps1) | [Index](../mods/VoyageAutoLoader/AGENTS.md) |

Read only the selected mod's index and relevant contracts. The producer owns
its fingerprint and provenance gates; a development probe or another mod's
builder is not a substitute.

Some preparation phases require a closed game even without `-Install`. The
selected producer checks its own preconditions and reports a running game;
there is no separate game-status or version preflight on the normal path. Do
not manually bypass its source or manifest gates.

For an already-built release, skip build/cook and use its release manifest with
the [installation guide](release-installation.md). The common installer accepts
schema-2 release manifests and schema 3 with one exact content-plugin
descriptor. A build manifest is not an installer input. Keep exact commands and
stage-specific details with the owning producer or its mod documentation.
The shared installer covers the container payload for BoatHUDTotalResources,
DonkLiftKeyboardControl, ElectrifiedBoat, MooringCable60m,
PalletContainerSnap, ScopeFix, and VoyageAutoLoader. Railgun and StableGyro
use their producer's `-Install` mode to preserve user INI settings.
ContainerLidLabel requires the shared container installer plus its
[INI migration](../mods/ContainerLidLabel/CONFIGURATION.md).

## Release container verification

```powershell
$check = & .\tools\Test-VoyageContainer.ps1 -Container '<exact .utoc>'
$check = & .\tools\Test-VoyageContainer.ps1 -Container '<exact .utoc>' `
    -ExpectedPackageList '<release-owned expected-paths.txt>'
```

The tool verifies one exact container and returns `status`, `packageCount`,
`packageSetMatches`, `reportPath`, and `packageListPath`. Supply an independently
maintained expected package list when exact scope matters; comparing the
container against its own returned list is circular. Without an expectation,
`packageSetMatches` is null. Read the detailed report only for a failed or
unexpected result.

The check does not establish mount precedence, game compatibility, or gameplay
behavior. For an already-built release, continue with the
[installation guide](release-installation.md). Tool-development regressions
belong in [toolchain validation](toolchain-validation.md).
