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
| `Project/EditorSupport/` | Native game API mirrors, saved-asset readers and cook adapter. |
| `Project/Tests/Native/` | Saved Blueprint settings-loader integration test. |
| `Assets/` | External model, image and audio inputs; distributed INI template. |
| `Settings/` | Runtime settings schema and bindings. |
| `Build/` | Cooked serialization contracts/codec, package checks and catalogue descriptor. |
| `slideshow/` | Publication images. |

[EditorSupport.json](Project/EditorSupport.json) declares the native
`RailgunSupport` plugin. Its `Voyage` module preserves `/Script/Voyage` identities;
`RailgunEditor` supplies inspection, cook and test commandlets. Public BPT
preparation supplies this support and `BlueprintTextWorkspace`, using the shared
[native support provider](../../tools/unreal-editor-support.md) for build/reuse
and loading checks. Neither plugin's DLLs belong in the game payload.

Changing an external GLB, image or sound alone does not update its saved release
representation in `Project/Content/`; accept the corresponding focused asset
edit before building.

[New-RailgunProjectCopy.ps1](New-RailgunProjectCopy.ps1) uses the
[shared pipeline](../../tools/mod-setup.md#shared-mod-pipeline) to copy current maintained
inputs, including uncommitted edits, into a fresh directory below `Tmp/Railgun/`.
It returns the copied `.uproject` path. Build and test entry points create their
own copies and remove them on success, retaining reports and candidate packages.
On failure, use the retained copy and diagnostics to investigate.

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

From the repository root, reserve the shared Unreal queue for the whole producer:

```powershell
$hostExe = if ($PSEdition -eq 'Core') { 'pwsh.exe' } else { 'powershell.exe' }
& ./tools/Invoke-UnrealQueued.ps1 -Operation 'Build Railgun' `
  -FilePath (Join-Path $PSHOME $hostExe) `
  -ArgumentList @('-NoProfile','-ExecutionPolicy','Bypass','-File',
    'mods/Railgun/Build-Railgun.ps1','-Install')
```

Omit `-Install` to prepare only. `-OutputRoot` selects a fresh directory below
`Tmp/Railgun/`; `-ArtifactVersion` labels that run, not the release version.
The archive name remains `Railgun-V<N>.zip`.
`-AllowDirtySource` explicitly permits uncommitted inputs, including when
building with `-Install`. `-EngineRoot` and `-GameRoot` override local installation
paths. The default editor is the reviewed UE 5.8.2 installation at `K:/Epic Games/UE_5.8`.
`-CacheRoot` selects external engine cache storage.
The parser profile is `UE5_8`; the accepted source-build pin lives in
[game-build.json](game-build.json). On a pin mismatch, follow the shared
[candidate workflow](../../docs/release-and-installation-rule.md#game-update-and-candidate-builds)
for `-IgnorePinnedGameVersion`.

The producer validates the settings schema, executes the saved Blueprint loader,
reads the primary DataAssets and model, cooks the owned inventory, converts the
required cooked serialization and verifies the final container, registry, ZIP
and release manifest. The registry is derived from fresh owned DataAsset
readbacks. A build-only result has `status: built` and returns `archivePath`, `releaseManifestPath` and
`validationSummaryPath`; the summary includes the settings-loader result.

The release ZIP contains the `Railgun.pak/.utoc/.ucas` triplet and `Railgun.ini`
under `Voyage/Content/Paks/`, plus the loose content-only descriptor at
`Voyage/Mods/RailgunCatalogue/RailgunCatalogue.uplugin`. The PAK's registry
contract belongs to [primary assets and research](ARCHITECTURE.md#primary-assets-and-research).

Run just the settings-loader integration test in its own disposable project:

```powershell
$hostExe = if ($PSEdition -eq 'Core') { 'pwsh.exe' } else { 'powershell.exe' }
& ./tools/Invoke-UnrealQueued.ps1 -Operation 'Test Railgun settings' `
  -FilePath (Join-Path $PSHOME $hostExe) `
  -ArgumentList @('-NoProfile','-ExecutionPolicy','Bypass','-File',
    'mods/Railgun/Test-Railgun.ps1')
```

[Test-Railgun.ps1](Test-Railgun.ps1) accepts `-EngineRoot` and a fresh
`-OutputRoot` below `Tmp/Railgun/`. On success, its `reportPath` points to
`settings-validation.json` with status `passed`. It covers saved
Blueprint execution with missing/custom/invalid values, inclusive bounds,
explicit zero, key case and reload/reset behavior. Game lifecycle remains a
separate acceptance boundary.

## Install an existing candidate

[Install-Railgun.ps1](Install-Railgun.ps1) installs the supplied release manifest
without rebuilding. It reads the INI template from that candidate's ZIP,
preserves player values and applies the owned retired-key and energy-unit
migrations. Container and settings recovery remain coupled. A legacy
`Railgun.autoload` blocks installation until its recoverable migration is handled.

```powershell
./mods/Railgun/Install-Railgun.ps1 -ReleaseManifest '<returned releaseManifestPath>'
```

Pass `-AllowDirtySource` when the candidate contains uncommitted source;
`-GameRoot` overrides the game installation. The game must be closed.
`Build-Railgun.ps1 -Install` delegates to this same installer after a successful
build and returns `status: installed`.

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
$project = & ./mods/Railgun/New-RailgunProjectCopy.ps1 `
  -Destination 'R:/Codex/TheLastCaretakerMods/Tmp/Railgun/edit/Project'
& ./tools/BlueprintText/Invoke-BlueprintWorkspace.ps1 -Action Export `
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
