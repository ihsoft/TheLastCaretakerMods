# MooringCable60m

**Version 1.0.0.** Only mooring cables change: manual deployment up to 60 m,
with the original 20 m automatic payout parameter for connected cables.
The supported game build and asset contracts live in
[ARCHITECTURE.md](ARCHITECTURE.md). Revalidate them for a later game build.

Strength, stretch, tightness and snap defaults remain stock. Other cable types
are unchanged. A connected cable's displayed ceiling may show 20 m; manual
deployment of a free end allows 60 m. Package ownership, native length
contracts and the runtime refresh are described in [ARCHITECTURE.md](ARCHITECTURE.md).

The original asset/class identity is retained for saves. Existing unusually
long connected cables are not forcibly cut short; the native reeling behavior
continues. The 20 m parameter retains stock payout logic; stretching under load
can produce a larger physical separation. Existing saves resolve the
replacement cable class; new-save/reload behavior has no separate claim.
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
On a new game build, the default invocation stops at its pinned-version gate.
Retry with `-IgnorePinnedGameVersion`:

```powershell
powershell.exe -NoProfile -ExecutionPolicy Bypass -File mods/MooringCable60m/Build-MooringCable60m.ps1 -IgnorePinnedGameVersion
```

That flag skips only the pinned Steam build ID comparison. The script
still requires the installed game's exact reviewed mappings,
extracts fresh stock assets, checks their roundtrips and expected fields,
compares the packaged overrides with stock, and validates the release manifest.
The mapping resolver and release manifest record the Steam build ID for
game provenance.
If the build passes, test the candidate in game. A successful runtime test
completes compatibility validation; investigate reflected classes or native
behavior only if the build or game test exposes a problem. A missing current
mapping remains a separate prerequisite and is not bypassed by this flag.

The producer compiles/generates/cooks the child with tagged properties;
the minimal mirror's inherited property indices must not enter an unversioned
runtime CDO. Its temporary editor parent is never packaged. It independently
checks both stock roundtrips, the relocated parent
against stock, child inheritance/events/CDO/absence of active SCS roots, and
exact three-package contents. It emits a schema-2 release manifest and never
installs. A successful build establishes a structurally validated candidate,
not gameplay behavior. The producer performs the routine stock-asset checks;
manual asset analysis is reserved for a failed gate or runtime discrepancy.

Install/restore through tools/Install-VoyageRelease.ps1 and
Restore-VoyageReleaseInstallation.ps1, using exact manifests and a closed game.
The generated `release-manifest.json` records source revision, payload hashes
and game fingerprint. Use the install manifest returned by the common installer
for rollback.
