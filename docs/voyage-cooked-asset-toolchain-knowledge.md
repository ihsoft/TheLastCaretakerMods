# Voyage cooked-asset toolchain knowledge (observed, scoped contracts)

This document records observed game, engine, and toolchain behavior with its
version and validation boundaries. Treat unsupported extensions as hypotheses.
Repository workflow rules are in [toolchain rules](voyage-cooked-asset-toolchain-rule.md).

The [reviewed mapping registry](../mappings/Voyage/) keeps exact Steam build
identities and validation manifests. Use the public mapping resolver for the
installed game; this knowledge does not select a mapping by a historical ID.

## Self-contained materials in additive mods

On UE 5.8, an owned material can ship inside an additive DML container without
replacing the game's material or shader archives. Cook the material for the
game's shader format; the Windows DX12 path uses `PCD3D_SM6` in
`D3D12TargetedShaderFormats`. Native mesh usages, including Nanite when needed,
must be enabled on the material.

For a package that does not ship a separate shader library, set
`bShareMaterialShaderCode=False` and `bSharedMaterialNativeLibraries=False`
under `/Script/UnrealEd.ProjectPackagingSettings` in the project's
`Config/DefaultGame.ini`. This keeps shader code inline with the cooked
material. Include it explicitly in the project's release cook and inventory.
These are authoring project settings,
not configuration to copy into the installed game.

A per-mesh dynamic material instance can update a named color parameter
without changing a shared stock material. A base-color tint alone does not
provide an emissive control: inspect the material's exposed parameters or
author an emissive input explicitly. Keep the original slot material for
cleanup. Emission strength is a material value; perceived brightness and halo
also depend on game exposure and bloom.

Shader compilation and package checks establish structural correctness;
visual acceptance still belongs to the real game.

## Reflected weak-object arrays

An editor-only mirror must retain the real `FArrayProperty` inner type.
In particular, Voyage `VoyageLevelInstanceComponent.OwnedActors` is a weak
Actor array. Declaring it as a strong object array changes the element layout
and is not a compatible Blueprint exposure workaround.

For UE 5.8, an editor-only adapter can expose the exact weak property and
generate `EX_ArrayGetByRef`. Its property copy to the script VM resolves a weak
pointer to an object. An impure helper can materialize that result into a
transient strong snapshot for ordinary Blueprint iteration or membership
checks. Do not apply ordinary `Array_Get` or strong-object `Array_Contains`
directly to the weak array: their compiled temporaries and comparison operands
must match the native layout.

Validate the generated Blueprint helper through `ProcessEvent` with live,
null, and live weak elements, and verify the cooked by-reference opcode.
Neither compilation nor a plausible node graph proves correct dereferencing.
The adapter and reflected-visibility changes belong only to the editor mirror;
they must not require shipping a replacement Voyage module.

## DML actor identity and physical installation

DML v0.6 discovers new mods in `Voyage/Content/Paks/LogicMods`. Its
`WBP_DML.FindLogicMods` helper accepts both `<ModName>.pak` and
`<ModName>_P.pak`. The installation requirement is in
[toolchain rules](voyage-cooked-asset-toolchain-rule.md#dml-installation).

The actor's virtual class identity remains
`/Game/Mods/<ModName>/ModActor.ModActor_C`; the physical directory does not
rewrite it. Discovery and loading a previously registered name are different
contracts. Existing saved registration can mask a wrong installation layout.
A successful load of an already known mod, or the existence of
`DML add <ModName>`, therefore does not prove that a fresh mod is discoverable
outside `LogicMods`.

The `_P` suffix is not a universal DML requirement. Apply the repository's
stock-package override suffix rule when a container actually overrides stock
packages; additive-only mods do not need it for that reason.

This contract was observed with DML v0.6. Its `CM_DMG` implements the `DML add`
command and virtual class construction;
the widget's physical discovery helper is a separate path.

## Additive content-plugin registries

Local UE 5.8 source establishes a separate registry-loading path for enabled
content plugins. `Projects/Private/PluginManager.cpp` reads loose project
`Mods` descriptors; `AssetRegistry/Private/AssetRegistry.cpp`, in
`LoadPremadeAssetRegistry_Plugins`, reads each enabled content plugin's
`<plugin base>/AssetRegistry.bin` and appends its state in a non-editor build.
This does not require a native plugin module or replacing the main project
registry. The descriptor enables discovery; the binary registry supplies asset
metadata, and the cooked containers must still supply the actual packages.

A plugin-local registry can describe `/Game/...` packages. Its physical registry
location does not relocate those assets into the plugin's virtual mount point.
Voyage primary assets must still satisfy the independently configured Item and
Skill scan roots and native class/primary-ID contracts; enabling a content
plugin alone does not prove Asset Manager registration or recipe availability.

The observed layout uses a loose content-only
descriptor, a plugin-local registry in its PAK, and Item/Skill packages under
the stock `/Game/Data/Assets` roots in IoStore. Package structure alone does
not guarantee gameplay registration or initialization order.

Plugin descriptors extend the installed footprint beyond `Content/Paks`.
Use the [archive-layout contract](vortex-release-archive-knowledge.md) and
[release installation interface](../tools/release-installation.md) for packaging,
manifest validation and recovery. A successful additive registry does not
establish arbitrary registry overrides or duplicate-ID resolution.

With UE 5.8's `PakPlatformFile` active, the plugin registry lookup resolves
through mounted PAK files and does not fall back to a same-path loose
`AssetRegistry.bin`. Keep the descriptor loose and the registry at the
plugin-local path inside the mod PAK; a loose registry beside the descriptor is
not a supported equivalent layout.

## Stock AssetRegistry access and PAK encryption

The following offline findings were observed on Steam build `25191271`.
Exact callback locations, keys and extracted inputs belong to their source
build; these general format observations remain usable until evidence disagrees.

- Stock `Voyage/Content/Paks/pakchunk0-Windows.pak` has a version-12 PAK footer
  with `bEncryptedIndex = 1` and a zero encryption-key GUID. The zero GUID
  selects the default key; it does not mean the AES key is zero or absent.
  UnrealPak without that key fails with `Failed to find requested encryption
  key 00000000000000000000000000000000`.
- The repository's `VoyageAssetInspector` Game provider registers stock
  `.utoc` containers only. Successful package extraction through that route
  does not establish access to the PAK or its `AssetRegistry.bin`.
- The game contains a 32-byte key callback. UE 5.8's
  `Core/Public/Modules/ModuleManager.h`, `UE_REGISTER_ENCRYPTION_KEY`, explains
  its construction; `Core/Private/Misc/CoreDelegates.cpp` registers the
  callback, and `PakFile/Private/IPlatformFilePak.cpp` uses it for the default
  PAK key. No running-game injection is needed to read this callback.

Bind recovered callback data to the inspected executable and fail closed when
the callback or encrypted-index layout is not recognized. Validate a recovered
default key by decrypting the version-12 index and matching its footer SHA-1;
readable text is insufficient. UnrealPak key-chain JSON uses Base64 key bytes,
while legacy `-aes` interprets its value differently. Keep keys, crypto JSON,
extractions and raw reports in ignored output under the
[artifact lifecycle](artifact-lifecycle-rule.md): `Tmp/` by default, retained
artifacts only for an identified continuing use. None belongs in Git or a
distributed mod package.

## Package serialization is a separate compatibility contract

Matching export classes and decoded property values does not establish runtime
equivalence. Package flags, property-stream encoding, object/legacy versions
and resolved custom versions form a separate contract with the game reader.
Successful registration, UI display or parser reopen alone does not prove
that a gameplay consumer can use the reconstructed package.

Maintained Unreal DataAssets own the item graph and gameplay values. When a
consumer requires a bounded serialization conversion, use fresh cook as its
input and preserve the reviewed property stream, object versions and custom
versions. JSON is an intermediate representation, not a second authoring source.
The [item validation boundary](voyage-item-fabrication-and-pickup-knowledge.md#cooked-json-validation-boundary)
describes comparison with native saved-asset and fresh-cook readbacks.

A working unversioned package does not prove that all tagged Voyage packages
fail or authorize bulk conversion. Derive serialization requirements from the
affected consumer and reviewed source metadata, not a universal flag list or
fixed property count.

Flipping `IsUnversioned` / `PKG_UnversionedProperties` in a header does not
convert the property payload. Any conversion must write the complete intended
stream with the canonical writer and reviewed mappings, preserve gameplay
semantics, and reopen successfully before container and real-game checks.
Reopen establishes reader/writer consistency, not gameplay compatibility.

## Known boundaries

### Import layout and engine profiles

- Inspection of UE 5.8.2's `FObjectImport` serializer found no new
  object/custom-version discriminator for the reviewed import-layout difference:
  `PackageName` uses `VER_UE4_NON_OUTER_PACKAGE_IMPORT`; filtering changes
  placeholder/reset behavior. This source observation does not establish the
  entire engine's format history. Do not invent an Unreal object-version number
  to distinguish producers.
- Reviewed filtered legacy inputs can omit or retain `FObjectImport.PackageName`.
  The accepted tools read both layouts; the explicit `UE5_8` writer profile
  selects the expanded Voyage layout, while older profiles retain the upstream
  layout. A profile selection is not proof that an arbitrary runtime accepts it.
- Resolve import stride from checked table boundaries and counts. Reject null,
  out-of-range, invalid-name and cyclic references with bounded diagnostics.
  Unknown layouts are errors, not permission for speculative version retries.
  Unfiltered records retain `PackageName`.
- Source legacy layout and target IoStore profile are separate. Do not use the
  target profile as an unproven source fallback. A conversion failure or
  unbounded allocation belongs in the [toolchain diagnostic workflow](../tools/toolchain-validation.md),
  not another unchecked retry.

### JSON, opaque exports and schemas

- `SpecifiedEngineVersion` is runtime-only and absent from JSON. After JSON
  deserialization, the writer must apply the explicit serialization hint before
  writing while preserving stored object/custom versions. The GUI's serialization
  selection changes that hint, not every serializer branch. It is not general
  cross-engine schema migration.
- JSON omits `OverrideNameMapHashes`, so a no-op JSON roundtrip can recompute
  name hashes. Do not promise whole-package byte identity or normalize arbitrary
  header differences. Preserve the distinction between binary save and
  JSON-import save.
- `VerifyBinaryEquality()` and fully parsed exports are independent checks.
  `RawExport` can preserve unknown bytes while concealing a parse failure;
  successful parsing can still produce different saved bytes. Report unexpected
  raw fallback, exceptions and unapplied mappings separately.
- Native custom serializers are not ordinary reflected structs. For example,
  the inspected `InstancedPropertyBag` layout in `ABP_HologramPedestal`
  begins with a separate `bHasData` field. A current USMAP alone cannot make an
  unsupported serializer safe; reject unknown variants.
- `RigHierarchy` and `RigVM` require native serializers. Explicit opaque
  preservation can support an unchanged roundtrip, but must remain a reported
  limitation and cannot justify structural edits.
- An enum absent from mappings need not acquire invented members. For an
  unchanged roundtrip, an explicitly marked numeric fallback is acceptable only
  with known property width, preserved bytes and a visible notice.
- Dynamic Blueprint schemas need the full inherited dependency chain. If schema
  preload fails for missing dependencies, a later pass must be able to retry;
  a permanent processed marker would hide the incomplete schema.

### Inspection and reproducibility

- Single-package `to-zen` can resolve external UE5 imports by package IDs and
  public export hashes without copying every referenced package into its input
  folder. A missing-script-import warning followed by an invalid outer index
  can instead indicate a misread import table; dropping references is not a fix.
- Partial parsing of a recovered historical package does not establish a
  complete diff or compatible schema. Confirm any used value independently
  against its serialized payload. Current dependencies do not establish an old
  package's provenance.
- Separate the game executable version, editor version and parser profile.
  The [Update mapping](game-update-versions-knowledge.md) owns observed game
  identities; the public cook interface owns required command-line options.
- A `.NET` dialog with `0xE0434352` means an unhandled managed exception.
  Inspect the actual application error before blaming the runtime.
- Repeated retoc or Unreal cook/SavePackage runs can differ in physical order
  or package bytes. Whole-build byte equality is not a general reproducibility
  requirement. Compare relevant package identities, imports, exports,
  references, properties and Blueprint behavior; ignore a differing field only
  under an established normalization contract. A semantic comparison does not
  replace a real-game test of the affected behavior.
- Digest use is owned by the [hash rule](hash-usage-rule.md). Tool identities,
  dependency preparation and publishing interfaces belong in the
  [tool maintenance guide](../tools/toolchain-maintenance.md), not a historical
  dependency checkpoint list in this document.
