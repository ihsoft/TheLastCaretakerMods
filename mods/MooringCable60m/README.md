# MooringCable60m

**Version 1.0.0.** Only mooring cables change: manual deployment up to 60 m,
with the original 20 m automatic payout parameter for connected cables.
Game-validated on Steam build 25191271: an existing save loaded, three connected
cables read 2000, a free cable read 6000, and the user physically deployed it
to 59 m. The user accepted the towing behavior and requested this final release.
The original final containers were byte-identical to the tested 60m-test5
candidate. For Steam build `25956314`, stock contracts and a rebuilt candidate
passed structural validation; gameplay on that build is not yet validated.

Strength, stretch, tightness and snap defaults remain stock. Other cable types
are unchanged. A connected cable's displayed ceiling may show 20 m; manual
deployment of a free end allows 60 m. Package ownership, native length
contracts and the runtime refresh are described in [ARCHITECTURE.md](ARCHITECTURE.md).

The original asset/class identity is retained for saves. Existing unusually
long connected cables are not forcibly cut short; the native reeling behavior
continues. The 20 m parameter retains stock payout logic; stretching under load
can produce a larger physical separation. A new-save/reload roundtrip after
installation was not separately reported; loading the existing save was tested.
No UE4SS, native DLL or autoload actor is installed. Editor mirror code is build-only.

Build from Windows PowerShell 5.1, outside the restricted sandbox:

```powershell
powershell.exe -NoProfile -ExecutionPolicy Bypass -File mods/MooringCable60m/Build-MooringCable60m.ps1
```

The script creates a fresh run under `Tmp/MooringCable60m/`, with intermediate
graph output in `graph/` and the validated release in `release/`. Its result
returns `releaseRoot` and `releaseManifest`; use the latter for installation.
Optional `-OutputRoot` selects a fresh run directory below `Tmp/MooringCable60m`.
`-GameRoot` and `-EngineRoot` select the installed game and UE 5.8 respectively.

The producer compiles/generates/cooks the child with tagged properties;
the minimal mirror's inherited property indices must not enter an unversioned
runtime CDO. Its temporary editor parent is never packaged. It independently
checks both stock roundtrips, the relocated parent
against stock, child inheritance/events/CDO/absence of active SCS roots, and
exact three-package contents. It emits a schema-2 release manifest and never
installs. Inputs are pinned to Steam build `25956314` and executable SHA-256
`0DAC6826ED94666064D06463597487970B8397CA0C124B74764AD37E8705135A`;
resolve mappings through tools/Get-VoyageMappings.ps1. A fingerprint change
requires fresh stock extraction and contract revalidation.

Install/restore through tools/Install-VoyageRelease.ps1 and
Restore-VoyageReleaseInstallation.ps1, using exact manifests and a closed game.
The generated `release-manifest.json` records source revision, payload hashes
and game fingerprint. Use the install manifest returned by the common installer
for rollback.
