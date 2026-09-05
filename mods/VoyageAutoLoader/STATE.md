# Voyage Auto Loader: working checkpoint

Date: 2026-09-05. Status: experimental, locally game-tested C8; not a release.

## What works

An asset-only loader discovers opt-in mods without knowing their filenames in
advance. It uses Voyage's existing Blueprint-callable
`VoyageEditorBlueprintFunctionLibrary.GetAllFilesInDirectory` to enumerate
top-level `Content/Paks/*.autoload`, then `LoadFileToArray` to read each sidecar.
These are existing game functions, not an injected native plugin. The editor
mirrors only let the generator compile calls against their real identities.
No UE4SS, player-side script runner, native DLL or Asset Registry discovery is
required at runtime.

The exact filename stem identifies the `.pak/.utoc/.ucas` family. All three
files must exist. An empty descriptor defaults to gameplay activation of
`/Game/Mods/<stem>/ModActor.ModActor_C`. Each field below is independently optional:

```text
entryClass: /Game/Mods/MyMod/ModActor.ModActor_C
activateIn: menu,gameplay
```

Keys and phases are case-sensitive. Whitespace, blank lines and whole-line `#`
comments are supported. Invalid/unknown/duplicate fields reject the descriptor.
Physical container names and virtual entry paths need not match when entryClass
is supplied. For example, a `_P` filename can use a class in an unsuffixed folder.

The worker applies phase gates, loads/casts an Actor class, checks for existing
instances and attempts at most one spawn per candidate. Entries own their Pawn,
control, widget and asynchronous initialization readiness. Menu and gameplay
actors are world-local; their state does not automatically transfer on travel.
The controller caches its worker to avoid a full actor search on every Tick.

## DML cooperation

Our Empty-map bootstrap has container priority. If DML is available, we load its
actual widget by soft class path and preserve its menu console. DML remains
optional: no DML payload redistribution, inheritance dependency or saved-list
mutation. Autoload does not wait for DML. Existing-actor guards reduce duplicates;
this is not a guarantee of strict ordering or arbitrary asynchronous completion.
Integrating sidecar discovery directly into DML could avoid a competing bootstrap.

## Evidence and limits

- Game: Steam build 25056839, UE5.8.1. Executable SHA256:
  `CA84428CF4562C703BEDFF053DB727D14CC70C593451C09BE75A92828EFD9933`.
- Build editor: UE5.8.2. C8 generator/parser: 24/24 Blueprint-VM descriptor tests;
  successful build/cook and exact four-package verification.
- C7 tests demonstrated discovery of two distinct sidecars, custom entry paths,
  independently optional fields and menu/gameplay selection.
- C8, with HUD autoload disabled: user confirmed the requested menu/save/menu/
  repeated-save/normal-exit test, including DML menu check. This is lifecycle
  smoke coverage, not proof of late actor counts.
- HUD-enabled runs had overlapping UI and broken menu/exit behavior on both C7
  and C8. C7 and C8 passed without that sidecar. HUD activation or its interaction
  with UI lifecycle is implicated; the exact defect is not established.
- C8 has no diagnostic SaveGame writes or Asset Registry probes. The separate
  test canary still has its own marker and is not shipped by normal generation.
- Existing game mounting is assumed. Copying containers after startup and hot
  reload are not supported. Discovery does not prove a package is mounted.
- Native file read has no success flag; empty/read-failure cannot be distinguished.
  The 4096-character/64-line bounds apply after reading, not to native allocation.
- Wider malformed-file runtime tests, late duplicate counts, fresh no-DML C8
  coverage, user-facing errors and a supported release producer remain follow-ups.

The source checkpoint includes the generator, editor mirrors, descriptor tests
and required shared identities. Cooked assets, game extracts, logs, backups and
release binaries remain ignored. No native module is part of the player payload.
Detailed experiment evidence and rollback identities are in README.md.
