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
- canonical VoyageAssetPatcher at `.tools/bin/VoyageAssetPatcher.exe`, size
  `5,556,137` bytes and SHA-256
  `4298009F9034E9F5D93BFF343ED9613D35F1F501EB7F4B6AAB5A390EDDBBA1A9`,
  built from patcher source checkpoint `9d32697` and accepted UAssetAPI
  `21c982f`;
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
- run normal surgical patch operations through `Invoke-VoyageAssetPatcher.ps1`,
  which resolves the current mapping and validated executable; publish the EXE
  through `Publish-VoyageAssetPatcherBinary.ps1` only after an intentional
  committed patcher or accepted UAssetAPI change;
- run native name, address, and member-offset correlation through
  `Invoke-VoyageExecutableInspector.ps1`; it resolves the validated EXE and
  returns compact fingerprinted evidence while retaining the detailed report.
  Publish through `Publish-VoyageExecutableInspectorBinary.ps1` only after an
  intentional committed source change. Its output does not prove reflected
  ownership, call relations, or lifecycle;
- build or prepare fork source only while deliberately changing a dependency
  checkpoint or diagnosing an unexpected publisher/tool result;
- create the common schema-2 manifest for an already-built triplet and ZIP with
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

To recover and validate the key for this exact executable:

1. Use `Invoke-VoyageExecutableInspector.ps1` for references and
   `Inspect-VoyageNativeMemberAccess.py` for bounded instruction decoding.
   The verified chain is the encryption-key delegate getter at preferred VA
   `0x141422E30`, registration function `0x14142D160`, initializer
   `0x14120CF70`, and callback `0x1456AD1F0`. These are offline PE addresses
   with image base `0x140000000`, not ASLR-adjusted process addresses.
2. Decode the callback at RVA `0x56AD1F0`. It writes eight immediate DWORDs
   to stack offsets `-0x30`, `-0x2C`, `-0x28`, `-0x24`, `-0x20`, `-0x1C`,
   `-0x18`, `-0x14`, then copies those 32 bytes to the output. Concatenate
   the DWORD values in that order, each little-endian. Reject a different
   instruction shape rather than reading unchecked offsets.
3. Read the PAK index position, length and SHA-1 from its footer. Decrypt the
   index using AES-256 ECB with no padding removal and compare its SHA-1
   against the footer. This check passed: the decrypted index has mount
   `../../../` and 4,748 entries. Readable text alone is not key validation.
4. For UnrealPak access, supply a local crypto-key JSON through `-cryptokeys`.
   UE 5.8 `Core/Public/Misc/KeyChainUtilities.h` reads the default key from
   `EncryptionKey.Key` as Base64 of the 32 bytes. Do not pass hex to legacy
   `-aes`, which interprets its value as ANSI characters. List the stock PAK,
   confirm the registry's exact virtual path, then extract only that file.
   Record the PAK and extracted-file hashes with the game fingerprint.

The verified gate here is index decryption and hash equality. It does not by
itself prove encryption of the registry payload, successful registry extraction,
or runtime Primary Asset ID registration. Those require separate evidence.
Keep recovered key material, local crypto JSON, extracted registries and raw
reports under ignored `artifacts/`, never in Git. Keys and crypto JSON must not
enter distributed mod packages. Packaging a rebuilt registry is a separate
operation requiring its own compatibility and runtime validation.

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

The preceding e362030 / 6b5ead3 checkpoint passed all `27/27` upstream binary-roundtrip
tests. A stress run over `Voyage/Content/Blueprints` attempted `1067` assets:
`1057` were clean, `10` were reviewed opaque/numeric notices, `0` failed, and
all were binary-equal. The changed-save canary for
`BP_ToolAbility_Maintenance_Dismantle` loaded in the game and applied
`Duration = 1`. These results validate the tested layouts and workflow, not
arbitrary structural edits to every asset.

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

- The promoted JSON/engine-selection checkpoint fixes the confirmed
  full-JSON failure; the user confirmed the rebuilt gui-review-build candidate
  works in interactive GUI testing on 2026-09-04 (exact identity in backlog).
  API changes address the
  full-JSON path. Per user decision, `SpecifiedEngineVersion` is runtime-only
  and ignored on JSON read/write, including old candidate JSON containing it.
  JSON-open restores the original object/custom versions without applying an
  engine hint. Every binary GUI Save sets the current dropdown selection through
  SetSerializationEngineVersion immediately before Write. CLI `fromjson` can
  supply a final explicit version; Voyage JSON callers must supply UE5_8 before
  writing. The setter preserves original object/custom versions; UNKNOWN leaves
  the current runtime hint unchanged.
  This does not implement general cross-engine asset/schema migration.
- The same checkpoint adds retoc `UE5_8` and forwards the actual GUI selection
  to IoStore extraction instead of hardcoding `UE5_7`. UE5.8 uses the expanded
  import writer; UE5.7, older versions and unspecified library callers restore
  upstream writer bytes. Both validated filtered layouts remain readable.
  Fresh Dismantle output at UE5.7 matched upstream `885a8da` byte-for-byte;
  UE5.8 matched the previous canonical `234f4e5`. The four producer/profile cases packaged
  and verified successfully. This proves the tested serialization boundary,
  not arbitrary runtime compatibility or a native UE5.7 Voyage fixture.
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
- retoc fork `234f4e5` changed filtered `FObjectImport` reading and writing to
  include `PackageName` unconditionally. Upstream `885a8da` omits it when
  editor-only data is filtered. Their loose legacy outputs are therefore not
  interchangeable. A fresh single-package Dismantle experiment on the accepted
  Steam `25056839` fingerprint reproduced the reported `18 / 226` panic when
  upstream extraction was read by the fork. Both same-binary roundtrips
  packaged exactly one asset and passed container verification; the reverse
  mixed roundtrip also panicked (`18 / 251`). This is packaging evidence only.
  Keep extraction, editing, and packaging layout contracts consistent; do not
  infer the producer from the shared CLI version `0.1.5`. Promoted retoc 49b7721 repairs cross-layout reading; the preceding failure is historical evidence for the fix.
- A successful `retoc verify`, parse, cook, or container load is not gameplay
  validation.
- The previous retoc `234f4e5` is Voyage-checkpoint evidence, not a passing
  upstream-regression-suite claim: the UE5.4 lamp fixture exposed an infinite
  Outer traversal after a misread import table, ending in a 128-GiB allocation
  request. The candidate rejects null/out-of-range/name/cyclic references with
  contextual errors and determines import stride from checked table boundaries
  and count. Filtered records may omit or retain PackageName; unfiltered records
  must retain it. Unknown sizes fail closed, with no speculative version retry.
  Its UE5_8 writer remains byte-identical for tested Voyage input; the
  explicit UE5_7 writer now matches upstream as described above. Reading both
  layouts does not make older upstream readers accept expanded UE5_8 output.
- Source legacy version and target IoStore version are separate contracts.
  The upstream test reused unversioned UE5.6 fixtures for UE5.7 but passed the
  target's version to the source reader. UE5.7 adds ImportTypeHierarchies in the
  legacy summary, so that assumption misaligned subsequent fields. The repaired
  harness keeps source fallback 1017 and target 1018, preserving all original
  structural/payload assertions. It tests 5.6-source/5.7-target compatibility,
  not native UE5.7 legacy fixtures. The earlier candidate passed 28/28;
  the engine-selection candidate passed 29/29 (evidence in the active backlog).
- Fresh Dismantle conversion from upstream, canonical fork, and candidate
  legacy output passes verify and exact single-asset inventory in the candidate.
  Candidate to-legacy preserves the canonical .uasset, .uexp, and ScriptObjects
  bytes. Cross-producer raw Zen differs only in CookedHeaderSize (22129 versus
  21295), copied from each source header; all other bytes match. The version-bound
  test verifies each stored value against the source before normalizing that
  field in memory. This is not permission to ignore arbitrary offsets or
  metadata in other comparisons. The candidate is not yet published or
  game-validated; its evidence and approval gate remain in the backlog.
  Further failing conversions use `Invoke-VoyageBoundedTool.ps1` with memory
  and time limits; never repeat an unbounded allocation regression.
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
- At the accepted API `6b5ead3` checkpoint, full-asset JSON deserialization
  loses UAssetAPI's internal
  `SpecifiedEngineVersion`. Both GUI JSON opening and CLI `fromjson` need an
  explicit version-preservation contract; selecting UE 5.8 in the GUI does not
  by itself prove that a JSON-created asset carries that contract. At API
  `6b5ead3`, filtered import serialization uses that internal value to decide
  whether to write `FObjectImport.PackageName`. A fresh
  `BP_FabricationPlacementComponent` full-JSON unchanged roundtrip reproduced
  malformed imports and 22 RawExports on Steam `25056839`. Direct binary
  unchanged save was byte-identical. Reapplying `SetEngineVersion(VER_UE5_8)`
  after JSON import restored parsing, but also resets custom versions and did
  not preserve the original header bytes; it is diagnostic evidence, not an
  accepted production fix. Preserve the exact original version metadata when
  evaluating the repair candidate described above. A normal Save of a directly opened binary and a
  Save of a full-JSON-imported asset are different paths.
- UE 5.8 filtered imports serialize `FObjectImport.PackageName`; older filtered
  fixtures do not. The API fix is deliberately gated on `VER_UE5_8`.
- The game is UE `5.8.1`, while the available editor is UE `5.8.2`; loose
  `.uasset/.uexp` cooking requires the documented `-SkipZenStore` path.
- Canonical retoc `234f4e5` historically uses its UE5_7 profile for this
  Voyage checkpoint. The engine-gated candidate requires explicit UE5_8 for
  Voyage's expanded legacy imports; UE5_7 deliberately chooses upstream
  filtered-import output. Keep the binary checkpoint, selected profile and
  extraction manifest together; the same profile name is not a cross-checkpoint
  serialization guarantee. Use the wrapper's documented contract.
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
