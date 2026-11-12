# ContainerLidLabel configuration

Runtime file: `Voyage/Content/Paks/LogicMods/ContainerLidLabel.ini`.
The [authored INI](Assets/ContainerLidLabel.ini) is distributed with the package;
the [settings schema](Settings/ContainerLidLabel.settings.json) validates it.

```ini
LargeContainerMaxWeightKg=200
MediumContainerMaxWeightKg=100
SmallContainerMaxWeightKg=50
```

These are independent mass limits in kilograms. Missing or invalid values fall
back to 200, 100 and 50 respectively. Values must be finite float32 numbers at
least 1 kg; overflow and nonzero underflow to zero are invalid, not clamped.

The flat Blueprint parser trims whitespace, accepts `#` comments outside quoted
strings and matches keys case-sensitively. A semicolon is not a comment. The
last occurrence of a key wins; an invalid final occurrence uses that key's
fallback. See the shared [parser contract](../../tools/BlueprintText/Library/IniFileParser/AGENTS.md)
for the complete grammar.

The single DML ModActor reads the file once at initialization and stores the
three effective values in its own fields. Containers reuse that snapshot.
Restart the game after editing the INI. Changing capacity does not remove
stored items, rebuild the inventory, or change the number of item slots.

[Install-ContainerLidLabel.ps1](Install-ContainerLidLabel.ps1) merges the new
INI template with the player's existing file, preserving active values and
comments and adding missing defaults. If the LogicMods INI is absent, it can
migrate `Voyage/Content/Paks/ContainerLidLabel.ini`. A legacy `MaxWeightKg` key
is renamed to `LargeContainerMaxWeightKg` only when the new key is absent.
The old key is not a runtime alias. Migration, readback and recovery belong
to the owning installer; the build does not modify installed files.
