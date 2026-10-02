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
output is written to a new `artifacts\railgun\build-*` directory and contains:

- `payload\Railgun_P.utoc`
- `payload\Railgun_P.ucas`
- `payload\Railgun_P.pak`
- `payload\Railgun_P.autoload`
- `payload\Railgun.ini`
- `payload\Mods\RailgunCatalogue\RailgunCatalogue.uplugin`
- a ZIP, release manifest and verification reports

`Build-Railgun.ps1` is the only supported build producer. It generates model,
construction, input, operator, HUD, projectile and audio assets, cooks them in
one pass, verifies them, and packages one IoStore container. The PAK contains
only Railgun's three-record primary-asset registry at
`Voyage/Mods/RailgunCatalogue/AssetRegistry.bin`; the ZIP and schema-3 release
manifest also own the matching loose content-only descriptor. Registry records
are derived from the three owned data-asset JSON readbacks and the compact
registry policy in `Assets\data-assets-contract.json`; no stock registry is
read, shipped or replaced.

## Inputs

All mod-owned inputs live below this directory:

- `Assets\Model\Railgun.glb` is the current user-authored model. Replace this
  file to revise geometry or materials without changing the build contract.
- `Assets\Model\model-source.json` maps stable node roles and explicit
  interaction/collision boxes. It contains no revision metadata or stored hash.
- `Assets\Fabricator\RailgunAmmoCassette.glb` is the user-authored physical
  single-round pickup model. Its sibling `ammo-cassette-source.json` binds the
  current source revision to the import readback contract.
- `Assets\Fabricator\railgun-ammo-item.json`,
  `Assets\Fabricator\railgun-item.json` and
  `Assets\Skill\railgun-skill.json` are the editable authoritative serialized
  sources for the ammo, gun item and research skill.
  `Assets\data-assets-contract.json` binds their package/native identities and
  owns one shared game, mapping, writer, registry-policy and revalidation gate
  for the complete set. It does not freeze editable gameplay or display values.
  The build writes all three JSON files directly as staged packages; it does
  not read, copy or patch stock asset packages.
- `Assets\Railgun_Shot_Blast.wav` is the shot sound.
- `Assets\Railgun.ini` is the distributable template and source of defaults,
  comments, ordering and formatting. `Settings\Railgun.settings.json` adds the
  runtime bindings, types and numeric ranges that INI cannot express. The build
  validates them against each other and generates the runtime header.
- `Build\New-RailgunSettings.ps1` is an internal build step; invoke the public
  `Build-Railgun.ps1` producer rather than running it directly.
- `Build\New-RailgunRegistryMetadata.ps1` derives compact registry metadata
  from the three staged package readbacks and the shared contract. Explicitly
  serialized values always win, class defaults apply only when a property is
  absent, and the contract owns deployment policy such as registry format and
  chunk IDs without duplicating item balance or recipe data.
- `Build\Test-New-RailgunRegistryMetadata.ps1` runs fixture-only metadata tests
  without launching or modifying the game:

  ```powershell
  powershell.exe -NoProfile -ExecutionPolicy Bypass `
    -File mods\Railgun\Build\Test-New-RailgunRegistryMetadata.ps1
  ```

- `Source\` contains editor-only generators and game API mirrors.
- `Config\` and `Voyage.uproject` define the authoring project.

Nothing below `artifacts\` is a build input. Artifacts may be deleted between
builds. Unreal's generated `Binaries`, `Content`, `DerivedDataCache`,
`Intermediate`, and `Saved` directories are also disposable. The tracked
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

## Compatibility and validation

The build is fingerprint-gated to the reviewed game and editor versions. Build,
cook and container verification are not gameplay validation. Changes to runtime
behavior require a proportional real-game check before they are treated as a
new validated baseline. See [ARCHITECTURE.md](ARCHITECTURE.md) for the durable
runtime contracts and current hypotheses.
