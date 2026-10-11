# Railgun

Railgun is a buildable, powered direct-fire weapon for The Last Caretaker. The
repository contains one authoring project and one public build entry point.

## Build

Requirements:

- the reviewed Voyage installation matching the compatibility gate;
- Unreal Engine 5.8.2 at `K:\Epic Games\UE_5.8`;
- the repository-published tools and reviewed mapping selected by the build;
- free cache space on `P:` (the default cache is
  `P:\UnrealCache\TheLastCaretakerMods\UE5.8`).

Run from the repository root:

```powershell
powershell.exe -NoProfile -ExecutionPolicy Bypass -File mods\Railgun\Build-Railgun.ps1
```

Add `-Install` to install the verified result when the game is closed. The
default preparation-only output is a unique temporary directory below
`Tmp\Railgun\`. `-OutputRoot` may select an exact fresh directory only below
that same owner root. The candidate contains loose installation inputs and one
ZIP whose user-facing layout is exactly:

- `Mods\RailgunCatalogue\RailgunCatalogue.uplugin`
- `Content\Paks\Railgun.pak`
- `Content\Paks\Railgun.ucas`
- `Content\Paks\Railgun.utoc`
- `Content\Paks\Railgun.ini`

There are no root files or README in the ZIP. It may be extracted manually
from inside `Voyage\Voyage`. On a successful `-Install`, the exact
artifact-versioned ZIP is copied to the game's Paks directory and becomes the
single retained release copy. The loose payload, release manifest,
`build-provenance.json`, `validation-summary.json`, cook output and logs remain
temporary and the producer removes the complete run only after installation,
settings merge, installed-hash readback and restore-plan validation succeed.
The returned `archivePath` then identifies the retained Paks ZIP;
build-specific temporary report paths are returned as null. Without `-Install`,
the returned candidate paths remain valid below `Tmp\Railgun` until explicitly
discarded.

`Build-Railgun.ps1` is the only supported build producer. It generates model,
construction, input, operator, HUD, projectile and audio assets, cooks them in
one pass, verifies them, and packages one IoStore container. The PAK contains
only Railgun's three-record primary-asset registry at
`Voyage/Mods/RailgunCatalogue/AssetRegistry.bin`; the ZIP and schema-3 release
manifest also own the matching loose content-only descriptor. Registry records
are derived from the three owned data-asset JSON readbacks by the shared
`tools\New-VoyageAssetRegistry.ps1` producer; no stock registry is read, shipped
or replaced.

The generated module shell is loader-free and initializes its owned
operator station from `ReceiveBeginPlay` and persistent post-load lifecycle
events. Both paths schedule one next-tick attempt, and initialization is
independent of whether the shell has an external parent. The owned station is
still attached to the shell's tagged model root and validates its own entry
component relationship. The station reacts to inherited Pawn
possession/unpossession events,
then performs one guarded next-tick continuation for camera and settings work;
the main actor tick does not poll for entry or exit transitions. The station
also observes its concrete shell owner's EndPlay event so dismantling blocks
new acquisition and tears down only after native exit has released possession.
`-Install` fails closed if a legacy `Railgun.autoload` remains in the
game. Updating such an installation requires the reviewed one-time recoverable
migration; do not delete unrelated mods or their autoload sidecars.

The exact cook inventory is written to the per-run TMP workspace. A small
editor-only adapter validates its package count and Core SHA-1 before passing
the complete list directly to Unreal's stock cook commandlet. This keeps the
launcher command line bounded as model dependencies grow while preserving the
single-pass `CookSinglePackageNoRefs` contract. A successful installation
removes the TMP workspace only after the common installer and restore validator
accept it. Installation evidence deliberately records historical source and
release-manifest paths; after TMP cleanup those paths are provenance text, not
a supported reinstall entry point. Operational restore uses the durable
install manifest and `previous-files` evidence.

## Controls

- Mouse aims the weapon; left mouse fires, right mouse toggles the scope, and
  `E` exits the operator station.
- `R` transfers the exact Railgun ammunition from the character who entered
  the station into the gun's native magazine, up to its six-round capacity.
  One press transfers as much as is available and fits; there is no automatic
  reload or reload animation.
- The standard action hint remains present while controlling the station. It
  is disabled when the source inventory or ammunition is unavailable, the
  magazine is full, or the player is outside valid station control. Charge and
  power state do not affect reload availability. The unchanged stock hint
  widget is a serialized direct child of the Railgun HUD canvas inside its
  invalidation root; availability changes do not recreate that outer widget.
  The stock class continues to own the lifecycle of its internal action rows.

## Inputs

All mod-owned inputs live below this directory:

- `Assets\Model\Railgun.glb` is the current user-authored model. Replace this
  file to revise geometry or materials without changing the build contract.
- `Assets\Model\model-source.json` maps stable node roles and explicit
  interaction/collision boxes. It contains no revision metadata or stored hash.
  Its `nodes.chargeIndicatorMesh` role identifies the model-authored render
  mesh used for the gun's local charge display; placement, dimensions and UVs
  remain properties of the GLB.
  Its `nodes.ammoPickupRoot` and `nodes.ammoPickupCarrier` roles also select the
  complete physical-round render subtree and its native pickup carrier. The
  pickup and all six gun slots therefore reuse the same imported meshes,
  materials and textures from this one GLB.
- `Assets\Fabricator\railgun-ammo-item.json`,
  `Assets\Fabricator\railgun-item.json` and
  `Assets\Skill\railgun-skill.json` are the editable authoritative serialized
  sources for the ammo, gun item and research skill.
  `Assets\data-assets-contract.json` binds their package/native identities and
  owns one shared game, mapping, JSON-writer and revalidation gate for the
  complete set. Shared native-class registry defaults and deployment policy live
  with the common registry tool. The contract does not freeze editable gameplay
  or display values.
  The build writes all three JSON files directly as staged packages; it does
  not read, copy or patch stock asset packages.
- `Assets\Railgun_Shot_Blast.wav` is the shot sound.
- `Assets\Railgun.ini` is the distributable template and source of defaults,
  comments, ordering and formatting. `Settings\Railgun.settings.json` adds the
  runtime bindings, types and numeric ranges that INI cannot express. The build
  validates them against each other and generates the runtime header.
- `Build\New-RailgunSettings.ps1` is an internal build step; invoke the public
  `Build-Railgun.ps1` producer rather than running it directly.
- `Source\` contains editor-only generators and game API mirrors.
- `Config\` and `Voyage.uproject` define the authoring project.

A build does not depend on preserved scratch output from an earlier Railgun
run. Published toolchain binaries and reviewed reusable caches selected through
their documented interfaces are separate dependencies. The producer writes
disposable logs, loose cook output and readbacks below repository `Tmp/`,
removes its owned successful-install scratch, and retains one validated,
self-contained artifact-versioned ZIP in the game's Paks directory. Common
installation backup/restore evidence remains below `artifacts/installations`.
Do not delete an existing artifact until its release, cache, research or
rollback value and dependencies are understood. Unreal's
generated `Binaries`, `Content`, `Intermediate`, and `Saved` directories are
producer workspaces that may be recreated when no run is active.
`DerivedDataCache` is a reusable performance cache: clear it only for an
intentional cache reset, not as part of general temporary cleanup. The tracked
scripts under `Build\` are source inputs and must not be deleted as generated
output.

## Settings

The installed `Railgun.ini` is read when the player enters the weapon. Existing
user settings are preserved by installation. A missing key uses its template
default at runtime and is added to the installed file by the next installation.
The installer also retains the existing one-time migration from the former
`FullChargeEnergyKJ` key.

```ini
OpticsMousePercent=35
YawLimitDegrees=80
MinimumPitchDegrees=-30
MaximumPitchDegrees=10
HitDamage=350
FullChargeEnergyKWh=0.85
FullChargeTimeSeconds=5.5
ShotVolumePercent=600
IdleConsumptionKW=1
OfflineDischargeKW=50
CameraRecoilStrength=1
ShipRecoilStrength=1
```

The HUD binds once to its owning Railgun station during widget construction and
takes an initial charge, ammunition, style and view-mode snapshot. Later charge
and ammunition changes refresh their displays through the existing station and
module event paths; settings reload and zoom changes refresh style and
visibility directly. The radial fraction and numeric value show the station's
cached charge without display interpolation. `ShotVolumePercent` is the
pre-category shot multiplier: `600` preserves the established six-times source
baseline and `0` mutes it. The imported wave uses Voyage's stock `SC_SFX`, whose
parent is `SC_Master`, so the game's saved/startup settings and live Master/SFX
changes remain in the native path. The shot-volume setting and live Master/SFX
response are confirmed in the real game; restoration of saved audio settings
during a new game startup was not separately verified. HUD label layout,
opacity, font, typeface and size are fixed presentation contracts rather than
user settings.

The widget and station retain transient direct references to each other. A
reconstructed widget unregisters its previous station first, and destruction
clears the station's reference only when it still names that exact widget.
Missing or unexpected ownership fails closed without retry, timer or world
discovery. Per-frame HUD graph work is limited to live range presentation.
Status classification is event-driven from the station's cached charge, socket
and power state, and the charging icon uses an authored looping UMG animation;
the gameplay HUD has no diagnostic status text and does not poll the module.

Electricity storage is configured in the same `KWh` unit shown by the game.
Voyage maps one displayed `KWh` to 1000 native electricity amount units; the
generator converts that amount to the module's W demand for the configured
charge time, plus `IdleConsumptionKW` converted to W. The idle setting defaults
to `1` and accepts zero and fractional kW values. Like other numeric settings,
nonnumeric input retains the template fallback and out-of-range input is
clamped to the declared range.

Station initialization caches the concrete module, binds its exact value,
socket-connection and power-state delegates, and takes initial charge and
supply snapshots. Value changes refresh stored charge and change custom
consumption only when the required charging/idle mode changes. For the
supported game fingerprint, the socket notification may precede freshness of
`HasSocketConnection`; an accepted event therefore schedules one coalesced
next-tick snapshot for the same still-bound module. Power events consume their
exact `bHasPower` payload immediately.
A re-entry settings read forces one demand refresh so changed charge-time,
capacity, idle-consumption or offline-discharge values take effect without
waiting for another resource event. Synchronous notifications raised by the native demand setter
are coalesced into one final read; the desired mode is cached before the setter
is called. Rebinding settles and stops the old module's drain timer, invalidates
pending supply reconciliation, removes old delegates and clears stale state
before resolving a replacement module. EndPlay performs the same timer and
binding teardown.

The station actor starts with Tick disabled and enables it only while the local
station view is owned; Actor Tick owns optics and range work only. Offline
discharge uses a separate looping timer that exists only while cached supply is
missing, stored energy is positive and `OfflineDischargeKW` is positive. It
runs at approximately 3 Hz, never polls socket or power, and integrates elapsed
game time rather than assuming a fixed callback interval. Stop, rate change,
rebind and teardown settle the last partial interval before clearing the timer;
idle, supplied, empty and zero-rate stations own no drain timer.

Actual station entry is driven by `ReceivePossessed`; it reloads the installed
INI and refreshes camera and energy state after one next-tick continuation.
Every completed entry restores the ordinary wide camera, baseline FOV and
normal wide-view input multiplier before refreshing the HUD mode.
`ReceiveUnpossessed` performs the symmetric guarded camera restoration. These
events use pending-state coalescing and have no periodic lifecycle discovery,
retry timer or polling fallback.

The model charge indicator uses the GLB mesh bound by
`nodes.chargeIndicatorMesh`. At runtime, only that tagged component loads the
stock `/Game/Materials/Modules/MI_PogressBar_Basic_LED` material and creates
its own dynamic material instance. Initialization binds a separate visual observer to the module's
`OnModuleValueChanged` event and reads the initial Electricity balance. Each
event invokes the visual refresh, which reads that module directly and writes `ProgressLevel` only when the
clamped `0..1` ratio changes, using the existing required-shot denominator.
Binding is idempotent, post-load initialization rebinds, and EndPlay unbinds.
The callback only updates the display; tick and post-shot observers are removed.
This model display does not add another charging accumulator, timer,
interaction or collision path. Display-specific save/load and multiplayer
behavior remain outside the established compatibility coverage.

The two recoil settings are independent multipliers. With no INI, a missing
recoil key, or an invalid numeric value, the runtime fallback is `0`, which
disables that effect. Explicit valid INI values override the fallback; `1`
selects the authored baseline. The historical
`CameraRecoilStrength` key now applies a persistent two-degree random aim
deflection at baseline strength; it does not shake the camera or return by
itself. Aim limits may reduce the actual displacement near their boundaries.
Ship recoil applies a backward impulse only when the module's attachment chain
reaches a simulated physical body. Recoil is emitted only after a successful
shot has consumed both charge and ammunition.

## Compatibility and validation

The build is fingerprint-gated to the reviewed game and editor versions. Build,
cook and container verification are not gameplay validation. Changes to runtime
behavior require a proportional real-game check before they are treated as a
new validated baseline. See [ARCHITECTURE.md](ARCHITECTURE.md) for the durable
runtime contracts and current hypotheses.
