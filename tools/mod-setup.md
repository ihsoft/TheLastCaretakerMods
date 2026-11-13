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
powershell.exe -NoProfile -ExecutionPolicy Bypass -File tools\New-ModSkeleton.ps1 `
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
Use Windows PowerShell 5.1 for these entry points. Reserve the
[Unreal queue](unreal-coordination.md) around the whole build or editor test.

| Function | Interface and result |
| --- | --- |
| `New-VoyageModRun` | `-ModRoot`, optional `-OutputRoot` and `-Kind`; creates and returns a fresh run directory below `Tmp/<Mod>/`. |
| `New-VoyageModProjectCopy` | `-ModRoot -Destination`; copies maintained inputs and returns the `.uproject` path without launching Unreal. |
| `Remove-VoyageModScratch` | `-ModRoot -RunRoot -Path`; removes only an unlinked descendant of that mod's run, preserving the run root and its evidence. |
| `Invoke-VoyageModEditorTest` | `-ModRoot -EngineRoot`, optional `-OutputRoot`, `-ReportName`, `-ExpectedSuiteCount`, and a required `-Run` script block; prepares a copy, runs the supplied checks, requires a passing report and removes the project only on success. Returns `status`, `reportPath`, `evidence`, `bptWorkspace`. |
| `Invoke-VoyageModCook` | `-ModRoot -ProjectFile -EngineRoot -OutputRoot -Packages -ParserVersion`, optional `-AdditionalCookArguments`; cooks an explicit additive `/Game/` inventory with unversioned properties, stages its asset/bulk files, converts the container and verifies its exact package set. Returns `container`, `stage`, `validation`. |

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

The cook helper is for the ordinary additive cook profile, not arbitrary
serialization adapters. A mod with a native readback/conversion contract keeps
that focused stage and its assertions. Archive creation and manifest production
continue through the [release tools](release-build.md); the shared helper does
not install or publish. Follow the [output lifecycle](../docs/artifact-lifecycle-rule.md)
for successful-run scratch and retained reports.

For changes to these mechanics, run
[`Test-VoyageModPipeline.ps1`](Test-VoyageModPipeline.ps1) in Windows PowerShell
5.1. It uses isolated fixtures below `Tmp/tests/`, including junction rejection,
test failure evidence and candidate-based INI installation/recovery. It launches
no Unreal process and writes no installed game files. Then run the affected
mods' public tests/builds for changes at those boundaries.

## Runtime INI preparation

Use the owning mod's public producer for normal builds. For maintained BPT
graphs, import [VoyageModSettings.psm1](VoyageModSettings/VoyageModSettings.psm1)
and call its exported `Read-VoyageModSettingsSchema`. It validates
case-sensitive keys, number/boolean/quoted-string values, schema fallbacks,
optional inclusive ranges, final storage, and duplicate-free repository INI.
It returns normalized setting records and writes no files. Validation errors
throw; only after success should the producer copy the canonical INI unchanged
to its output directory, distinct from the input paths:

```powershell
$ErrorActionPreference = 'Stop'
Import-Module ./tools/VoyageModSettings/VoyageModSettings.psm1 -Force
$null = Read-VoyageModSettingsSchema `
    -SchemaPath './mods/MyMod/Settings/MyMod.settings.json' `
    -DefaultIniPath './mods/MyMod/Assets/MyMod.ini'
$preparedDirectory = './Tmp/MyMod/settings'
$null = New-Item -ItemType Directory -Path $preparedDirectory -Force
Copy-Item -LiteralPath './mods/MyMod/Assets/MyMod.ini' `
    -Destination (Join-Path $preparedDirectory 'MyMod.ini')
```

The [BPT INI library](BlueprintText/Library/IniFileParser/integration.md) supplies the
runtime parser; the consuming graphs own getters, fallbacks and application of
values. This preparation path generates no header or graph and needs no separate
mod adapter. It writes no game files and requires no installed game or
tool-version precheck.

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

For an existing user INI, [`Merge-VoyageModIni.ps1`](Merge-VoyageModIni.ps1)
prepares a **new** file from `-TemplatePath`, `-CurrentPath`, and a fresh
`-OutputPath`. Optional `-RetiredKeys`, `-RetiredComments`, and
`-DefaultsComment` describe mod-owned migrations. Omit `-CurrentPath` to
copy the template for a first installation. Its compact result reports
`Created`, `Changed`, `AddedKeys`, and `RemovedKeys`. It never writes
the installed INI; the owning installer checks that the game is closed, keeps
a recovery copy, replaces the file, and checks the readback hash. A value or
unit conversion remains mod-specific. See the
[INI recipe](../docs/runtime-configuration-patterns-knowledge.md) for the
runtime read policy and schema authoring contract.
