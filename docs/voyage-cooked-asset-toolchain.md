# Voyage cooked-asset toolchain

This document records the accepted contracts and limits of the cooked-asset
workflow. Commands and parameters remain owned by `tools/README.md`.

## Operating rule

Use the repository scripts as black boxes first. Pick the operation from the
tool-routing table, run the documented interface, and consume its diagnostics,
manifest, summary, and output. Do not spend an ordinary task re-reading the
implementation or reconstructing the dependency chain.

Investigate internals only when a tool exits non-zero, crashes or opens a crash
dialog, hangs, rejects valid-looking inputs, returns an unexpected result, or
lacks a documented operation required by the task. Diagnose from the outside
in: inputs and fingerprint, paths and exact dependency commits, mapping gate,
tool log/manifest, minimal reproducer, then source.

## Accepted checkpoint

The accepted UE 5.8 checkpoint is:

- retoc fork `ihsoft/retoc` at `49b7721`;
- jmap fork `ihsoft/jmap` at `4f88d8a`;
- UAssetAPI fork `ihsoft/UAssetAPI` at `21c982f`;
- UAssetGUI fork `ihsoft/UAssetGUI` at `df18b5f`, using that UAssetAPI commit;
- unmodified CUE4Parse upstream at
  `ec6595e46448a817ac21ea9bde01caa48f80a420`;
- canonical compact GUI executable at `.tools/bin/UAssetGUI.exe`, published by
  `tools/Publish-UAssetGuiBinary.ps1`, size `12,762,803` bytes and SHA-256
  `42837CD279A78DF57B537020A0169C5D2259A4570D3B62F4B96852F0F5C27F96`;
- canonical retoc at `.tools/bin/retoc.exe`, size `6,833,664` bytes and SHA-256
  `6F8F86AE3FD747A3B785E787A33C24F9A11735D03664948D6B619F18861650F5`;
- canonical jmap dumper at `.tools/bin/jmap_dumper.exe`, size `9,472,000` bytes
  and SHA-256
  `75E693C2ACD22BB63671EF368C8608931CEF59E8F6F441AFFA3F7A72A3D11543`;
- canonical UAssetAPI at `.tools/bin/UAssetAPI/UAssetAPI.dll`, size `4,210,176`
  bytes and SHA-256
  `6DF2606BBA89987AEB4BF1EFBD3C64AC565DBC5D6113A0A7A5062C7CD8B249FD`;
- canonical managed CUE4Parse at `.tools/bin/CUE4Parse/CUE4Parse.dll`, size
  `4,025,344` bytes and SHA-256
  `F304981BAD4C53D209DFDABA9EB65A01D825572E543A04914BEBFD3538DCF4FD`;
- canonical VoyageExecutableInspector at
  `.tools/bin/VoyageExecutableInspector.exe`, size `196,700` bytes and SHA-256
  `3A33483362EF5BA122C370C76322A6A4012BEB298D58A07C67CD6045A2F6C718`,
  built from source checkpoint `79d01b3`.

For Steam build `25056839`, game UE `5.8.1`, executable SHA-256
`CA84428CF4562C703BEDFF053DB727D14CC70C593451C09BE75A92828EFD9933`,
the reviewed mapping is
`mappings/Voyage/steam-25056839-ue5.8.1/Voyage-25056839.usmap`.
Its sibling manifest owns the generator identity, file hash, validation
evidence, and invalidation condition.

Do not infer that a newer fork checkout, similarly named binary, or mapping at
another path has the same guarantees. Builders and tests should use or verify
these identities explicitly.

Promotion, runtime-only JSON version handling, validation and exact manifests
are recorded in [the JSON save checkpoint](voyage-json-save-checkpoint.md).

## Operations

Use the wrappers listed in `tools/README.md`:

- fingerprint the installation with `Get-VoyageBuildFingerprint.ps1`;
- resolve the existing reviewed mapping with `Get-VoyageMappings.ps1`; create a
  new candidate with `New-VoyageMappings.ps1` only after a confirmed unmatched
  game fingerprint, then validate it with `Test-VoyageMappings.ps1`;
- cache an asset as versioned JSON with `Get-VoyageAssetJson.ps1`;
- answer ordinary Blueprint structure questions with
  `Get-VoyageAssetSummary.ps1`, selecting only the needed focus and opening the
  full returned summary only when the compact records are insufficient;
- extract an exact loose package with `Extract-VoyagePackage.ps1`;
- inspect packages and reflection with `Inspect-VoyageAsset.ps1`;
- publish and stress the exact GUI with `Publish-UAssetGuiBinary.ps1` and
  `.tools/bin/UAssetGUI.exe stress-open`;
- publish retoc, jmap, UAssetAPI, and CUE4Parse through their dedicated
  `Publish-*` wrappers, then consume only `.tools/bin/` on the normal path;
- publish the tracked Inspector through `Publish-VoyageAssetInspectorBinary.ps1`;
  both inspection wrappers resolve `.tools/bin/VoyageAssetInspector.exe` with
  `Get-VoyageAssetInspectorBinary.ps1`. Runtime paths check provenance and never
  build or restore. The framework-dependent EXE requires .NET 10 and external
  reviewed mappings; source/dependency changes invalidate it, not a new query;
- run native name, address, and member-offset correlation through
  `Invoke-VoyageExecutableInspector.ps1`; it resolves the validated EXE and
  returns compact fingerprinted evidence while retaining the detailed report.
  Publish through `Publish-VoyageExecutableInspectorBinary.ps1` only after an
  intentional committed source change. Its output does not prove reflected
  ownership, call relations, or lifecycle;
- build or prepare fork source only while deliberately changing a dependency
  checkpoint or diagnosing an unexpected publisher/tool result;
- create the common schema-2 manifest for an ordinary triplet and ZIP, or
  schema 3 when the release owns one exact content-plugin descriptor, with
  `New-VoyageReleaseManifest.ps1`; give it the exact source scope and let its
  installer validation gate publish the immutable manifest;
- validate or install an already-built standalone IoStore release through
  `Install-VoyageRelease.ps1`, preserving its exact archive and installation
  transaction evidence; restore its predecessor through
  `Restore-VoyageReleaseInstallation.ps1` rather than copying backups manually;
- install and remove unchanged runtime canaries only through the hash- and
  fingerprint-gated probe scripts.

The script is the reusable method; game-derived JSON, packages, reports, raw
mapping candidates, and test containers stay under ignored `artifacts/`.

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

Inspection on Steam build `25191271`, executable SHA-256
`747DC2553F7E68D8EA7ED0B2E0CAC6D08943EA3F50DD6ED822E9293E0B45F58B`,
first confirmed this layout in an external mod: a loose content-only descriptor,
one plugin-local registry in its PAK, and Item/Skill packages under the stock
`/Game/Data/Assets` roots in IoStore. Structural inspection confirmed a native
VoyageSkill referencing a native VoyageItem with crafting properties and an
owned actor. That external package remains structural evidence rather than a
general gameplay or initialization-order guarantee.

Railgun's normal source producer derives an exact three-record plugin registry
from its owned package readbacks and explicit registry policy, packages it at
`Voyage/Mods/RailgunCatalogue/AssetRegistry.bin`, and releases a loose
content-only descriptor through the schema-3 manifest contract. The installed
`build-20261002-donor-free-03` artifact was confirmed by the user with the
general result that everything works, including while `VoyageAssetPool_P`
supplied a global registry override. This validates additive loading for that
combination; it does not prove arbitrary registry overrides, duplicate IDs or
additional individually unspecified gameplay scenarios. Build artifacts remain
evidence, not source inputs. Plugin descriptors extend the installed footprint
beyond `Content/Paks`, so schema-3 installation and restoration retain the
common backup, hash, path, reparse-point and closed-game gates.

## Stock AssetRegistry access and PAK encryption

The following offline findings apply only to Steam build `25191271`, executable
`VoyageSteam-Win64-Shipping.exe`, SHA-256
`747DC2553F7E68D8EA7ED0B2E0CAC6D08943EA3F50DD6ED822E9293E0B45F58B`.
Re-fingerprint the installation before using these addresses; a changed EXE
requires new validation, not reuse of its predecessor's offsets.

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

## Validation ladder

Passing a lower level never implies a higher one:

1. fingerprint and dependency provenance match;
2. the mapping passes header, payload, required-schema, hash, and fingerprint
   gates;
3. the asset opens with no unexpected `RawExport` fallback;
4. unchanged save is binary-equal and reopens;
5. independent CUE4Parse inspection agrees with the intended structure;
6. retoc converts, verifies, and reports the exact semantic inventory;
7. an independent rebuild has equivalent reviewed package structure and
   Blueprint behavior, when byte equality is not expected;
8. a separately named unchanged or deliberately changed canary loads in the
   game;
9. the requested runtime behavior is tested in the real game.

The accepted writer passed broad unchanged-asset regression and a changed-save
canary. That evidence validates the tested layouts and workflow, not arbitrary
structural edits to every asset.

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
[item fabrication and pickup](voyage-item-fabrication-and-pickup.md).

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
- The game is UE `5.8.1`, while the available editor is UE `5.8.2`; loose
  `.uasset/.uexp` cooking requires the documented `-SkipZenStore` path.
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
