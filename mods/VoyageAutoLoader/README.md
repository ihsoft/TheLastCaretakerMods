# Voyage Auto Loader

Voyage Auto Loader is an asset-only opt-in loader for already mounted mod
containers. It discovers top-level `Content/Paks/*.autoload` sidecars through
Voyage's Blueprint-callable file APIs and demand-loads one Actor entry class per
descriptor. The shipped payload contains no native DLL, UE4SS code, DML payload
or external runtime preprocessor.

## Provenance and validation scope

The source contract is bound to Steam build `25056839`, game UE `5.8.1`, and
executable SHA-256
`CA84428CF4562C703BEDFF053DB727D14CC70C593451C09BE75A92828EFD9933`.
Revalidate reflected game identities after a fingerprint change. Generated
Content, cooked packages, extracted game data, logs and release evidence remain
ignored.

The user validated a menu/save/menu/repeated-save/normal-exit lifecycle smoke
test with DML's menu available and the incompatible BoatHUD sidecar disabled.
This establishes the tested lifecycle and descriptor path, not strict loader
ordering, late actor counts, arbitrary entry-mod compatibility or hot mounting.

## Runtime architecture

The normal generator emits four packages: the Empty-map bootstrap, menu
GameMode, controller and worker. Canary generation is opt-in through
`-GenerateCanary` and is not part of the production package.

The worker uses
`VoyageEditorBlueprintFunctionLibrary.GetAllFilesInDirectory` to enumerate
sidecars and `LoadFileToArray` to read them. These are existing game functions;
the editor mirrors only compile calls against their real reflected identities.
The exact sidecar filename stem identifies the adjacent `.pak/.utoc/.ucas`
family, and all three container files must exist. Discovery assumes the game
mounted those files during startup; copying a container afterward is not
supported.

For each accepted descriptor the worker applies the phase gate, loads and casts
the Actor class, checks for an existing instance and attempts at most one spawn.
Entries own Pawn/control readiness, widgets and asynchronous initialization.
Menu and gameplay actors are world-local and do not transfer state across
travel. The controller caches its worker and reopens discovery only after that
reference becomes invalid.

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

## Optional DML cooperation

The bootstrap has container priority and can load DML's actual menu widget by
soft class path when DML is present. DML remains optional and is never a parent
dependency. The loader does not redistribute DML assets, edit its saved mod
list, wait for arbitrary DML initialization, or promise a strict activation
order. Existing-actor guards reduce duplicate creation but do not prove
completion of another loader's asynchronous work.

Enabling the BoatHUD autoload sidecar caused overlapping UI and broken
menu/exit behavior; the same lifecycle passed when that sidecar alone was
disabled. This establishes an incompatibility at the loader/UI-lifecycle
boundary, not a specific BoatHUD-internal root cause. Keep that sidecar disabled
until the owning HUD integration is redesigned and separately validated.

## Build contract

There is no supported one-command release producer. Until one is justified by
another requested iteration, the version-bound manual path is:

1. build `VoyageEditor Win64 Development` from the installed UE 5.8.2 editor
   with `-WaitMutex -NoHotReloadFromIDE`;
2. run `UnrealEditor-Cmd.exe <project> -run=GenerateVoyageAutoLoader
   -unattended -nop4 -nullrhi` against a fresh ignored `Content` tree;
3. cook the four production packages with `-SkipZenStore
   -CookSinglePackageNoRefs`;
4. package with canonical retoc using `UE5_8` and fingerprint-matched
   `scriptobjects.bin`;
5. verify the exact package inventory and produce a schema-2 release manifest;
6. install only through the common closed-game, backup and hash-readback gates.

Unreal build, generation and cook must run outside the restricted sandbox.
Source/native modules and editor stand-ins are never shipped. Keep every
generated tree and release artifact under ignored `artifacts/` paths.

## Known limits

- Existing game mounting is assumed; sidecar discovery does not mount files.
- The loader has no production user-facing error channel.
- File-read failure cannot be distinguished from an empty descriptor.
- Strict ordering with another loader and arbitrary asynchronous completion are
  not guaranteed.
- Wider malformed-file runtime tests, late duplicate counts, fresh no-DML
  coverage and a supported release producer remain open.

Reusable discovery findings and rejected Asset Registry assumptions live in
[autoload research](../../docs/voyage-autoload-research.md). The optional DML
integration proposal is [`DML-PROPOSAL.md`](DML-PROPOSAL.md).
