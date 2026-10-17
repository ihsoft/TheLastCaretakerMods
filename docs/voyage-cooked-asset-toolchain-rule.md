# Voyage cooked-asset toolchain rules

This document owns the repository workflow for cooked-asset tools. Game and
engine contracts are in [toolchain knowledge](voyage-cooked-asset-toolchain-knowledge.md);
commands and parameters are in [the tool index](../tools/README.md).

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

## Tool boundary

The public scripts in [`tools/README.md`](../tools/README.md) select the
published retoc, jmap, UAssetAPI/UAssetGUI, CUE4Parse and executable-inspection
tools needed for each operation. Run the selected public interface first and
evaluate its result. Publisher manifests retain binary provenance for diagnosis;
normal callers do not compare tool commits, sizes or hashes in advance.

If a tool fails or produces unexpected output, inspect its diagnostics and
publication identity before changing the asset contract. The reviewed mapping
still has its own file-integrity and schema gates.

Publisher manifests record exact binary provenance. Runtime-only JSON version
handling and its boundaries are in [toolchain knowledge](voyage-cooked-asset-toolchain-knowledge.md#known-boundaries);
command interfaces remain in the [tool index](../tools/README.md).

## DML installation

Install each new DML mod's `.pak`, `.ucas`, and `.utoc` together directly in
`Voyage/Content/Paks/LogicMods`. Release instructions target that directory;
extract a ZIP containing root-level container files into `LogicMods`, not the
parent Paks directory. Validate first-time discovery without relying on an
existing saved registration or the `DML add <ModName>` command. The
[observed discovery contract](voyage-cooked-asset-toolchain-knowledge.md#dml-actor-identity-and-physical-installation)
explains this layout.

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
  `Install-VoyageRelease.ps1`, retaining a selected release ZIP outside the
  game and using temporary transaction data during mutation; reinstall the
  previous ready package if a game test fails;
- install and remove unchanged runtime canaries only through the hash- and
  fingerprint-gated probe scripts.

The script is the reusable method. Generated output follows the
[artifact lifecycle](artifact-lifecycle-rule.md): use ignored `Tmp/` by default
and retain only selected output with a continuing use under `artifacts/`.

## Validation ladder

Passing a lower level never implies a higher one:

1. relevant Steam build and asset provenance match;
2. the mapping passes header, payload, required-schema, hash, and Steam build
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

The writer supports the reviewed unchanged-asset and changed-save layouts; it
does not guarantee arbitrary structural edits to every asset.
