# Procedural settings generator compatibility

Read this interface only when maintaining an existing producer that still
uses generated C++ settings tables and graphs. New authoring uses maintained
Blueprints through BPT and the [INI preparation interface](../mod-setup.md#runtime-ini-preparation).
Consumer lifecycle and migration constraints stay with the owning mod; this
guide does not define an alternative development workflow.

Existing procedural consumers can call
[`New-VoyageModSettings.ps1`](../New-VoyageModSettings.ps1) with `-SchemaPath`,
`-DefaultIniPath`, `-HeaderPath`, `-IniPath` and `-GeneratedBy`. It uses the schema
validator described in the preparation guide, copies the INI unchanged and
emits the common C++ array table.
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
