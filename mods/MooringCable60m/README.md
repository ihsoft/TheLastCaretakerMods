# MooringCable60m

**Version 1.0.0.** Only mooring cables change: manual deployment up to 60 m,
with the original 20 m automatic payout parameter for connected cables.
Game-validated on Steam build 25191271: an existing save loaded, three connected
cables read 2000, a free cable read 6000, and the user physically deployed it
to 59 m. The user accepted the towing behavior and requested this final release.
The final containers are byte-identical to the tested 60m-test5 candidate;
only release packaging and source/documentation cleanup changed.

The selected behavior is 60 m manual deployment and the original 20 m automatic
payout ceiling. The mod preserves the freshly extracted stock mooring class as
`/Game/Blueprints/Cables/BP_ModuleCable_Orig060`, with MaxLengthBase=6000, and
puts a generated child at the original BP_ModuleCable_Mooring identity. Its
BeginPlay starts a 0.05 s timer: two valid cached sockets set MaxLength=2000;
a free end sets MaxLength=6000. The male socket supplies IntegratedCableLength=6000.
Strength, stretch, tightness and snap defaults remain stock. Other cable types
are unchanged. A connected cable's displayed ceiling may therefore show 20 m;
manual deployment must still show and physically allow 60 m.

The original asset/class identity is retained for saves. Existing unusually
long connected cables are not forcibly cut short; the native reeling behavior
continues. The 20 m parameter retains stock payout logic; stretching under load
can produce a larger physical separation. A new-save/reload roundtrip after
installation was not separately reported; loading the existing save was tested.
No UE4SS, native DLL or autoload actor is installed. Editor mirror code is build-only.

Build from Windows PowerShell 5.1, using fresh ignored output directories:

```powershell
# Unreal producer: run outside the restricted sandbox.
powershell.exe -NoProfile -ExecutionPolicy Bypass -File mods/MooringCable60m/Build-LimitGraph.ps1 -OutputRoot R:\Codex\TheLastCaretakerMods\artifacts\mooring60\my-graph
powershell.exe -NoProfile -ExecutionPolicy Bypass -File mods/MooringCable60m/Build-Candidate.ps1 -OutputRoot R:\Codex\TheLastCaretakerMods\artifacts\releases\my-mooring-candidate -GraphManifest R:\Codex\TheLastCaretakerMods\artifacts\mooring60\my-graph\graph-manifest.json
```

The graph producer compiles/generates/cooks the child with tagged properties;
the minimal mirror's inherited property indices must not enter an unversioned
runtime CDO. Its temporary editor parent is never packaged. The candidate
producer independently checks both stock roundtrips, the relocated parent
against stock, child inheritance/events/CDO/absence of active SCS roots, and
exact three-package contents. It emits a schema-2 release manifest and never
installs. Inputs are pinned to executable SHA-256
`747DC2553F7E68D8EA7ED0B2E0CAC6D08943EA3F50DD6ED822E9293E0B45F58B`;
resolve mappings through tools/Get-VoyageMappings.ps1. A fingerprint change
requires fresh stock extraction and contract revalidation.

Install/restore through tools/Install-VoyageRelease.ps1 and
Restore-VoyageReleaseInstallation.ps1, using exact manifests and a closed game.
Final release: `artifacts/releases/MooringCable60m-1.0.0/`.
Its `release-manifest.json` records source revision, payload hashes and game
fingerprint; `promotion-report.json` links the exact accepted test5 payload.
Use the install manifest returned by the common installer for rollback.

Research, detailed evidence and compact tool-use report:
[mooring-cable-length-research.md](../../docs/mooring-cable-length-research.md).
