# Changelog

## Next

- Add lid names and icons to medium and small containers.
- Configure each size independently: defaults are 200 kg for large, 100 kg for
  medium and 50 kg for small containers.
- Use DML without replacing the game's container classes. Only the three
  supported container classes receive changes; other descendants remain unchanged.
- Move the INI to `Voyage/Content/Paks/LogicMods/ContainerLidLabel.ini`.
  The owning installer preserves existing settings and can migrate the old INI.
