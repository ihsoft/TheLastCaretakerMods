# New mods and runtime settings

## New mod skeleton

Use [`New-ModSkeleton.ps1`](New-ModSkeleton.ps1) after choosing a mod directory
name and one-sentence goal. It creates only `AGENTS.md`, `ARCHITECTURE.md`,
`README.md`, `CHANGELOG.md`, and `version.txt` (`0`) under a new
`mods/<Mod>/` directory.
It does not inspect the game, select an implementation, build, install, or
overwrite an existing directory. The command emits a compact JSON result with
the new path and files. See the [new-mod workflow](../docs/new-mod-rule.md)
for when to add other files.

```powershell
powershell.exe -NoProfile -ExecutionPolicy Bypass -File tools\New-ModSkeleton.ps1 `
  -Name BlueLargeContainers `
  -Summary 'Make all large containers blue.'
```

`Name` must be 1-64 ASCII letters or digits, starting with a letter; Windows
device names are excluded. `Summary` is one line of 3-240 characters.
The changelog starts with an empty `Next` section; fill it with player-facing
changes before publication and never include it in the Vortex ZIP.

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

The [BPT INI library](BlueprintText/Library/IniFileParser/AGENTS.md) supplies the
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

`id` supplies the key and runtime member name when those fields are omitted.
Use `storage: "float"` only when the consumer property is float. Boolean and
string entries use JSON `true`/`false` and a JSON string as their required
schema fallback. The distributed INI owns player comments, order, formatting,
and shipped values.

### C++ generator consumers

Existing procedural consumers can call
[`New-VoyageModSettings.ps1`](New-VoyageModSettings.ps1) with `-SchemaPath`,
`-DefaultIniPath`, `-HeaderPath`, `-IniPath` and `-GeneratedBy`. It uses the same
validator, copies the INI unchanged and emits the common C++ array table.
`-GeneratedBy` names the adapter in the header comment. Legacy `-CollectionKind`
is accepted for compatibility; it no longer selects a different table layout.

An editor generator adds `tools/VoyageModSettings/Public` to its include paths,
includes `VoyageModSettingsGraph.h`, and includes its generated header inside a
small mod namespace. After creating the Blueprint and before adding graphs that
read settings, it calls:

```cpp
VoyageModSettings::Graph::AddSettingMembers(
    Blueprint, ModSettings::Settings, UE_ARRAY_COUNT(ModSettings::Settings),
    [](const VoyageModSettings::FSetting&) { return true; });
check(VoyageModSettings::Graph::AddSettingsLoader(
    Blueprint, ReloadSettings, ModSettings::RelativePath,
    ModSettings::Settings, UE_ARRAY_COUNT(ModSettings::Settings),
    [](const VoyageModSettings::FSetting&) { return true; }));
```

`AddSettingsLoader` owns the parser/compile/loader/compile ordering. The cooked
Blueprint carries those graphs, so players do not install a shared runtime mod.
The mod keeps its existing lifecycle trigger and application setters.

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
