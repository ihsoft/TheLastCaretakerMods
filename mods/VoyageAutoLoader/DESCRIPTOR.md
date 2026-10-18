# Voyage Auto Loader sidecar format

## Descriptor contract

An empty sidecar selects gameplay activation and the conventional class:

```text
/Game/Mods/<container-stem>/ModActor.ModActor_C
```

Two independently optional case-sensitive fields override those defaults:

```text
entryClass: /Game/Mods/MyMod/ModActor.ModActor_C
activateIn: menu,gameplay
```

- `activateIn` accepts `menu`, `gameplay`, or both comma-separated in either
  order.
- Surrounding whitespace, blank lines and whole-line `#` comments are accepted.
- Unknown or duplicate keys, duplicate phases, empty explicit values, missing
  colons and unsupported phases reject the descriptor. Invalid nonempty input
  never silently falls back to a default entry.
- JSON, a version field, quoted values, escapes and inline comments are not
  supported.
- Parsing is bounded to 4096 characters and 64 lines after native read/join.
  The native file read itself has no success flag or pre-allocation bound, so
  an empty file and a failed read cannot be distinguished at this layer.
- Physical container identity and virtual class identity are independent. A
  `_P` container may point to an unsuffixed package through `entryClass`.
- Main menu and gameplay are positive phase classifications. A generic
  non-playing or transitional world is not automatically the menu.

The descriptor parser has 24 Blueprint-VM cases covering defaults, independent
optional fields, both phases and invalid input. Broader malformed-file runtime
coverage remains open.
