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
- a ZIP, release manifest and verification reports

`Build-Railgun.ps1` is the only supported build producer. It generates model,
construction, input, operator, HUD, projectile and audio assets, cooks them in
one pass, verifies them, and packages one IoStore container.

## Inputs

All mod-owned inputs live below this directory:

- `Assets\Model\Railgun.glb` is the current user-authored model. Replace this
  file to revise geometry or materials without changing the build contract.
- `Assets\Model\model-source.json` maps stable node roles and the two explicit
  interaction/collision boxes. It contains no revision metadata or stored hash.
- `Assets\Railgun_Shot_Blast.wav` is the shot sound.
- `Assets\Railgun.ini` is the default settings template copied beside the mod.
- `Source\` contains editor-only generators and game API mirrors.
- `Config\` and `Voyage.uproject` define the authoring project.

Nothing below `artifacts\` is a build input. That directory may be deleted
between builds. Unreal's generated `Binaries`, `Build`, `Content`,
`DerivedDataCache`, `Intermediate`, and `Saved` directories are also disposable.

## Settings

The installed `Railgun.ini` is read when the player enters the weapon. Existing
user settings are preserved by installation.

```ini
OpticsMousePercent=35
YawLimitDegrees=80
MinimumPitchDegrees=-50
MaximumPitchDegrees=10
ShotVolumePercent=600
```

## Compatibility and validation

The build is fingerprint-gated to the reviewed game and editor versions. Build,
cook and container verification are not gameplay validation. Changes to runtime
behavior require a proportional real-game check before they are treated as a
new validated baseline. See [ARCHITECTURE.md](ARCHITECTURE.md) for the durable
runtime contracts and current hypotheses.
