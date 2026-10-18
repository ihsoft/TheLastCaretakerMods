# New mods and runtime settings

## New mod skeleton

Use [`New-ModSkeleton.ps1`](New-ModSkeleton.ps1) after choosing a mod directory
name and one-sentence goal. It creates only `AGENTS.md`, `ARCHITECTURE.md`,
`README.md`, and `version.txt` (`0`) under a new `mods/<Mod>/` directory.
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

## Runtime INI preparation

Use the owning mod's public producer for normal builds. Its small settings
adapter calls [`New-VoyageModSettings.ps1`](New-VoyageModSettings.ps1) with a
canonical default INI, schema, generated header path, and copied INI path.
The tool accepts `-CollectionKind Array` (C++ array) or `TArray`, validates
case-sensitive keys, types, ranges, and defaults, and copies the INI unchanged.
`-GeneratedBy` names the mod adapter in the header comment. It writes no game
files and requires no installed game or tool-version precheck.

For an existing user INI, [`Merge-VoyageModIni.ps1`](Merge-VoyageModIni.ps1)
prepares a **new** file from `-TemplatePath`, `-CurrentPath`, and a fresh
`-OutputPath`. Optional `-RetiredKeys`, `-RetiredComments`, and
`-DefaultsComment` describe mod-owned migrations. Omit `-CurrentPath` to
copy the template for a first installation. Its compact result reports
`Created`, `Changed`, `AddedKeys`, `RemovedKeys`, and `Sha256`. It never writes
the installed INI; the owning installer checks that the game is closed, keeps
a recovery copy, replaces the file, and checks the readback hash. A value or
unit conversion remains mod-specific. See the
[INI recipe](../docs/runtime-configuration-patterns-knowledge.md) for the
runtime read policy and schema authoring contract.
