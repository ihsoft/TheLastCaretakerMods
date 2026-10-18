# VoyageAutoLoader architecture

This asset-only bootstrap discovers `.autoload` sidecars beside already mounted
mod containers. Its generated menu GameMode and controller own a worker that
reads sidecars through Voyage's Blueprint-callable file functions, selects the
requested menu or gameplay phase, checks for an existing entry actor, and
attempts one spawn. The entry mod owns its later readiness and initialization.
The bootstrap ships without a native DLL, UE4SS, or DML payload.

[Descriptor contract](DESCRIPTOR.md) describes sidecar fields and runtime limits.
[Mod rules](MOD-RULES.md) retain the loader's specific constraints.
[DML-PROPOSAL.md](DML-PROPOSAL.md) is an external proposal, not a
runtime dependency.

The public producer builds `VoyageEditor`, generates the loader, and cooks
the four production packages. The generated stock menu GameMode mirror is an
editor-only parent and does not enter the release container.

## Compatibility scope

The last pinned Steam build is in `game-build.json`; its source contract was
recorded for game UE `5.8.1`. Revalidate reflected game identities if a build
or game test exposes a mismatch. Generated
Content, cooked packages, extracted game data, logs and release evidence remain
ignored.

The loader supports the menu/save/menu/repeated-save/normal-exit lifecycle with
DML's menu available. Strict loader ordering, late actor counts, arbitrary
entry-mod compatibility and hot mounting are outside its established scope.

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
## Known limits

- Existing game mounting is assumed; sidecar discovery does not mount files.
- The loader has no production user-facing error channel.
- File-read failure cannot be distinguished from an empty descriptor.
- Strict ordering with another loader and arbitrary asynchronous completion are
  not guaranteed.
- Wider malformed-file runtime tests, late duplicate counts, fresh no-DML
  coverage and a supported release producer remain open.

The optional DML integration proposal is
[`DML-PROPOSAL.md`](DML-PROPOSAL.md).
