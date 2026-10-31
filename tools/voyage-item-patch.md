# VoyageItem project updates

`Update-VoyageItemAsset.ps1` refreshes a maintained editor DataAsset from the
current game and applies a bounded JSON recipe. It owns extraction, reviewed
mappings, native support preparation, replacement, fresh editor reopen, cook
and complete semantic comparison. Run it once; do not manually extract the item,
copy native headers into the project or build a project target first.

The supported profile is `voyage-item-default-generator-v1`: the reviewed
Steam 25982825 / UE 5.8 VoyageItem with its owned default crafting-generator
subobject. Recipes allow zero changes or one `DroppedActor` soft-class replacement,
guarded by the exact expected original reference. Other fields, shapes and game
builds fail rather than falling back to an older asset or partial serialization.
This is not a universal DataAsset importer.

The consumer supplies an existing project and a recipe. For example, this
recipe selects an owner-authored Gyro child; the replacement class must already
be available to the consumer:

```json
{
  "schemaVersion": 1,
  "profile": "voyage-item-default-generator-v1",
  "asset": "/Game/Data/Assets/Modules/DA_Item_Module_GyroCopter",
  "patches": [{
    "property": "DroppedActor",
    "expect": "/Game/Blueprints/Vehicles/BP_GyroCopter_Possessable.BP_GyroCopter_Possessable_C",
    "value": "/Game/Mods/Example/BP_GyroChild.BP_GyroChild_C"
  }]
}
```

```powershell
./tools/Update-VoyageItemAsset.ps1 `
  -Project './mods/Example/Voyage.uproject' `
  -Recipe './mods/Example/Build/Item.patch.json'
```

`-EngineRoot` and `-GameRoot` override the installations. Optional `-OutputRoot`
must be a fresh directory below repository `Tmp/`; mod-selected output belongs
under `Tmp/<Mod>/`. The project filename for this profile is `Voyage.uproject`.
The result reports `status`, the asset, allowed changes, cooked/source locations
and the comparison evidence. A failed command is not a usable update result.

## Native ownership

The project declares its provider through
[EditorSupport.json](unreal-editor-support.md#prepare-declared-project-dependencies).
It must load `Voyage` and `VoyageItemPatchEditor`. The tool owns
`VoyageItemPatch/Profile/` and `VoyageItemPatch/Source/VoyageItemPatchEditor/`.
The profile headers must compile inside the single `Voyage` module to preserve
their `/Script/Voyage` identities. Mod-specific native stubs remain with the mod;
compose these source trees into one provider instead of defining two competing
Voyage modules. The owner supplies the plugin descriptor, Voyage module build
rules and registration. In the project declaration, add the tool's `Profile/`
as an overlay at `Source/Voyage`, and its `Source/VoyageItemPatchEditor/` at
`Source/VoyageItemPatchEditor`. Overlay source paths are relative to the owner's
plugin descriptor. Declare Voyage as Runtime and VoyageItemPatchEditor as Editor;
the common tool builds editor binaries for both without shipping native code.

The updater uses ready DLLs installed by the common preparation tool. It does
not write project source modules, targets, ignore rules or mirrored headers.
Native preparation does not require BPT. BPT independently uses the same project
declaration so an ordinary Export or Preview can prepare a clean checkout too.

## Asset replacement and validation

Each update replaces the item and its owned crafting-generator stub at their
existing package identities. Existing reference dependencies are reused; missing
ones are created as explicitly marked editor placeholders. They do not become
runtime replacements merely because they exist in the project. The release
producer selects the actual shipped packages.

The tool compares both serialized exports, all represented values, object
references, localization and the unversioned/version contract after cook. Only
the recipe's replacement may differ from the current original. A changed
expected reference or unsupported shape fails before asset replacement.

Failures during asset mutation restore backed-up asset files and report rollback
status. Native-plugin preparation has its own transaction; a successful dependency
installation remains available if later asset validation fails. Preserve diagnostics
and backups on a reported incomplete rollback. The tool does not install a mod or
establish gameplay compatibility.

`Test-VoyageItemPatch.ps1` is the producer-facing validator for an original and
cooked/re-extracted package. Supply `-OriginalAsset`, `-CookedAsset`,
`-MappingsPath`, `-Recipe` and a fresh `-OutputRoot`. Its successful compact result
is the semantic check; ordinary callers need not inspect the complete dumps.

## Development checks

Preserve the profile's ordinal layouts and serialization sentinels. The native
`bIsRecyclable=true` and generator `AllowWithWater=true` defaults retain explicitly
false source values during unversioned cook; they are not gameplay defaults to
apply to the game. Localized text must retain its histories and flags through
the native reconstruction path; generic text import alone can lose `Immutable`.

Run `python -B tools/VoyageItemPatch/test_item_patch.py` for offline profile tests.
`VoyageItemPatch/tests/Test-ReadyProject.ps1` accepts a migrated `-Project`,
`-Recipe`, selected `-BlueprintAsset` paths and fresh `-OutputRoot`. It copies only
assets/configuration into a source-free consumer, runs cold public BPT Export,
updates twice and previews. It checks reuse without recompilation, absence of
duplicate/unrelated asset changes and absence of consumer C++/build sources.
Optional `-BlueprintSourceRoot` also compares the export to tracked BPT JSON.
These engine checks use the shared queue and do not install into the game.
Use PowerShell 7 (`pwsh`) for the updater and its integration checks.
