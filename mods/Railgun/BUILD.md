# Build and author Railgun

[Project/Voyage.uproject](Project/Voyage.uproject) owns the saved release assets.
The sole public [producer](Build-Railgun.ps1) cooks a temporary copy and packages its
explicit inventory. It does not regenerate graphs, models or primary DataAssets.
The shared [Unreal project rule](../../docs/unreal-project-rule.md) owns working-copy
isolation and accepting source edits; the [release workflow](../../docs/release-and-installation-rule.md)
owns installation, game acceptance and candidate selection.

## Files and working copies

| Path | Responsibility |
| --- | --- |
| `Project/Content/` | Authored assets and required editor reference stubs. |
| `Project/BlueprintText/` | Synchronized Blueprint JSON, mirroring paths below `/Game/`. |
| `Project/Config/` | Required renderer, collision, packaging and project settings. |
| `Project/EditorSupport/` | Native game API mirrors and saved-asset readers. |
| `Project/Tests/Native/` | Saved Blueprint settings-loader integration test. |
| `Assets/` | Model review/editing copy and role registry; distributed INI template. |
| `Settings/` | Runtime settings schema and bindings. |
| `Build/` | Cooked serialization contracts/codec, package checks and catalogue descriptor. |
| `slideshow/` | Publication images. |

[EditorSupport.json](Project/EditorSupport.json) declares the native
`RailgunSupport` plugin. Its `Voyage` module preserves `/Script/Voyage` identities;
`RailgunEditor` supplies inspection and test commandlets. Public BPT
preparation supplies this support and `BlueprintTextWorkspace`, using the shared
[native support provider](../../tools/unreal-editor-support.md) for build/reuse
and loading checks. Neither plugin's DLLs belong in the game payload.

[New-RailgunProjectCopy.ps1](New-RailgunProjectCopy.ps1) uses the
[shared pipeline](../../tools/mod-setup.md#shared-mod-pipeline) to copy current maintained
inputs, including uncommitted edits, into a fresh directory below `Tmp/Railgun/`.
It returns the copied `.uproject` path. Build and test entry points create their
own copies and remove them on success, retaining reports and candidate packages.
On failure, use the retained copy and diagnostics to investigate.

### Model authoring and preview

The saved meshes, materials and component hierarchy in `Project/Content/` own
the gun model. `Assets/Model/Railgun.glb` is its review/editing copy under the
[shared project ownership rule](../../docs/unreal-project-rule.md).
`Assets/Model/model-source.json` selects component roles and interaction/collision
boxes in that saved hierarchy; the producer consumes this registry, not the GLB.

For an explicitly requested replacement, use the shared
[complete GLB importer](../../tools/unreal-glb-import.md) in a fresh content
directory of the working project. Review its `BP_Model`, then integrate the
accepted hierarchy/resources with the maintained shell and
[model bindings](ARCHITECTURE.md#station-model-and-possession). The imported
graph-free actor does not replace the gameplay shell. Check yaw and pitch around
their authored pivots: the muzzle and descendants must follow, while the base
and unrelated branches remain fixed. Retain the role registry and intentional
collision, socket, indicator and cassette bindings when accepting the model.
Routine builds neither import nor require the GLB.

Use the shared [GLB preview](../../tools/glb-tools.md) to inspect the copy through
Blender. Preview output belongs below `Tmp/Railgun/`; inspect the updated project
model as well when accepting a replacement.

### Images and audio

Edit images and sound in the Unreal working project. For agent-assisted export
or an explicitly selected replacement file, use the shared
[resource tool](../../tools/unreal-resources.md). Accept reviewed saved assets
through the shared authoring workflow; ordinary builds cook those assets.
No external PNG/WAV copies are maintained as alternative inputs.

| Resource | Asset below `/Game/Mods/Railgun/` |
| --- | --- |
| Scope mask | `Station/T_RailgunOpticalMask` |
| Ready, charging and offline icons | `Station/T_RailgunStatusReady`, `Station/T_RailgunStatusCharging`, `Station/T_RailgunStatusOffline` |
| HUD ammunition indicator | `Station/T_RailgunAmmoIndicator` |
| Fabricator gun and ammunition icons | `Fabricator/T_RailgunIcon`, `Fabricator/T_RailgunAmmoIcon` |
| Research icon | `Research/T_RailgunSkill` |
| Shot audio | `Station/S_RailgunShotBlast` |

Preserve the image references in the HUD/primary assets and the sound's stock
`SC_SFX` binding unless changing those contracts intentionally. Export current
resources below `Tmp/Railgun/` for visual/audio review.

### Game-derived editor inputs

The `Voyage` mirror headers retain their source Steam build and reconstruction
method: reviewed UE 5.8 mappings, native metadata and the named stock consumers.
These are partial hand-reconstructed editor definitions, not a native game SDK.
The current accepted build is recorded in [game-build.json](game-build.json);
it does not replace the individual headers' source provenance.

Saved stock-reference packages in `Project/Content/` are reconstructed editor
placeholders for package/class identities, input actions and localization.
They are excluded from the explicit release inventory; the game supplies the
real objects. Railgun's own assets, including its three primary DataAssets,
remain authored project content. On a game update, use the candidate workflow
below to revalidate these dependencies; a failed reference or serialization
check requires correcting the affected input before release.

## Build and test

From the repository root, use the shared PowerShell launcher. The producer
reserves the Unreal queue for its entire operation:

```powershell
python -B tools/run_powershell.py mods/Railgun/Build-Railgun.ps1 -Install
```

Omit `-Install` to prepare only. `-OutputRoot` selects a fresh directory below
`Tmp/Railgun/`; `-ArtifactVersion` labels that run, not the release version.
The archive name remains `Railgun-V<N>.zip`.
`-AllowDirtySource` explicitly permits uncommitted inputs, including when
building with `-Install`. `-EngineRoot` and `-GameRoot` override local installation
paths. The default editor is the reviewed UE 5.8.2 installation at `P:/UnrealEngine/UE_5.8`.
`-CacheRoot` selects external engine cache storage.
The parser profile is `UE5_8`; the accepted source-build pin lives in
[game-build.json](game-build.json). On a pin mismatch, follow the shared
[candidate workflow](../../docs/release-and-installation-rule.md#game-update-and-candidate-builds)
for `-IgnorePinnedGameVersion`.

Cook and IoStore packaging use the [shared pipeline](../../tools/mod-setup.md#cook-and-container-stages).
Railgun supplies its owned inventory and selects tagged cook for all packages,
plus its shader policy. The common cooker transports the large inventory by file;
there is no mod-specific cook commandlet. The DataAsset conversions below run on
the returned stage before shared packaging. The producer supplies its verified
registry PAK as `CompanionPak`; the common container stage obtains game
`scriptobjects.bin` and verifies the final package set.

The producer reads the primary DataAssets and model, cooks the owned inventory,
converts the required cooked serialization and verifies the final container, registry, ZIP
and release manifest. The registry is derived from fresh owned DataAsset
readbacks. A build-only result has `status: built` and returns `archivePath`, `releaseManifestPath` and
`validationSummaryPath`.

The package check compares model hierarchy, transforms, references and collision
with this run's project inventory, and checks the final native item/module and
pickup identities. These comparisons protect the handoff between authored
inputs and the selected release payload; their expectations follow the current
inputs rather than a historical model snapshot. DataAsset conversion owns
fresh-cook/native comparison, serialization contracts and post-write readback.
The build does not pin gameplay function sets, bytecode structure or visual
tuning, and does not rerun settings-loader regressions. Those belong to changed
behavior acceptance rather than package production.

The release ZIP contains the `Railgun.pak/.utoc/.ucas` triplet and `Railgun.ini`
under `Voyage/Content/Paks/`, plus the loose content-only descriptor at
`Voyage/Mods/RailgunCatalogue/RailgunCatalogue.uplugin`. The PAK's registry
contract belongs to [primary assets and research](ARCHITECTURE.md#primary-assets-and-research).

When changing settings bindings, schema bounds/fallbacks or reload behavior,
run the saved Blueprint loader integration test in its own disposable project:

```powershell
python -B tools/run_powershell.py mods/Railgun/Test-Railgun.ps1
```

[Test-Railgun.ps1](Test-Railgun.ps1) accepts `-EngineRoot` and a fresh
`-OutputRoot` below `Tmp/Railgun/`. On success, its `reportPath` points to
`settings-validation.json` with status `passed`. It covers saved
Blueprint execution with schema-derived field bindings, inclusive bounds,
out-of-range fallbacks and reload/reset behavior. The schema determines the
setting set; the test does not pin its count or order. These checks catch
incorrect getter arguments and stale assigned values in the consumer. Grammar,
duplicate keys and case handling belong to the shared parser suite. Game
lifecycle remains a separate acceptance boundary.

## Install an existing candidate

[Install-Railgun.ps1](Install-Railgun.ps1) installs the supplied release manifest
without rebuilding. The [shared installation operation](../../tools/release-installation.md#install-a-candidate-with-settings)
replaces the complete INI from that candidate's ZIP together with the container
and plugin descriptor. `-ValidateOnly` returns the read-only plan.

```powershell
python -B tools/run_powershell.py mods/Railgun/Install-Railgun.ps1 -ReleaseManifest '<returned releaseManifestPath>'
```

Pass `-AllowDirtySource` when the candidate contains uncommitted source;
`-GameRoot` overrides the game installation. The game must be closed.
`Build-Railgun.ps1 -Install` delegates to this same installer after a successful
build and returns `status: installed`.

## Changing settings

After changing the schema or canonical INI, validate those inputs once:

```powershell
python -B tools/run_powershell.py tools/Validate-VoyageModSettings.ps1 `
  -SchemaPath mods/Railgun/Settings/Railgun.settings.json `
  -DefaultIniPath mods/Railgun/Assets/Railgun.ini
```

Ordinary builds copy the INI unchanged. Saved Blueprint loader checks cover
runtime behavior separately from this file/schema check.

## Primary DataAsset serialization

The producer reads the maintained ammo, gun and skill natively and cooks them
with tagged properties. Its [converter](Build/Convert-RailgunDataAsset.ps1)
processes only fresh cook through the reviewed UAssetGUI/UAssetAPI writer and
game mappings. It compares fresh cook with the native state, then the converted
package with the fresh cooked object graph,
including owned objects, references and FText. Uncooked UAssetGUI parsing is not
this authoring/validation boundary.

[data-assets-contract.json](Build/data-assets-contract.json) maps the source
assets to metadata-only `Build/*-serialization.json` contracts. These fix
package/version serialization, not gameplay values or complete payloads. Ammo
and gun use unversioned properties; the skill remains tagged. Do not extend
this conversion to unrelated native-child assets by analogy. Import encoding
and comparison constraints belong to the shared
[cooked JSON boundary](../../docs/voyage-item-fabrication-and-pickup-knowledge.md#cooked-json-validation-boundary).

The gun's authored `properties` map is explicitly empty and has no owned property
subobjects. The editor `VoyageItem` mirror uses a map sentinel to retain that
override; its [serializer](Project/EditorSupport/Source/Voyage/VoyageItem.cpp)
preserves complete map values without shipping the sentinel. General
format constraints belong to the shared [serialization contract](../../docs/voyage-cooked-asset-toolchain-knowledge.md#package-serialization-is-a-separate-compatibility-contract).

## Interactive and BPT authoring

Create a working copy before launching Unreal or exporting through BPT:

```powershell
$project = python -B tools/run_powershell.py mods/Railgun/New-RailgunProjectCopy.ps1 `
  -Destination 'R:/Codex/TheLastCaretakerMods/Tmp/Railgun/edit/Project'
python -B tools/run_powershell.py tools/BlueprintText/Invoke-BlueprintWorkspace.ps1 -Action Export `
  -ProjectFile $project -WorkspaceRoot './Tmp/BlueprintText/Railgun-edit' `
  -Asset '/Game/Mods/Railgun/Station/BP_RailgunOperator'
```

Follow the shared [BPT workflow](../../tools/blueprint-workspace.md) for
Export/edit/Apply/Preview. For native DataAssets, prepare the working copy and
use the [ordinary Details editor](#primary-dataasset-authoring).
Launch interactive Unreal through the [queue](../../tools/unreal-coordination.md).
Accept reviewed changes back into matching paths under `Project/` using the
[shared transfer rules](../../docs/unreal-project-rule.md#project-layout-and-temporary-data).

### Primary DataAsset authoring

Edit ammo, gun and skill in the working copy through Unreal's ordinary DataAsset
Details editor. Their native mirrors expose editable fields, including nested
structures and references; BPT represents their Blueprint consumers, not these
native DataAssets. Source paths are listed in
[data-assets-contract.json](Build/data-assets-contract.json).

Preserve native class/property identities, serialization defaults and
`VoyageItem::Serialize` behavior when extending the partial mirrors. Save,
review and transfer the intended asset change back, then use the public producer
for native-read/cook/converted-package comparison. The mirrors cover maintained
fields, not the complete game API or arbitrary owned-object creation.
