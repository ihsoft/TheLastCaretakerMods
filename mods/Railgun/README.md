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

Current releases are loader-free: the built module shell initializes its owned
operator station from `ReceiveBeginPlay` and persistent post-load lifecycle
events. `-Install` fails closed if a legacy `Railgun.autoload` remains in the
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

## Inputs

All mod-owned inputs live below this directory:

- `Assets\Model\Railgun.glb` is the current user-authored model. Replace this
  file to revise geometry or materials without changing the build contract.
- `Assets\Model\model-source.json` maps stable node roles and explicit
  interaction/collision boxes. It contains no revision metadata or stored hash.
  Its `nodes.chargeIndicatorMesh` role identifies the model-authored render
  mesh used for the gun's local charge display; placement, dimensions and UVs
  remain properties of the GLB.
- `Assets\Fabricator\RailgunAmmoCassette.glb` is the user-authored physical
  single-round pickup model. Geometry and materials may be replaced while the
  import still produces the stable owned mesh identity, nonempty render
  geometry, finite nondegenerate bounds, simple pickup collision, and all
  material and texture dependencies used by the current source.
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
ShotVolumePercent=600
StatusIconOpacityPercent=50
ChargeIndicatorSmoothingSpeed=4
ChargeTextOpacityPercent=100
ChargeTextFontSize=14
ChargeTextFontPath=/Engine/EngineFonts/Roboto.Roboto
ChargeTextTypeface=Regular
HitDamage=350
FullChargeEnergyKWh=0.85
FullChargeTimeSeconds=5.5
OfflineDischargeKW=10
CameraRecoilStrength=1
ShipRecoilStrength=1
```

The wide-view charge gauge is evaluated every rendered widget frame. Its
`ChargeIndicatorSmoothingSpeed` only interpolates the displayed value between
the game's discrete energy samples; it does not change charging or firing.
`0` disables interpolation. Charge text font, typeface, size and opacity are
configured independently by the corresponding `ChargeText*` keys.

Electricity storage is configured in the same `KWh` unit shown by the game.
Voyage maps one displayed `KWh` to 1000 native electricity amount units; the
generator converts that amount to the module's W demand for the configured
charge time, plus the 1 kW idle load.

The model charge indicator uses the GLB mesh bound by
`nodes.chargeIndicatorMesh`. At runtime, only that tagged component loads the
stock `/Game/Materials/Modules/MI_PogressBar_Basic_LED` material and creates
its own dynamic material instance. Initialization binds to the module's
`OnModuleValueChanged` event and reads the initial Electricity balance. Each
event reads that module directly and writes `ProgressLevel` only when the
clamped `0..1` ratio changes, using the existing required-shot denominator.
Binding is idempotent, post-load initialization rebinds, and EndPlay unbinds.
The callback only updates the display; tick and post-shot observers are removed.
This model display does not add another charging accumulator, timer,
interaction or collision path. Its appearance and charge-linked operation are
user game-validated with event-driven updates on `build-20261005-053611`.
Save/load and multiplayer behavior were not separately confirmed for this display.

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
