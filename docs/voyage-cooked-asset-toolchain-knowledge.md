# Voyage cooked-asset toolchain knowledge (observed, scoped contracts)

This document records observed game, engine, and toolchain behavior with its
version and validation boundaries. Treat unsupported extensions as hypotheses.
Repository workflow rules are in [toolchain rules](voyage-cooked-asset-toolchain-rule.md).

The [reviewed mapping registry](../mappings/Voyage/) keeps exact Steam build
identities and validation manifests. Use the public mapping resolver for the
installed game; this knowledge does not select a mapping by a historical ID.

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

This contract is bound to the DML v0.6 payload whose `dml_P.utoc` SHA-256 is
`8DDE713A29CA21227E91757DFE64A9928135C3AD1C549E207EAED8BF412B7A2F`.
Its `CM_DMG` implements the `DML add` command and virtual class construction;
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

Railgun's normal source producer derives an exact three-record plugin registry
from its owned package readbacks through the shared Voyage registry profile and
class-agnostic native writer, packages it at
`Voyage/Mods/RailgunCatalogue/AssetRegistry.bin`, and releases a loose
content-only descriptor through the schema-3 manifest contract. This
three-record plugin registry supports Railgun without a global registry
override; it does not establish behavior for arbitrary registry overrides or
duplicate IDs. Plugin descriptors extend the installed footprint
beyond `Content/Paks`, so schema-3 installation and restoration retain the
common backup, hash, path, reparse-point and closed-game gates.

With UE 5.8's `PakPlatformFile` active, the plugin registry lookup resolves
through mounted PAK files and does not fall back to a same-path loose
`AssetRegistry.bin`. Keep the descriptor loose and the registry at the
plugin-local path inside the mod PAK; a loose registry beside the descriptor is
not a supported equivalent layout.

## Stock AssetRegistry access and PAK encryption

The following offline findings apply to Steam build `25191271`.
Check the installed Steam build ID before using these addresses. A different
build requires new validation of the addresses.

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

Encrypted stock PAK work must remain fingerprint-gated and fail closed when the
callback or encrypted-index layout changes. Validate a recovered default key by
decrypting the version-12 index and matching its footer SHA-1; readable text is
not sufficient evidence. UnrealPak key-chain JSON uses Base64 key bytes, while
legacy `-aes` interprets its value differently. Keep keys, local crypto JSON,
extracted files and raw reports below ignored `artifacts/`; none may enter Git
or a distributed mod package. Railgun no longer consumes this path.

## Package serialization is a separate compatibility contract

Matching export classes and decoded property values does not establish runtime
equivalence. Package flags, property-stream encoding, object/legacy versions
and resolved custom versions form a separate contract with the game reader.
Successful registration, UI display or parser reopen alone does not prove
that a gameplay consumer can use the reconstructed package.

For the fingerprint-bound Railgun ammunition consumer, registration and UI
display were insufficient when the package-format contract was wrong. The
game-validated item is authored directly from owned JSON and preserves its
reviewed unversioned property stream, object versions and custom-version
container. The current contract and validation scope live in
[Railgun architecture](../mods/Railgun/ARCHITECTURE.md); fabrication and pickup
consumer boundaries live in
[item fabrication and pickup](voyage-item-fabrication-and-pickup-knowledge.md).

This does **not** prove that all tagged Voyage packages fail or authorize bulk
conversion of other assets. Treat serialization as an asset-specific contract
derived from reviewed source metadata and mappings, not as a universal list of
flags or fixed entry counts.

Do not flip `IsUnversioned` / `PKG_UnversionedProperties` in a header or exported
JSON and assume the payload has been converted. Author the complete package JSON
with the intended property stream, stored object/custom versions and reviewed
mapping, then write and reopen it with the canonical UAssetGUI/UAssetAPI writer.
Compare serialization metadata and intended semantics before container
verification and a real-game test of the actual consumer. Reopen verifies
reader/writer consistency, not gameplay compatibility.

## Known boundaries

- Source audit of installed Unreal 5.8.2 (CL 56702186, compatible CL
  55116800) found no new object/custom-version discriminator in
  `Engine/Source/Runtime/CoreUObject/Private/UObject/ObjectResource.cpp`,
  `operator<<(FStructuredArchive::FSlot, FObjectImport&)`: PackageName uses
  the old `VER_UE4_NON_OUTER_PACKAGE_IMPORT` threshold; filtering changes the
  placeholder/reset behavior, not whether the field exists. The installed
  `ObjectVersion.h` UE5 enum still ends at IMPORT_TYPE_HIERARCHIES (1018).
  This is direct 5.8.2 source evidence, not a full 5.7-to-5.8 source-history
  audit or proof that no other package metadata could distinguish producers.
  Do not invent a newer Unreal object-version number to distinguish layouts.
- GUI Save's current selection override is intentionally narrower than full
  target-version conversion: SetSerializationEngineVersion changes only the
  explicit hint. ObjectVersion/ObjectVersionUE5/CustomVersionContainer remain
  source metadata, and GetEngineVersion still derives from those fields.
  Thus the selected version owns the import-layout hint, not every serializer
  branch. Do not describe this as unconditional cross-engine Save conversion.

- `SpecifiedEngineVersion` is runtime-only and ignored on JSON read/write.
  JSON retains stored object/custom versions, while every binary GUI save and
  API JSON write must apply an explicit serialization hint immediately before
  `Write`. Voyage uses `UE5_8`. This is not general cross-engine schema migration.
- UAssetGUI forwards its selected profile to retoc. `UE5_8` uses the expanded
  Voyage import layout; UE5.7, older profiles and unspecified library callers
  preserve the upstream writer layout. The accepted tools read both reviewed
  layouts, but that does not establish arbitrary runtime compatibility.
- Full JSON intentionally omits API `OverrideNameMapHashes`. No-op JSON
  roundtrips can therefore recompute name hashes even after the version fix.
  Tests distinguish raw-export/schema errors, original object/custom versions,
  `.uexp` identity and whole-package identity. Do not claim byte-identical
  `.uasset` JSON roundtrips or normalize arbitrary differing header fields.

- Single-package `to-zen` does not require copying referenced packages into
  the input folder. UE5 external imports are represented by package IDs and
  public export hashes (`resolve_zen_package_import`). A missing-script-import
  warning followed by an out-of-range outer index can instead indicate a
  misread legacy import table; suppressing the warning or dropping references
  would not repair it.
- Filtered legacy imports may omit or retain `FObjectImport.PackageName`.
  Loose outputs from writers using different layouts are not interchangeable
  with readers that assume only one stride. Accepted retoc `49b7721` validates
  and reads both reviewed layouts. Keep extraction, editing and packaging
  profiles consistent, and do not infer the producer from the shared CLI
  version string alone.
- A successful `retoc verify`, parse, cook, or container load is not gameplay
  validation.
- Import traversal must reject null, out-of-range, invalid-name and cyclic
  references with bounded contextual errors. Determine import stride from
  checked table boundaries and counts; unknown layouts fail closed without a
  speculative version retry. Unfiltered records retain `PackageName`. Reading
  both filtered layouts does not make older readers accept expanded UE5.8 output.
- Source legacy version and target IoStore version are separate contracts.
  A source reader must use the source summary layout, while the target writer
  uses the requested output profile. Do not reuse a target version as an
  unproven source fallback or normalize differing header fields without an
  explicit version-bound comparison contract. Run failing conversions through
  `Invoke-VoyageBoundedTool.ps1`; never repeat an unbounded allocation failure.
- `VerifyBinaryEquality()` and fully parsed exports are independent gates:
  `RawExport` can preserve unknown bytes while hiding a parse failure.
- Legacy recovery can succeed at extraction while failing at property parsing.
  In the ScopeFix investigation, retoc recovered two packages despite missing
  directory-index filenames; incomplete historical dependencies left unknown
  imports. CUE4Parse decoded early camera vectors but failed on later fields.
  Treat such JSON as partial evidence: independently confirm any used value
  against its serialized payload, and never infer a complete historical diff
  or schema compatibility from readable early properties. Current mappings
  and dependencies do not establish the old package's provenance. For current
  builds, change fresh fingerprint-matched stock assets and validate the result.
- A normal save of a directly opened binary and a save after full-JSON import
  are different paths. JSON does not carry UAssetAPI's runtime-only engine
  hint, so the writer must apply the explicit `UE5_8` serialization hint after
  deserialization while preserving stored object/custom versions. Do not call
  a broader engine-version setter that rewrites source metadata.
- UE 5.8 filtered imports serialize `FObjectImport.PackageName`; older filtered
  fixtures do not. The API fix is deliberately gated on `VER_UE5_8`.
- Update 5.5 reports numeric `Voyage.exe` version `5.8.1.0`; the available
  editor is UE `5.8.2`. Loose `.uasset/.uexp` cooking requires the documented
  `-SkipZenStore` path. These are distinct game and toolchain identities.
- Keep the retoc binary identity, selected profile and extraction manifest
  together. Accepted retoc `49b7721` uses explicit `UE5_8` for Voyage's
  expanded imports, while `UE5_7` deliberately selects the upstream filtered
  layout. A profile name alone is not a cross-checkpoint serialization guarantee.
- The canonical CUE4Parse bundle is managed-only. Its publisher uses a
  temporary host to replace upstream `Microsoft.Bcl.Memory 9.0.0` with
  `10.0.11` without modifying the upstream checkout. NuGet can still print the
  dependency project's upstream audit warning during compilation; the
  published manifest, DLL metadata, and `VoyageAssetInspector` output were
  verified to resolve `10.0.11`.
- A `.NET` dialog with exit code `0xE0434352` means an unhandled managed
  exception; inspect the application error and logs before blaming the CLR.
- Two valid retoc builds can differ in physical chunk order, and repeated Unreal
  cook/SavePackage runs can change cooked package bytes even with the same
  gameplay source. Whole-build byte equality is therefore not a reproducibility
  requirement. Compare the same source/tool/game provenance, package identities
  and IDs, exported imports/exports/references and properties, Blueprint
  pseudocode, and sorted retoc semantic inventory. Do not discard a differing
  field as volatile until the normalization contract proves it. Reserve exact
  hashes for a prepared release artifact versus its installed or archived copy.
  Any unexplained semantic difference or new compatibility claim still requires
  a real-game canary; the normalization contract itself remains active research.
