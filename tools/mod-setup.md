# Mod setup, shared execution and runtime settings

## New mod skeleton

Use [`New-ModSkeleton.ps1`](New-ModSkeleton.ps1) after choosing a mod directory
name and one-sentence goal. It creates only `AGENTS.md`, `ARCHITECTURE.md`,
`README.md`, `CHANGELOG.md`, and `version.txt` (`0`) under a new
`mods/<Mod>/` directory.
It does not inspect the game, select an implementation, build, install, or
overwrite an existing directory. The command emits a compact JSON result with
the new path and files. See the [new-mod workflow](../docs/new-mod-rule.md)
for when to add other files.

For an implemented mod's folders and command conventions, use the
[standard mod setup](../docs/unreal-project-rule.md#standard-mod-layout).
The skeleton generator does not scaffold a project or build/test/install scripts.

```powershell
python -B tools/run_powershell.py tools/New-ModSkeleton.ps1 `
  -Name BlueLargeContainers `
  -Summary 'Make all large containers blue.'
```

`Name` must be 1-64 ASCII letters or digits, starting with a letter; Windows
device names are excluded. `Summary` is one line of 3-240 characters.
The changelog starts with an empty `Next` section; fill it with player-facing
changes before publication and never include it in the Vortex ZIP.

## Shared mod pipeline

Normal authoring, build and test work starts at the mod's indexed public
command. When implementing or unifying those commands, import
[`VoyageModPipeline.psm1`](VoyageModPipeline.psm1). Its functions provide the
repeated mechanics; the mod supplies its inputs, assertions and packaging policy.
Launch these entry points through the [shared PowerShell launcher](powershell-execution.md).
Reserve the
[Unreal queue](unreal-coordination.md) around the whole build or editor test.

| Function | Interface and result |
| --- | --- |
| `New-VoyageModRun` | `-ModRoot`, optional `-OutputRoot` and `-Kind`; creates and returns a fresh run directory below `Tmp/<Mod>/`. |
| `New-VoyageModProjectCopy` | `-ModRoot -Destination`; copies maintained inputs and returns the `.uproject` path without launching Unreal. |
| `Remove-VoyageModScratch` | `-ModRoot -RunRoot -Path`; removes only an unlinked descendant of that mod's run, preserving the run root and its evidence. |
| `Get-VoyageModSourceSnapshot` | `-ModRoot -Paths`; captures current file bytes or absence for mod-relative paths before generation. Returns an in-memory hashtable. |
| `Save-VoyageModSourceUpdate` | `-ModRoot -RunRoot -Paths -Snapshot`; accepts selected validated files from matching paths under the run into maintained source, checking the snapshot first. |
| `Get-VoyageProjectContentSnapshot` | `-ProjectRoot`; captures all files under `Content/`, keyed by project-relative path. |
| `Assert-VoyageProjectContentUpdateScope` | `-ProjectRoot -Before -AllowedPaths`; rejects added, removed or changed content outside the supplied project-relative paths. |
| `Invoke-VoyageModEditorTest` | `-ModRoot -EngineRoot`, optional `-OutputRoot`, `-ReportName`, `-ExpectedSuiteCount`, and a required `-Run` script block; prepares a copy, runs the supplied checks, requires a passing report and removes the project only on success. Returns `status`, `reportPath`, `evidence`, `bptWorkspace`. |
| `Invoke-VoyageModCook` | Ordinary combined cook and packaging: `-ModRoot -ProjectFile -EngineRoot -OutputRoot -Packages -ParserVersion`. Optional `-TaggedPackages`, `-AdditionalCookArguments`, `-GameRoot`, `-ContainerName`, `-Retoc`. Returns `status`, `container`, `stage`, `validation`, `scriptObjects`. |
| `Invoke-VoyageModCookAssets` | The same cook/staging stage without packaging: `-ModRoot -ProjectFile -EngineRoot -OutputRoot -Packages`, optional `-TaggedPackages` and `-AdditionalCookArguments`. Returns `stage`, `packages`, `taggedPackages`. Use when a documented serialization conversion must run before packaging. |
| `New-VoyageModContainer` | Package a prepared stage: `-ModRoot -Stage -OutputRoot -Packages -ParserVersion`. Optional `-GameRoot`, `-ContainerName`, `-Retoc`, `-CompanionPak`. Returns the same container result as the combined operation. |

For example, a mod's project-copy wrapper needs only the shared operation:

```powershell
Import-Module ./tools/VoyageModPipeline.psm1
New-VoyageModProjectCopy -ModRoot './mods/MyMod' `
  -Destination './Tmp/MyMod/authoring/Project'
```

The copy validates source entries and rejects linked paths and existing output
before copying. It includes current uncommitted inputs, omits generated
intermediates, and permits absent optional source directories. It does not
invent missing project content or accept unexpected generated folders.

The test block receives one context object with `repo`, `modRoot`, `output`,
`project`, `projectRoot`, `reportPath` and `engineRoot`. It runs the owning suite
and writes the report at `reportPath`; a missing/failed report or mismatched
required suite count fails the operation. Restore any environment variables
introduced by the block in `finally`. Failed runs retain their project and
diagnostics; inspect the reported run directory before retrying with fresh output.

### Explicit source-update acceptance

Use these operations when a generator or importer must produce editor assets in
a disposable project. Ordinary build/test commands do not accept source changes.
The mod owns the recipe, output validation and exact file list; shared acceptance
does not establish asset correctness or synchronize BPT.

Before generation, capture the acceptance paths with `Get-VoyageModSourceSnapshot`.
Include any recipe files whose concurrent edits would invalidate the result;
these can be watched without being copied back. Paths are relative to the mod;
both slash styles are accepted, but rooted paths, traversal and aliases are not.
After validating the generated outputs, call `Save-VoyageModSourceUpdate` with
the original snapshot and an ordered subset of those paths: assets first,
source-state records last. Each new file must exist at the same relative path
under `RunRoot`, an unlinked descendant of `Tmp/<Mod>/`.

When a generator can also modify dependencies, capture its working project's
content before running it and call `Assert-VoyageProjectContentUpdateScope`
afterward. `AllowedPaths` names the intended files relative to that project,
including `Content/`. An unexpected change stops acceptance for review; it is
not silently included or discarded. This check belongs to that update operation,
not to every build.

Acceptance rejects concurrent edits to all captured source paths. Before copying,
it saves the selected previous files under `RunRoot/source-backup/`, preserving
their mod-relative paths; `absent-files.txt` lists previously absent files.
Success removes this backup. Generation or validation failure leaves maintained
source unchanged. A copy failure or interruption can leave partial changes:
preserve the run, review current edits and restore affected files from the backup
or Git; remove partially created files listed as previously absent when restoring
the old state. The copy is not atomic and does not lock out concurrent writers;
there is no recovery journal or automatic rollback. Review accepted changes and
synchronize affected BPT before building or committing.

### Cook and container stages

The mod supplies one explicit `/Game/` package inventory. Cook, staging and exact
container verification consume that same inventory; do not maintain duplicate
copy lists. `TaggedPackages` is a subset of that inventory that needs named
property tags; remaining packages use unversioned properties. The mod architecture
owns the reason for choosing a different serialization mode. Ordinary additive
mods can omit this option.

The cook operation prepares the private `UnrealCookSupport` editor provider in
the working project through the shared support tool. It passes each group's
inventory by file, calls Unreal's stock cooker, checks its output serialization,
and stages every group before another cook can clear the cook directory. It
preserves asset companions including `.m.ubulk`. The provider has no game types
or runtime payload and is reused through the normal editor-support cache.
Additional cook arguments may select renderer/shader/cache policy, but may not
replace the package inventory, serialization mode, commandlet or output target.
The working project must be below the supplied run `OutputRoot`.

For a required cooked-format conversion, call `Invoke-VoyageModCookAssets`,
apply the bounded mod-owned conversion to its returned stage, then call
`New-VoyageModContainer` with the same inventory. Do not copy cook or container
orchestration into that converter. The container output must be a fresh directory
separate from the stage, both under `Tmp/<Mod>/`.

Supply `GameRoot` when packaging needs the installed game's script-object table.
The container stage extracts only metadata from `global.utoc` with assets and
shaders disabled, copies `scriptobjects.bin` into the stage, and records its
source paths and extraction report. It does not extract an unrelated game asset,
refresh maintained project inputs, or reuse a caller-selected old extraction.
Without `GameRoot`, no game metadata is added. The producer remains responsible
for its game/source-build applicability checks.

`CompanionPak` supplies a caller-prepared PAK for extra files such as a plugin
registry. It replaces retoc's generated companion PAK before final container
verification; the caller owns validation of those extra files and their mount
paths. It is not merged with another PAK. Ordinary asset-only releases omit it.
Container verification runs once on the final triplet against the supplied
inventory. Stage/conversion/extraction failures retain diagnostics in the run.

Archive creation and manifest production continue through the
[release tools](release-build.md); these stages do not install or publish.
Remove the returned stage and working project after success, retaining the
container and validation reports under the [output lifecycle](../docs/artifact-lifecycle-rule.md).

For changes to these mechanics, run
[`Test-VoyageModPipeline.ps1`](Test-VoyageModPipeline.ps1) in Windows PowerShell
5.1. It uses isolated fixtures below `Tmp/tests/`, including junction rejection
and test failure evidence. Installation changes use
`Test-Install-VoyageRelease.ps1 -WithSettings`, also with
`-WithContentPluginDescriptor` when that payload is affected; it covers complete
INI replacement and coupled rollback against a synthetic game directory.
These checks launch no Unreal process and write no real installed game files.
Then run the affected
mods' public tests/builds for changes at those boundaries.

## Runtime INI preparation

When changing a settings schema or its canonical INI, run
[`Validate-VoyageModSettings.ps1`](Validate-VoyageModSettings.ps1). It validates
case-sensitive keys, number/boolean/quoted-string values, schema fallbacks,
optional inclusive ranges, final storage, and duplicate-free repository INI.
It returns `status: passed`, the setting count and the two input paths, and writes
no files. Invalid input fails with the offending setting; correct the input and
rerun. Ordinary builds copy the canonical INI unchanged without repeating this
validation or tracking a separate validation cache.

```powershell
python -B tools/run_powershell.py tools/Validate-VoyageModSettings.ps1 `
    -SchemaPath './mods/MyMod/Settings/MyMod.settings.json' `
    -DefaultIniPath './mods/MyMod/Assets/MyMod.ini'
```

The [BPT INI library](BlueprintText/Library/IniFileParser/integration.md) supplies the
runtime parser; the consuming graphs own getters, fallbacks and application of
values. This preparation path generates no header or graph and needs no separate
mod adapter. It writes no game files and requires no installed game or
tool-version precheck.

Tools that consume normalized setting records can import
[VoyageModSettings.psm1](VoyageModSettings/VoyageModSettings.psm1) and use
`Read-VoyageModSettingsSchema`, the same implementation as the public validator.

A minimal numeric schema entry is:

```json
{
  "schemaVersion": 2,
  "settings": [
    { "id": "Speed", "type": "number", "default": 1.0,
      "minimum": 0.0, "maximum": 10.0 }
  ]
}
```

For maintained Blueprint consumers, the schema fields mean:

| Field | Meaning |
| --- | --- |
| `id` | Setting identity; also supplies omitted `key` and `runtimeName`. If omitted, the validator derives it from `key`, then `runtimeName`. |
| `key` | Case-sensitive INI key. |
| `runtimeName` | Consumer member/value-pin name; the maintained graph must use the same binding. |
| `type` | `number`, `boolean` or `string`. |
| `default` | Required runtime fallback, expressed as a JSON value of the selected type. |
| `minimum`, `maximum` | Optional inclusive numeric bounds; the schema fallback and distributed value must satisfy them. |
| `storage` | Numeric storage, `double` by default; use `float` only for a float consumer property. |

The distributed INI owns player comments, order, formatting and shipped values.
Its values and the schema's runtime fallbacks have distinct roles.

### Existing user INI

Installation replaces the complete INI with the selected archive's file under
the [release contract](../docs/release-and-installation-rule.md#installation-and-rollback).
Keep a temporary backup for failed-install recovery; do not merge old values or
migrate keys. The [shared installer](release-installation.md#install-a-candidate-with-settings)
owns these mechanics; mod installers supply the exact INI archive entry.
Runtime read policy and schema authoring belong to the
[INI recipe](../docs/runtime-configuration-patterns-knowledge.md).

`Merge-VoyageModIni.ps1` is a local development utility, not the distribution or
candidate-installation path. Its existing interface produces a new file from
`-TemplatePath`, optional `-CurrentPath`, and a fresh `-OutputPath`; optional
`-RetiredKeys`, `-RetiredComments`, and `-DefaultsComment` control that local
transformation. It returns `Created`, `Changed`, `AddedKeys`, and `RemovedKeys`
without writing the installed INI. Do not infer player migration support from
this utility.
