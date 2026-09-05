# Voyage Auto Loader candidate

Start with [the working checkpoint](STATE.md) for current behavior, evidence and
limits. The experiment chronology below preserves earlier, superseded decisions.

Status: C8 without HUD autoload passed the user's lifecycle retest (2026-09-05).
C7 remains the recoverable game-tested predecessor. C7 both-phase and BoatHUD entry-only
tests proved discovery, phase exclusion and custom-path activation; BoatHUD UI
failed separately. C8 error-isolation game tests and wider compatibility remain.
C6 descriptor-read gate passed in game. JSON was dropped at the
user's request in favor of a simple two-field text format. C5 runtime
observed our loader create the canary first. C4 menu compatibility
and existing-actor guard were observed at runtime. Shared-entry reverse order,
late counts and repeat-travel coverage remain incomplete.
C1 no-DML candidate passed one
menu-to-gameplay runtime transition and is now preserved outside Paks.
The independent test14 proves discovery, class load, actor BeginPlay
and a sequential same-world duplicate guard. DML cooperation is not implemented
in C1; C2 deliberately retains the DML-present entry-activation stop while
testing the higher-priority bootstrap and real widget handoff.

## Requested protocol

Current normal producer emits only Empty, menu GameMode, controller and worker.
Canary generation is explicitly opt-in with -GenerateCanary; it is not part of
normal generation/cook/package. Existing C1 canary payload remains unchanged.

- Discover top-level `Content/Paks/*.autoload` sidecars at runtime.
- Removing only `.autoload` identifies the matching `.pak/.utoc/.ucas` family.
- An empty sidecar is sufficient for opt-in. Do not require a JSON parser for
  the minimum protocol.
- Default entry is an Actor Blueprint under `/Game/Mods/<mod-id>/ModActor`,
  with generated class path ending `/ModActor.ModActor_C`.
- Container stem and virtual mod-id are separate identities. C1 uses the exact
  stem as the default mod-id; a future descriptor can override entryClass.
  Do not heuristically strip prefixes, version numbers or patch priorities.
- C7 implements independent optional entry/phase overrides in key:value text.
  An invalid nonempty descriptor must not silently select a default actor.
- Loading a Blueprint loads its required hard dependencies; it neither executes
  arbitrary Blueprint classes nor eagerly loads every soft dependency. A single
  entry actor can own the mod's initialization; multiple entries are not needed
  for the first protocol.
- Detection does not mount containers copied after the game's initial mounting
  phase. An opt-in sidecar is not proof of mounted/loadable package identity.

## Descriptor contract (implemented since C7)

User approved phase selection through the existing physical .autoload sidecar:

```text
entryClass: /Game/Mods/MyMod/ModActor.ModActor_C
activateIn: menu,gameplay
```

- Every field is independently optional. entryClass alone keeps gameplay;
  activateIn alone keeps the conventional class. Neither means both defaults.
- activateIn accepts menu, gameplay, or both comma-separated in either order.
- Trim surrounding whitespace from lines/keys/values/phase items. Blank lines
  and whole-line # comments are ignored. Keys/phase values are case-sensitive.
- Unknown/duplicate keys, duplicate phases, empty explicit values, missing
  colon and unsupported phases reject the whole descriptor. No JSON, version
  field, quoted values, inline comments or escape syntax. Add future fields
  deliberately rather than silently ignoring a misspelled activation setting.
- Parsing is limited to 4096 characters and 64 lines after native read/join;
  LoadFileToArray itself remains unbounded and has no success flag. An empty
  read cannot distinguish an empty sidecar from a failed read. This inherited
  reader limitation is not solved by the parser's post-read limits.
- Empty sidecars retain the conventional entry class and gameplay-only default.
- entryClass overrides virtual identity without renaming physical containers
  or changing their assets, including the previously discussed _P migration.
- Both phases mean separate world-scoped actor instances, not automatic state
  transfer. Retain the existing-actor guard within each appropriate world.
- Pawn/player-control readiness is the entry mod's responsibility. C7 removes
  the Pawn gate; controller permits exact Empty menu or IsPlaying gameplay.
- DML is untouched: these settings govern only our loader, not DML activation.
- Rejecting invalid nonempty descriptors must not silently fall back to an
  unintended default actor or phase. Parser bounds and precise validation
  behavior are covered by 24 Blueprint-VM tests; broader malformed-file runtime
  coverage remains pending. JSON is not supported.
- World classification must distinguish the main menu, gameplay and transitional
  worlds; !IsPlaying alone is not proof of menu identity. Validate this before
  enabling menu entry spawning. Do not remove lifecycle gates indiscriminately.

This replaces the previous universal gameplay-only design because the user
wants each mod to declare its activation phase. Current installation is
C8 with HUD autoload disabled. The native read gate passed. The initial unbuilt JSON
automaton draft was removed when the user selected key:value text; no such
parser was shipped or retained. C7 tests cover each optional field separately,
empty/menu/gameplay/both and invalid input without changing DML's saved list.
Documentation-only tool report: current rules/protocol inspected and updated;
no asset operation, build, install, game launch or save edit. Coverage N/A.

## Optional DML cooperation

Current user decision: preserve DML menu availability and run gameplay autoload
independently. DML is optional compatibility, not a parent dependency. No
inheritance implementation was made. Actor-existence guards replace reliance
on strict activation order. No fixed delay is used as a DML completion signal.

Current order: our Empty/bootstrap override wins and initializes optional DML
in the menu. Gameplay autoload is not gated on DML and does not promise to run
before it. Earlier C1-C3 notes below are historical experiments. Container
mounting precedence and runtime activation order remain separate contracts.

Local UE5.8.2 source check (IPlatformFilePak.cpp, FPakPlatformFile::Mount and
GetPakOrderFromPakFilePath): ordinary `_P.pak` adds 100 to base order; `_1_P.pak`
adds 200. This order is passed to both the IoStore file backend and package
store. The reviewed DML files use plain `_P`; a future dedicated loader stem
VoyageAutoLoader_1_P is therefore a stronger candidate than relying on `zzz`
tie ordering. This is local engine-source evidence, not yet validation of
Voyage's UE5.8.1 runtime with both installed. Do not merely rename the installed
C1 files; package, verify and install the next candidate with its own manifest,
remove the competing C1 footprint recoverably, and prove bootstrap ownership
with its unique marker while DML is present. C1 remains unchanged.

Preserve the independent implementation. Do not distribute DML packages or
reconstructed DML implementation in our candidate. Use the separately installed
DML through its real package identities, only when available. No native DLL,
UE4SS, runtime external script or player-side preprocessing is introduced.

Reviewed community fix: DML v0.5 UE5.8fix, linked by its maintainer in the
[Nexus pinned post](https://www.nexusmods.com/thelastcaretaker/mods/77?tab=posts).
Downloaded ZIP SHA256:
`A94B0CFBCECF3CB4855E4E47B8B57C0C23DE928C54C409BAF0DB95FEB7D262A2`.
Download and uncooked inspection outputs remain under ignored artifacts; the
mod was installed with C2; runtime compatibility is not independently confirmed.

Its `dml_core_P` supplies `/Game/Maps/Empty` with an always-loaded streaming
sublevel `/Game/DmgModLoader/DMG`. The latter's level-script Tick creates the
`/Game/DmgModLoader/WBP_DML` widget when none exists. Therefore loading the
original overridden Empty path from our override would not delegate to DML;
it resolves the winning override again. The bridge must use DML's unique
sublevel/widget identity instead, preserving the stock menu contract.

WBP_DML Init distinguishes the `Empty` main menu and skips the mod-loading
branch there. Outside the menu it loads its save, checks AutoLoad, waits 0.2s
and calls LoadMods for its stored class map. It also contains a separate
asynchronous LoadNextMod/LoadAssetClass chain. Thus streaming completion or
return from Init is not a universal "all DML logic finished" event. Ordering
must be verified for each lifecycle path; arbitrary Delay15 is not a proof.
The stored-class LoadMods route has a GetAllActorsOfClass guard, but the separate
asynchronous path must not be assumed to have the same guard.

Do not modify the user's DML save/enable list to implement our opt-in protocol.
Do not silently bypass a DML initialization failure and claim ordered startup.
Unknown DML versions and incomplete installations need visible diagnostic status.

## Accepted lifecycle and next discriminating test

User chose gameplay-world-only activation. Menu initialization must never
create entry actors. C1 gates its worker with VoyageGameInstance.IsPlaying
and a valid player pawn, then retains the tested asynchronous discovery delay.
A noninteractive viewport controller attempts to follow travel by recreating
itself on Destruct; this new lifecycle path requires runtime validation.
WorldName is included in diagnostic markers. One worker per world and the
two sequential entry-instance guards are independent protections.

C1 is only the no-DML lifecycle candidate, not the complete requested loader.
If its optional soft-class lookup finds WBP_DML, it records
VoyageAutoLoader_C1_DML_ORDER_PENDING and does not activate entry actors.
It neither initializes DML nor implements the DML-first completion adapter
yet. This fail-closed intermediate behavior must not be marketed as DML
coexistence. Test without DML first; retain the downloaded DML for the next
isolated adapter test. No JSON support is claimed.

C1 naming is exact: Example.autoload identifies Example.pak/.utoc/.ucas and
/Game/Mods/Example/ModActor.ModActor_C. No suffix/prefix is removed except
.autoload. All three container files must exist. Contents are ignored in C1.

User's migration case confirms the future descriptor requirement: a container
DonkLiftKeyboardControl_P.pak may have an entry under a different virtual mod-id.
Keep DonkLiftKeyboardControl_P.autoload bound to its physical container family
and allow a future `entryClass` override to name the actual generated class.
Neither container renaming nor editing existing assets should be required.
This example does not establish that the current autonomous DonkLift actually
contains a DML-style ModActor. JSON schema/reader remain future work after C1.

After resolving that contract, first test optional DML bootstrap with explicit
markers for absent/present/streamed/initialized stages; do not label it complete
mod-order compatibility. Then test a real level transition, a mod listed in both
loaders, two distinct opt-in mods, no marker, and invalid/missing entry. Require
single instances per intended world, no menu/control regression, and no DML
save mutation. Preserve test14 rollback until its replacement passes these gates.

## Current game API evidence

Installed Steam build 25056839 / executable SHA256
CA84428CF4562C703BEDFF053DB727D14CC70C593451C09BE75A92828EFD9933 was rechecked
before reusing the reviewed native reflection snapshot. Existing filename
enumeration is game-validated. The same owner exposes BlueprintCallable static
`TArray<FString> LoadFileToArray(FString InPath)` under
`/Script/Voyage.VoyageEditorBlueprintFunctionLibrary`. Signature existence is
confirmed, and C6 read a known multiline ASCII fixture successfully in game.
General encoding, failure semantics and native input bounds remain unverified.
The current descriptor format is key:value, not JSON.

Investigation evidence: artifacts/dml-ue58-review; fingerprint output
artifacts/fingerprints/autoload-candidate.json. Returned inspector JSON paths:
artifacts/asset-inspections/20260905T030602670Z-a172ec89/export/Voyage_Content_Maps_Empty_umap.json;
artifacts/asset-inspections/20260905T030618030Z-b74f7957/export/Voyage_Content_DmgModLoader_DMG_umap.json;
artifacts/asset-inspections/20260905T030636238Z-015a444e/export/Voyage_Content_DmgModLoader_WBP_DML_uasset.json.
The isolated WBP inspection does not mount dml_core2; unresolved BFL references
are not evidence of broken DML. Use a documented multi-container dependency
inspection before making claims about those calls.

Tool report: public Get-VoyageBuildFingerprint and Get-VoyageAssetJson inventory
and exact isolated Mod inspections passed (3/3 recurring package operation
families). Source-only design and bounded existing-reflection diagnosis are
outside that denominator. No tool internals were inspected. No build, cook,
install, game launch, game-file mutation or commit. No new reusable tool gap;
descriptor parsing and runtime DML coordination are unresolved mod behavior.

## C1 build and installation evidence (superseded installation)

Generated sources are independent under this mod, based on the owned test14
generator; no DML payload was copied. The editor-only WorldSettings and
GameInstance mirrors are bound to the fingerprint above. Stock menu GameMode
is an identity stand-in used only during generation, excluded from the release.
Source/native modules are never shipped; the loader payload contains exactly
five cooked packages and the canary contains exactly two.

Source build: Engine/Build/BatchFiles/Build.bat VoyageEditor Win64 Development
with this mod's Voyage.uproject, -WaitMutex and -NoHotReloadFromIDE. Generation:
UnrealEditor-Cmd.exe <project> -run=GenerateVoyageAutoLoader -unattended -nop4
-nullrhi. Use a fresh generated Content tree; move any previous generated tree
to a checked path under ignored artifacts before regenerating. All UE/.NET
calls run outside the restricted sandbox. Cook only the seven release packages
using -run=cook -targetplatform=Windows -unversioned -SkipZenStore
-CookSinglePackageNoRefs. Exact package lists and command logs are retained
under artifacts/autoload-candidate/c1; no broad stock extraction/cook is needed.
Packaging uses canonical retoc to-zen --version UE5_8 on isolated loader/canary
staging trees with the fingerprint-matched scriptobjects.bin from test14.

Final build-release.log, generate-release.log and cook-release.log passed;
generation/cook each report zero errors/warnings. Earlier regeneration over
existing Content failed; preserving it and generating fresh resolved that
failure. The new WorldName diagnostics initially used unconnected impure
GetCurrentLevelName nodes; their exec wiring was fixed before final packaging.
An intermediate source patch matched the wrong helper and was corrected before
the next build. Neither failure reached the installed game; no .NET crash.

Final integrity/exact-set reports:

- artifacts/container-checks/303b60df46d9440aa95b957004479626/verification.json (loader 5)
- artifacts/container-checks/27e1c02263614c57b9ed89b09ed3458d/verification.json (canary 2)

Final isolated inspection runs 20260905T033728994Z-87c70640,
20260905T033731201Z-acf78fc5, 20260905T033733265Z-c01cca47 and
20260905T033735295Z-16634df7 confirmed IsPlaying/player-pawn gates,
HitTestInvisible controller, bootstrap widget guard, three physical FileExists
checks, *.autoload discovery, independent BeginPlay/world-name marker and no
concrete AutoLoaderCanary identity embedded in the loader graph.

Both schema-2 manifests validated and installed through Windows PowerShell 5.1.
The candidate temporarily reuses zzz_VoyageRegistryProbeLoader_P to replace
exactly the previous bootstrap footprint, not install a competing Empty override.
Its release ZIP has the distinct VoyageAutoLoader_25056839-c1 identity. Restore
the following loader installation to recover exact test14; remove the following
canary installation through the same common restoration tool:

- artifacts/installations/VoyageAutoLoader/20260905-033825-25056839-c1-fe8e39d7/install-manifest.json
- artifacts/installations/AutoLoaderCanary/20260905-033824-25056839-c1-7672c6af/install-manifest.json

AutoLoaderCanary.autoload is exactly zero bytes and is manifest-owned. The
previous independent RegistryProbe container and historical ZIPs remain untouched
as experiment continuity; clean them up through their installation evidence after
the C1 replacement lifecycle is understood. No C1 markers existed at install.

Expected manual run: wait 30 seconds in the normal menu, load a save, wait
30 seconds after gaining control, exit. Inspect CONTROL_STARTED/CONTROL_DESTRUCT,
STARTED and DONE, then ACTOR_BEGINPLAY; record WorldName and actor counts.
Expected discovered candidate AutoLoaderCanary.autoload, default entry
/Game/Mods/AutoLoaderCanary/ModActor.ModActor_C, LoadResults=[true],
BeforeActorCounts=[0,1], SpawnResults=[true], FinalActorCounts=[1]. Gameplay
world must not be Empty. If menu/control/UI breaks, restore test14 before
changing the lifecycle again. Do not install DML for this first run.

Tool report: Get-VoyageBuildFingerprint, Test-VoyageContainer,
Get-VoyageAssetJson, Install-VoyageRelease and Get-VoyageInstallationStatus
passed; final installed hashes/fingerprints match, game closed. Public coverage
5/9 recurring asset/release operation families (56%); source build/generation,
cook and packaging still use the recorded experimental route. Existing gaps
remain deferred, not a mandate for new generic tooling. Tool development is
separate: PS5.1 legacy and empty-sidecar install/restore/rollback regressions
passed, including mismatched-sidecar rejection (latest sidecar evidence under
artifacts/tests/install-voyage-release/83c2285edbeb4703abb65b8de8003947).
Installer/restorer source inspection was required because the old documented
contract supported only triplet plus ZIP. No gameplay validation, game launch,
DML installation, player-save modification or commit was performed.

## C1 runtime result: gameplay entry succeeds

User run recorded CONTROL_STARTED and CONTROL_DESTRUCT at 2026-09-05
03:45:13 UTC with WorldName=Empty. Worker STARTED was written at 03:45:21;
DONE and the independent ACTOR_BEGINPLAY marker at 03:45:36 both identify
VoyageWorld2. CandidateFiles=[AutoLoaderCanary.autoload], derived entry is
/Game/Mods/AutoLoaderCanary/ModActor.ModActor_C, LoadResults=[true],
BeforeActorCounts=[0,1], SpawnResults=[true], FinalActorCounts=[1].
RejectedFiles is not serialized (default-empty); the actor marker identifies
the independent /Game/Mods/AutoLoaderCanary/BP_ActivationMarker class.

This validates delivery of the controller's work into the gameplay world,
empty-sidecar discovery, demand class loading, actor BeginPlay and the
sequential duplicate guard in this run. The fixed save slots can overwrite
earlier events; they do not prove an exhaustive absence of earlier menu
activation or exactly one controller over every travel. Repeated travel,
multiple mods, user-visible UI/control behavior and DML ordering require their
own checks. JSON/entryClass parsing remains unimplemented.

Exact evidence: artifacts/autoload-candidate/c1/runtime-result.json. Public
Get-VoyageInstallationStatus confirmed both installed file sets/fingerprints
match; game process still observed. No installed mutation or shutdown was
attempted. Only C1 diagnostic saves were read, using the existing bounded
PowerShell/zlib fallback; headers, totals, EOF and array strings were checked,
WorldName StrProperty bytes inspected, and all marker hashes were stable
before/after read. Readback report: 1/2 operation families public; existing
marker-decoder gap, no new tool-source inspection, .NET call, build or launch.

## C2 active test: own bootstrap before real DML

User authorized replacement and compatible DML installation. C2 keeps the
accepted Empty/menu/pawn/IsPlaying contracts and changes one architectural
behavior: own controller Construct records C2_CONTROL_STARTED, soft-loads
/Game/DmgModLoader/WBP_DML.WBP_DML_C, casts to UserWidget, queries existing
top-level widgets of that class, and creates/adds the real widget only if none
exists. This is the target that DML's own DMG level-script Tick creates. No
DML assets or hard DML imports are included in our payload, and no call into
our overridden Empty path is used as delegation. Both widgets retain their
own Destruct recreation behavior; duplicate-free travel remains a test gate.

Unique diagnostic slots (prefix VoyageAutoLoader_C2_): CONTROL_STARTED,
CONTROL_DESTRUCT, DML_CREATED, DML_EXISTING, DML_UNAVAILABLE and
DML_CREATE_FAILED. Creation is not initialization completion. Gameplay worker
still emits STARTED, then DML_ORDER_PENDING and stops if DML is loadable;
DONE/canary activation is deliberately NOT expected with DML present. An
unavailable widget is not a certified absent or supported DML version.
The no-DML worker route remains the C1 discovery protocol. The canary is the
unchanged C1 installation and its old diagnostic files are not fresh C2 proof.

The new physical family VoyageAutoLoader_1_P is intended to outrank the three
DML plain _P families. The four exact C1 files (triplet and its provenance ZIP)
were backed up, hash-checked and removed from Paks, without restoring the older
test14 Empty override. Retirement evidence and exact recoverable files:
artifacts/autoload-candidate/c2/retire-c1-plan.json,
retire-c1-result.json and retired-c1/. Old independent registry-probe files
and historical probe ZIPs remain untouched; no unrelated mod was modified.

All nine DML container files were compared to entries in the reviewed original
ZIP and installed byte-for-byte unchanged. To use the common single-family
installer, three local-only release manifests/ZIPs were prepared, with explicit
upstream archive SHA and a sourceCommitScope identifying the local installation
recipe, NOT claiming that our Git commit built DML. They are installed directly
in Content/Paks (not the archive's dml subdirectory). Virtual identities remain
unchanged; this placement and runtime coexistence require the manual gate.
No DML source/payload is redistributed in our mod, and no DML save or enable-list
was edited. All output remains ignored artifacts.

Current installation manifests:

- artifacts/installations/VoyageAutoLoader/20260905-040739-25056839-c2-68f901b1/install-manifest.json
- artifacts/installations/dml_P/20260905-040756-25056839-v0.5-ue58fix-ce413809/install-manifest.json
- artifacts/installations/dml_core2_P/20260905-040758-25056839-v0.5-ue58fix-0ae34368/install-manifest.json
- artifacts/installations/dml_core_P/20260905-040759-25056839-v0.5-ue58fix-458a7ff0/install-manifest.json

Final installed-readback.json under artifacts/autoload-candidate/c2 confirms
all four installations plus the unchanged C1 canary match their manifests and
Steam 25056839/executable fingerprint. Game was closed in all five snapshots.
No C2 marker existed before the test. No game launch or commit was performed.

Manual test: stay in menu 30 seconds, load a save, wait 30 seconds with control,
then exit. Check menu, camera and HUD behavior; do not change DML mod settings
for this test. Own C2_CONTROL_STARTED must exist with DML installed, followed
by DML_CREATED or DML_EXISTING, and worker STARTED/DML_ORDER_PENDING.
C2 has a diagnostic gap: these early worker checkpoints do not yet assign
WorldName (see runtime result below). Inspect DML's visible behavior separately; marker creation
alone does not prove that its later Tick/Init succeeded. Fixed slots can
overwrite earlier world events. Stop and recover C1 if menu/control regresses.

Recovery: while closed, use the common restoration tool on the four new
installation manifests (they remove newly added files), then hash-check and
copy back only the four exact retired-c1 files to their recorded source paths.
Do NOT restore the old C1 installation manifest: that would restore test14,
not C1. Do not touch the independent canary or player saves.

Tool-use report, C2:

- Fingerprint, container integrity, asset inspection, install and readback:
  Get-VoyageBuildFingerprint, Test-VoyageContainer, Get-VoyageAssetJson,
  Install-VoyageRelease, Get-VoyageInstallationStatus passed. Four install
  manifests validated, all installed hashes matched; PS5.1 -File installs.
- Build, fresh Content generation and five-package cook succeeded; generation
  and cook each 0 errors/0 warnings. All UE/.NET calls outside sandbox; no
  dotnet crash. Published retoc packaged only the five owned loader packages.
  Exact-set verification: container-checks/f726ec5e9ee1441ca56c45816fc32199.
  DML integrity: 3a82a07f124d46ff9568314467aa31c1 (9 packages),
  6d0a2d7a144a4fc1ae5f802d996b7e07 (1), 45d69158f5e34b1f8ad7ab7fc75df032 (1).
- Final controller JSON returned by inspection run
  20260905T040714413Z-48703010 confirms soft class path, C2 stage markers,
  widget query, IsPlaying/pawn gates and HitTestInvisible; no canary identity.
- Invocation errors only: unsupported -Asset argument and relative ModContainer
  resolved against game Paks. Corrected using documented positional identity
  and absolute container path; no tool internals or game-store files inspected.
- C1 retirement used a bounded exact-file script because common restore would
  reinstate an obsolete predecessor. It validates paths/hashes, copies all
  backups before removals, rechecks closed game at each mutation, and provides
  handled-error recovery. PS5.1 -File completed; four backups verified.
- Conservative coverage: 5/10 operation families public (50%); experimental
  build/generation/cook/package and exact old-name retirement are uncovered.
  Existing producer gaps plus retirement gap recorded in toolchain backlog;
  no generic tool extension authorized just to improve coverage.
- Runtime precedence, DML initialization and completion ordering remain pending.

## C2 runtime readback

User reported test done. All four C2/DML install manifests and the current game
fingerprint matched. Game process was still observed; no mutation or shutdown.
Own C2 markers did not exist before installation's requested test.

- 2026-09-05 04:11:49 UTC: DML_CREATED, WorldName=Empty.
- 04:12:46: CONTROL_DESTRUCT, CONTROL_STARTED and DML_EXISTING, all Empty.
  The Construct slot was overwritten during recreation; its current timestamp
  must not be used to claim that DML preceded our initial controller.
- 04:12:54: STARTED; 04:13:09: DML_ORDER_PENDING, as designed. No C2 DONE,
  DML_UNAVAILABLE or DML_CREATE_FAILED marker was present at readback.

This demonstrates our bootstrap is live while DML is installed, our optional
handoff creates the real DML widget, and the observed recreation path detects
an existing widget. Worker reached the intentional DML stop after its delay.
Source gates worker creation on IsPlaying and valid player pawn, but its early
checkpoints lack WorldName: assignment is currently below the DML branch,
only on the no-DML path. This is a diagnostic placement defect, not evidence
that the worker ran in an empty/incorrect world. Move assignment before STARTED
in the next authorized candidate; do not modify the running installation.

No claim yet that DML Init/Tick or its autoload work completed, that the
existing widget is unique over every transition, or that menu/camera/HUD are
unchanged. Ask the user for visible behavior separately. Keep DML-present
autoload activation fail-closed until the completion contract is addressed.

Evidence: artifacts/autoload-candidate/c2/runtime-result.json contains exact
marker hashes, times, decoded worlds and installation checks. Bounded zlib
headers, segment sums, EOF, FString type/length/terminator and own report-class
identity checked; all six marker hashes stable before/after read.
Tool report: installation status through Get-VoyageInstallationStatus passed;
existing PowerShell/zlib marker decoder fallback used (1/2 families public).
One inline PowerShell foreach-pipeline syntax error corrected before status
readback; no tool-source inspection, new reusable gap, build, game launch,
installed-file mutation or player-save write. Documentation/evidence only.

## Post-C2 confirmation and cleanup

User confirmed dml help/list responses and the old mod list from DML's save.
This establishes console command handling and saved-list readback, not that
every listed actor was activated or that all initialization completed.

User authorized removing unnecessary probes. With Voyage closed, 16 exact
manifest/hash-owned RegistryProbe files were removed from Content/Paks:
the old zz_VoyageRegistryProbeMod_1_P triplet and 13 historical probe ZIPs.
All were copied and hash-verified before removal; per-file closed-game and
source-hash gates passed. Recovery files and plan/result are under
artifacts/autoload-candidate/cleanup-old-probes/. Current C2, DML, HUD,
DonkLift and the still-required AutoLoaderCanary were untouched. Research
source/history remains in the repository; this cleanup concerns installation.

No subclass, candidate build or replacement installation was made. Review of
the existing DML bytecode confirms startup Init calls the synchronous LoadMods
route after its latent delay, while LoadNextMod is a distinct async route;
this fact alone does not provide an external completion notification.
Tool report: public fingerprint/status passed; exact-file retirement reused
the documented bounded fallback (PS5.1 -File) because predecessor restoration
would reinstate old probes. Backups/readback passed, no failure, game launch,
tool-source change or new reusable gap; existing retirement gap remains.

## C3 active candidate: first-world loader priority

User chose reversing entry order to keep DML optional. C3 retains the higher
priority VoyageAutoLoader_1_P footprint and the accepted gameplay/pawn gates.
Controller Construct now only records its marker; it does not create DML.
Worker records WorldName before STARTED, runs physical *.autoload discovery
and its existing guarded spawn loop, writes DONE, then optionally loads and
creates the real WBP_DML. No subclass, parent mirror, native runtime module,
DML payload or hard class dependency is introduced. The 15-second worker
Delay and DML_ORDER_PENDING stop were removed. The controller still polls
IsPlaying/pawn readiness once per second; this is not a DML completion delay
and does not establish a before-scene-loading lifecycle guarantee.

First gameplay world after process start is the discriminating scope: our
entry pass must complete before DML_CREATED. With no sidecars, the empty pass
must still continue to optional DML. Missing DML must not prevent own entries.
DML console can be unavailable in the initial menu until the gameplay pass.
Existing DML widget detection is recorded rather than assumed to prove priority.

Important remaining limitation: DML recreates its base widget independently
on later travel. C3 does NOT guarantee our first execution on those transitions.
Our GetAllActorsOfClass guard also skips actors created earlier by DML; DML's
reviewed startup LoadMods guard skips ours. Runtime duplicate freedom for the
same opted-in mod remains untested, especially manual/async LoadNextMod paths.
No claim that dependencies' asynchronous initialization finishes on BeginPlay
return. This is a first-world ordering candidate, not complete interop.

Installed artifact 25056839-c3 (candidate-c3), Steam 25056839 and unchanged
executable SHA from the provenance above. Install manifest:
artifacts/installations/VoyageAutoLoader/20260905-043711-25056839-c3-c7659823/install-manifest.json.
The common installer preserved exact C2 predecessors; common restoration of
this manifest recovers C2. DML, canary and other mod files were untouched.
Current readback in artifacts/autoload-candidate/c3/installed-readback.json:
files/fingerprint match, game closed, no C3 marker existed before testing.

Test: fresh game process, load a save, wait 5-10 seconds after control appears,
run dml help and dml list, then exit. Expect C3_STARTED/DONE with gameplay
WorldName and C3_DML_CREATED after DONE, CandidateFiles=[AutoLoaderCanary.autoload],
LoadResults=[true], BeforeActorCounts=[0,1], SpawnResults=[true], final count 1.
The unchanged independent canary still uses C1_ACTOR_BEGINPLAY; require a fresh
timestamp for this run, never accept the old C1 file as new activation evidence.
Then test repeat travel and a genuinely shared opt-in entry in a separate gate;
do not alter the user's DML save/list implicitly for that test.

Tool-use report: fingerprint, exact-set/integrity validation, Mod inspection,
manifest validation/install and readback passed. Build/generate/cook were
outside sandbox; generation and cook reported 0 errors/0 warnings, no .NET
crash. Five-package retoc verification f579a86432594c119b3b654475d6b3fb passed.
Returned worker/controller JSON runs 20260905T043617641Z-837417f3 and
20260905T043619990Z-cd5f7cff confirmed no worker Delay or DML stop, no controller
DML path, no concrete canary identity, DONE preceding the optional DML lookup
on that execution branch, and WorldName assignment before STARTED. PS5.1
-File install validated and read back matching hashes. No tool-source
inspection, new gap, failure, game launch or gameplay claim. Conservative
coverage 5/9 families public; experimental build/generate/cook/package remain
the previously recorded producer gaps. No commit.

## C4 active candidate: menu compatibility, independent gameplay pass

User accepted restoring DML in the menu and relinquishing strict runtime
activation order. C4 moves the unchanged soft-class/widget creation branch
back into controller Construct, with existing-widget guard. Worker contains
no DML path, wait, completion dependency or stop. Physical sidecar discovery,
guarded actor spawning and the corrected WorldName-before-STARTED assignment
remain. Controller's one-second gameplay/pawn polling is unchanged; it is
neither a DML-completion timer nor proof of before-scene activation.

No subclass, DML payload, native runtime dependency or player-save/list edit.
Higher container/bootstrap priority remains. Startup LoadMods has a reviewed
existing-actor guard; runtime shared-entry testing and independent asynchronous
LoadNextMod/manual-command behavior remain pending. Do not claim global
duplicate freedom or ordered completion from this candidate.

Build/generation/cook passed outside sandbox; generation and cook each report
0 errors/0 warnings. Five-package integrity/exact-set gate passed:
artifacts/container-checks/3f34e5813c4d495eab238ffcd02e4f2d/verification.json.
Returned worker/controller JSON runs 20260905T044850187Z-455a5b6d and
20260905T044852362Z-0c602ff5 verified: no worker Delay/DML path/stop or concrete
canary identity; expected C4 markers, sidecar filter, actor guard, soft DML
widget creation in controller, IsPlaying/pawn gates and HitTestInvisible.

PS5.1 ValidateOnly and installation passed; artifact 25056839-c4, candidate-c4,
Steam 25056839 and unchanged executable hash. Installation manifest:
artifacts/installations/VoyageAutoLoader/20260905-044920-25056839-c4-fc8833ad/install-manifest.json.
Common restoration recovers the exact C3 triplet. No other mod was changed.
Final artifacts/autoload-candidate/c4/installed-readback.json confirms matching
files/fingerprint and game closed. No C4 marker existed before testing.

Manual test: fresh process, run dml help in the main menu, load a save, wait
5-10 seconds after control, verify dml help again, then exit. Inspect C4 menu
DML_CREATED/EXISTING and gameplay STARTED/DONE, discovered canary and actor
counts. Independent canary remains C1; its BeginPlay marker needs a fresh
timestamp. Repeated travel and shared-entry loading remain additional tests.

Tool report: public fingerprint/status, container verification, exact Mod
inspection, manifest validation/install/readback all passed; experimental
build/generate/cook/package route reused. Conservative coverage 5/9 families
public, existing producer gaps unchanged. No failures, .NET crash, tool-source
inspection, new reusable gap, game launch or commit; runtime result pending.
Rule changed because user explicitly rejected C3 menu-console loss as a
compatibility regression and accepted non-strict activation order instead.

## C4 shared-canary runtime result

User first confirmed C4 works as intended, including restored menu DML. He
then executed dml add AutoLoaderCanary in the menu; screenshot confirmed DML
success and a saved list containing DonkLiftKeyboardControl and AutoLoaderCanary.
This save/list change was made by the user, not by the agent. After the requested
save load/test, readback found game closed and C4, all three DML families and
the canary matching installation manifests and the current fingerprint.

2026-09-05 UTC: DML_CREATED at 05:11:45 in Empty; fresh independent canary
BeginPlay at 05:13:12.076 in VoyageWorld2; C4_STARTED at 05:13:15.722 and DONE
at 05:13:15.927 in VoyageWorld2. CandidateFiles=[AutoLoaderCanary.autoload],
AttemptPaths=[/Game/Mods/AutoLoaderCanary/ModActor.ModActor_C], LoadResults=[true],
BeforeActorCounts=[1,1], SpawnResults=[], FinalActorCounts=[1], RejectedFiles=[].
Absent empty properties are interpreted using their known default-empty schema.

This validates our already-existing-actor guard in the shared-entry run: our
loader found one actor and did not spawn another. The earlier fresh BeginPlay
is consistent with DML activating it first; marker does not identify its caller.
It does not prove the reverse order's DML guard at runtime, a late/end-of-session
count, all manual async commands, or repeated travel. Do not require another
identical run as proof of those distinct cases; strengthen the count marker if
late-duplicate validation is pursued.

Evidence: artifacts/autoload-candidate/c4/runtime-shared-canary.json. Public
Get-VoyageInstallationStatus readbacks passed; bounded PowerShell/zlib fallback
decoded only our diagnostic saves with header/size/sum/EOF, FString type/bounds,
array count/terminator checks and stable before/after hashes. 1/2 operation
families public; existing decoder gap, no failures or tool-source inspection.
No game launch, build, install, deletion, player-save modification or commit.

User-requested unchanged restart, 2026-09-05 05:19 UTC: canary BeginPlay
05:19:30.113, C4_STARTED 05:19:33.394, DONE 05:19:33.614; all VoyageWorld2.
Again BeforeActorCounts=[1,1], SpawnResults=[], FinalActorCounts=[1], load true,
same discovered sidecar/class. This repeats the existing-actor outcome, not
the reverse-order test. Earlier BeginPlay is consistent with DML-first; no
late/end-session count is claimed. Evidence:
artifacts/autoload-candidate/c4/runtime-shared-canary-repeat.json.
Game closed; public installation status confirms C4 hashes/fingerprint match.
Bounded read-only zlib/FString fallback and stable hashes passed, no failures,
mutation or launch; same 1/2 public operation families and existing decoder gap.

## C5 active experiment: remove one-second polling delay

User requested isolating the cadence hypothesis after two DML-first C4 runs.
Only behavioral change: controller Tick connects directly to the existing
GameInstance cast / IsPlaying / valid Pawn gates, without Delay(1). Existing
per-world worker actor guard and DML menu compatibility remain unchanged.
Diagnostic prefix becomes VoyageAutoLoader_C5_; canary stays unchanged at C1.
No guarantee of earlier scene readiness or beating DML: gating/lifecycle and
tick scheduling are independent of the removed interval. Per-frame guard
queries now continue after startup as well; this is a bounded experiment, not
a production performance signoff. Do not mix in readiness/lifecycle changes
when interpreting this run.

Build, fresh generation, five-package cook passed outside sandbox; generation
and cook 0 errors/0 warnings. Container exact-set/integrity gate:
artifacts/container-checks/862d009c385949b1aeb44f30d6660f92/verification.json.
Returned controller JSON run 20260905T052746265Z-9a663220 verified no Delay,
preserved IsPlaying/pawn/worker guard, widget guard, soft DML path and invisible
controller. PS5.1 -File ValidateOnly/install passed. Current installation:
artifacts/installations/VoyageAutoLoader/20260905-052814-25056839-c5-13950d0b/install-manifest.json.
Common restoration recovers exact C4. Artifact 25056839-c5, candidate-c5,
Steam 25056839 and same reviewed executable hash; installed-readback.json under
artifacts/autoload-candidate/c5 confirms matching hashes/fingerprint and game
closed. No C5 markers existed before testing. Other mods/DML save list untouched.

Test: fresh process, load same save with both opt-ins unchanged, wait 5-10s
after control and exit. Compare fresh canary BeginPlay against C5_STARTED/DONE
and BeforeActorCounts/SpawnResults/FinalActorCounts. [1,1]/[] means our guard
again found an existing actor; [0,1]/[true] means our loader spawned one, not
proof that no late duplicate exists. Compare timing cautiously across runs.

Tool report: fingerprint/status, container verification, Mod inspection,
manifest validation/install/readback passed. Experimental producer route
build/generation/cook/packaging reused, 5/9 operation families public. No failure,
.NET crash, tool-source inspection, new gap, game launch or commit. Existing
producer gaps remain deferred; gameplay/performance verification pending.

### C5 runtime: own loader spawned the canary

2026-09-05 UTC, user test complete: STARTED 05:30:57.029914, canary BeginPlay
05:30:57.068906, DONE 05:30:57.236964; all VoyageWorld2. CandidateFiles contains
AutoLoaderCanary.autoload, expected generated class resolves, load true,
BeforeActorCounts=[0,1], SpawnResults=[true], FinalActorCounts=[1], rejected empty.
This time our loader created the actor, unlike the two C4 existing-actor runs.
IsPlaying and Pawn checks remain unchanged; they did not prevent this outcome.
One uncontrolled run does not quantify the isolated timing effect, guarantee
priority, prove DML's later completion, or provide a late/end-session count.
No further gate removal was implemented. The user's earlier question about
whether both conditions are necessary remains a design question, not evidence
that either is required for arbitrary entry mods.

Evidence: artifacts/autoload-candidate/c5/runtime-result.json. Game closed;
public installation-status readback matched C5 hashes and game fingerprint.
Bounded read-only zlib/FString fallback decoded our three markers, checked
segment totals/EOF and string/array bounds, and verified stable hashes.
Tool report: 1/2 operation families public; existing decoder gap, no errors,
source-tool inspection, build, install, launch or player-save modification.

## C6 active gate: descriptor file content, not yet parsing

User authorized descriptor implementation. Before combining file access,
parsing and activation-phase changes, this candidate isolates the unvalidated
native reader. Current installed fingerprint was rechecked against provenance:
Steam 25056839 and executable CA84428CF4562C703BEDFF053DB727D14CC70C593451C09BE75A92828EFD9933.
Bounded identity searches in the existing matching native reflection snapshot
confirmed /Script/Voyage.VoyageEditorBlueprintFunctionLibrary:LoadFileToArray,
static public BlueprintCallable, FString InPath -> TArray<FString>. Its exact
editor-only mirror was added; no native module is shipped.

No general Blueprint JSON parser was found in that snapshot. JsonUtilities
JsonObjectWrapper exists, but that alone is not a callable parsing API.
Curve-table JSON conversion is not a descriptor parser. This search does not
prove that no other module or route can expose parsing; do not claim complete
JSON support or implement a naive split-based parser as equivalent to JSON.

C6 keeps C5 cadence, IsPlaying, Pawn gate, gameplay-only default entry and DML
menu compatibility unchanged. It reads each discovered sidecar via full path,
records DescriptorFiles, DescriptorLineCounts, DescriptorCharCounts and
DescriptorText (lines joined with newline). Diagnostic text capture is capped
at 4096 characters AFTER read/join; this is not a bounded file-read guarantee.
The native reader has no returned success flag, so empty result alone cannot
distinguish empty file from failure. Error and resource-limit semantics remain
an implementation gate. C6 is a controlled reader probe, not a production
descriptor interpreter: content is still ignored for activation, including
unknown/invalid JSON. Do not treat that temporary behavior as the approved
future fail-closed descriptor contract.

Only existing AutoLoaderCanary was used. Its C1 triplet is byte-for-byte
unchanged; its formerly empty marker now contains a 124-byte multiline JSON
fixture with version=1, its conventional entryClass and activateIn=[gameplay].
File SHA256 026FBF5C01BA259B656C3796E6688BCEFAF80A620ED93491840E595048B2823A.
Expected readback reconstructs this JSON (allowing native line-ending/trailing
newline handling), with nonzero line/character counts. Successful actor spawn
alone is NOT evidence of descriptor reading. No additional probe mod/family,
DML payload, DML save/list edit or menu entry spawn was introduced.

Installations, both via PS5.1 ValidateOnly then common installer:

- artifacts/installations/VoyageAutoLoader/20260905-054734-25056839-c6-7da33f48/install-manifest.json
- artifacts/installations/AutoLoaderCanary/20260905-054735-25056839-c6-reader-a29b5bb6/install-manifest.json

Restore these through the common tool to recover exact C5 and the empty
canary sidecar. New ZIP identities are 25056839-c6 and 25056839-c6-reader.
Final artifacts/autoload-candidate/c6/installed-readback.json confirms both
file sets/fingerprints match, game closed; no C6 markers existed before test.

Test: fresh launch, load same save, wait 5-10s after control, exit. Inspect C6
DONE's four Descriptor fields and compare captured JSON to installed fixture;
also retain prior actor-count/world checks. Only after the file-content gate
passes proceed with parsing, exact phase classification, Pawn-gate removal
and their separate empty/menu/gameplay/both/invalid tests. No game launched.

Tool report: fingerprint/status, exact container verification, Mod inspection,
manifest validation/install and readback passed. Build/fresh generation/cook
outside sandbox; final generation and cook 0 errors/0 warnings, no .NET crash.
Len's input identity S was corrected after header comparison before generation;
no failed generation or installed bad artifact. Loader 5-package verification
2bbc8371b02f41ddb0000c14f90584ea; unchanged canary 2-package verification
170f17f58c7b4b2e90200ab9b973bb82. Returned worker JSON inspection run
20260905T054644622Z-5d0ffc86 verified native LoadFileToArray owner, four new
report fields, join and C6 markers, with no concrete canary class embedded.
No tool-source inspection, broad extraction, new reusable tool gap or commit.
Conservative coverage 5/9 families public; existing experimental producer
gaps unchanged. Reading/parser/phase runtime work is unfinished mod behavior,
not a reason to introduce an external runtime prerequisite for players.

### C6 runtime: descriptor content read successfully

User test, 2026-09-05 UTC: STARTED 05:56:12.665207, canary BeginPlay
05:56:12.711760, DONE 05:56:12.902556, all VoyageWorld2. DescriptorFiles has
AutoLoaderCanary.autoload; native reader returned 7 lines, joined length 123.
Captured text exactly matches the installed 124-byte fixture except its final
newline. Offline JSON decoding confirms version=1, expected entryClass and
activateIn=[gameplay]; this is NOT proof of runtime JSON parsing, which remains
unimplemented. File-content gate passed independently of DML/actor ownership.
BeforeActorCounts=[0,1], SpawnResults=[true], FinalActorCounts=[1]: our loader
spawned the canary this run. No late/end-session duplicate count is claimed.

Evidence: artifacts/autoload-candidate/c6/runtime-result.json. Public
Get-VoyageInstallationStatus confirmed both C6 installations and fingerprints
match, game closed. Bounded read-only PowerShell 7 zlib/FString fallback ran
outside sandbox; segment/header/size/totals/EOF and string bounds passed,
with stable marker hashes before/after reading. Only our diagnostic saves read.
Tool report: 1/2 operation families public, existing decoder gap; no failures,
tool-source inspection, game launch, build, install or player-save mutation.
Next implementation remains runtime descriptor parsing, positive world-phase
classification and Pawn-gate removal with separate behavior tests. C6 still
ignores descriptor contents for activation; do not ship it as that feature.

## C7 installed candidate: optional key:value fields and world phases

User replaced JSON with a simple line format and explicitly required that EACH
field be optional. Both defaults are reset independently on every parser call;
invalid descriptors never reach class loading. The generated ParseAutoLoadDescriptor
Blueprint function passed 24/24 direct Blueprint-VM tests in the editor:
empty/comments, entry only, each phase only, both fields/orders, whitespace/CRLF,
duplicates, unknown keys/phases, empty values, malformed lines/JSON and bounds.
The override-only case uses a different physical default and virtual entry.
Tests execute compiled Blueprint with ProcessEvent; there is no second native
parser whose success could mask runtime graph errors. Test CDO state is cleared
before saving. Actual game parser/lifecycle behavior remains a separate gate.

Controller now admits exact current-level name Empty OR native IsPlaying;
there is no Pawn check, delay, DML wait or change to DML's widget/save behavior.
Worker snapshots phase as menu for Empty, gameplay otherwise (only controller-
eligible worlds spawn workers). Existing per-world worker/entry actor guards
remain. Parser errors, validity, selected entries and phase skips are recorded.
Worker checkpoint slots now end in _menu or _gameplay, preventing one phase
from overwriting the other's report. Repeated visits within a phase still
overwrite its fixed slot; final actor count is not an end-session count.

Current canary triplet is byte-for-byte C1, with only its descriptor/ZIP changed:

```text
entryClass: /Game/Mods/AutoLoaderCanary/ModActor.ModActor_C
activateIn: menu,gameplay
```

86-byte sidecar SHA256 179AD4446C9B3A155D3D828904DB2BE5F6D93A6E3BCB9ACCD6E88CA99F662D6D.
No new mod family or DML payload. Class-path precheck is deliberately basic
(absolute path, dot, generated-class suffix), not a complete package-name
validator; LoadClass/Actor-cast still gate spawning for nonexistent/non-Actor
classes. Native reader failure-vs-empty ambiguity and unbounded read remain.

Build-clean.log, generate-clean.log and cook.log under artifacts/autoload-candidate/c7
passed outside sandbox; final generation/cook 0 errors, 0 warnings. First build
had C++ API/type errors (TObjectPtr auto deduction, unsupported SelectBool and
FindFPropertyChecked, shadowed name), fixed before generation. First generation
passed VM tests but emitted return-node warnings. Targeted engine source review
showed FunctionResult::PostPlacedNewNode already reconstructs pins; calling the
generic allocator again duplicated exec pins. Special result-node construction
fixed the warnings. Neither intermediate reached Paks; no .NET crash.

Exact container verification: loader 5 packages acf3dc4b74d14aee9c1563e886a4b9da;
canary 2 packages 1f5c87ea37684e0bbc0bdeb5defb1c53. Returned worker JSON inspection
20260905T062319690Z-35d02137 confirmed parser/report fields and no canary/test
identity or abandoned DFA leak. Controller JSON 20260905T062348435Z-e7b32857
confirmed IsPlaying, positive menu check, OR/worker guard, optional soft DML,
and absence of GetPlayerPawn/delay. No stock game extraction was required.

Both installations used PS5.1 -File ValidateOnly then the common installer:

- artifacts/installations/VoyageAutoLoader/20260905-062417-25056839-c7-139595f1/install-manifest.json
- artifacts/installations/AutoLoaderCanary/20260905-062418-25056839-c7-356d4822/install-manifest.json

Restore these manifests to recover exact C6 loader and its JSON reader fixture.
Final c7/installed-readback.json confirms matching hashes/fingerprint for both,
game closed, Steam 25056839 / unchanged reviewed executable hash. No C7 markers
existed before test. No DML enable-list/player-save edit, game launch or commit.

Runtime test: fresh launch, remain in menu a few seconds, load the usual save,
wait 5-10 seconds and exit. Inspect C7_DONE_menu and C7_DONE_gameplay: respective
WorldName/ActivationPhase, descriptor valid, correct SelectedEntryPaths, no
phase skip/rejection, successful class load and one actor at each worker's end.
Menu/UI/control and DML console should remain normal. DML can win gameplay
creation; entry ownership is not proof of parser validity. This one both-phase
run does not prove every phase-exclusion/invalid case in game; editor VM tests
cover parsing, not all integration paths or arbitrary mod initialization.

Tool report: public fingerprint, container verification, Mod inspection,
manifest validation/install and status/readback succeeded. Existing experimental
producer build/generation/cook/packaging path reused: conservative 5/9 public
operation families. No new reusable gap or tool-source inspection; targeted
engine node inspection was triggered by the generation warnings. Existing
producer/diagnostic decoder gaps remain deferred, not authorization for new tools.

### C7 runtime: both-phase activation passed

User run 2026-09-05 UTC: DONE_menu 06:32:24.551100, WorldName=Empty,
ActivationPhase=menu; DONE_gameplay 06:32:43.427605, WorldName=VoyageWorld2,
ActivationPhase=gameplay. Both read exactly the two-line descriptor (85 joined
characters, final file newline omitted), valid=true, error empty, selected
entry matches the canary, no rejected or phase-skipped files. Both have
BeforeActorCounts=[0,1], SpawnResults=[true], FinalActorCounts=[1]. Our loader
created one actor in each world; the menu actor did not serve as the gameplay
instance. Fresh gameplay canary BeginPlay is 06:32:43.266389. Its fixed slot
overwrites menu BeginPlay, so the separate worker reports are the phase evidence.

This validates the known both-phase integration without a Pawn gate. It does
not prove phase exclusion, a different virtual entry in game, late duplicates,
repeat travel, visual/UI/console behavior or arbitrary mod initialization.
Those are not inferred from the user's short completion message or these saves.

Evidence: artifacts/autoload-candidate/c7/runtime-result.json. Public
Get-VoyageInstallationStatus confirmed both file sets/fingerprints match and
game closed. Bounded read-only PowerShell 7 zlib/FString fallback outside sandbox
checked headers, limits, block totals, EOF, string/array bounds and stable hashes;
empty FString entries are supported for DescriptorErrors. 1/2 public operation
families, existing decoder gap. No failure, tool-source inspection, build,
install, game launch, DML/player-save modification or commit.

### Pending C7 integration: BoatHUD entryClass only

User supplied the actor built in task "Проверь наследование BoatHUD"
(01a06fe3-b779-7da2-8650-f2fe9213cfa0). Current mod backlog/build/install
evidence and live installed hashes matched Steam 25056839 / reviewed exe.
Installed actor was inspected through Get-VoyageAssetJson -Source Mod;
returned JSON run 20260905T063640627Z-fd92064a confirms BeginPlay swap graph.
HUD candidate is not yet game-validated; its UI/lifecycle remains its owner's
responsibility. No HUD source, container or DML list was changed here.

Added only BoatHUDTotalResources_P.autoload at 2026-09-05T06:38:12.5821353Z:
entryClass: /Game/Mods/BoatHUDTotalResources/BP_BoatHUDTotalResourcesSwapActor.BP_BoatHUDTotalResourcesSwapActor_C
No activateIn field: gameplay-only default is intentionally under test. Physical
stem retains _P while virtual mod folder and actor name differ from convention.
Canary remains both-phase; C7 loader unchanged. Evidence/source sidecar under
artifacts/autoload-candidate/c7/boat-hud-entry-only. New file was absent before
creation, process and loader/HUD hash gates passed, sidecar readback matched,
HUD hashes unchanged afterward. Rollback removes only this exact sidecar after
closed-game/hash checks; do not remove or overwrite the colleague's containers.

Test: fresh launch/menu, load save, possess Boat, inspect Diesel/Electricity
totals/style, leave and re-enter Boat, then exit. C7 menu report must list HUD
as phase-skipped with no attempt; gameplay must select the explicit actor,
load and spawn it. Judge activation and visible HUD success separately.
Fixed C7 reports from the prior run must be distinguished by new timestamps.

Tool report: public fingerprint/status and Mod inspection passed; legacy HUD
installation hashes read manually because common status accepts only its own
schema. Sidecar-only creation was a bounded no-overwrite fallback (common
installer requires a full family release); no container rewrite was needed.
One read-only PowerShell command had a foreach-pipeline syntax error, corrected
before mutation. No build, cook, game launch or tool-source inspection. Deferred
sidecar-only install gap recorded in the cross-cutting backlog.

### BoatHUD integration result: loader contract passed, HUD visually failed

2026-09-05 UTC: menu report 06:41:28.018127 finds both sidecars, accepts both
descriptors and explicitly phase-skips BoatHUDTotalResources_P.autoload; no HUD
class load/spawn attempted in menu. Gameplay report 06:41:36.728081 selects and
loads the exact supplied SwapActor class. Before counts [0,1,0,1], spawn results
[true,true], final counts [1,1] in canary/HUD order: one spawn of each by us.
This validates entryClass-only default gameplay, nonconventional physical/path
mapping and two distinct sidecars in game. Counts are worker completion only.

User reports no crash but overlapping HUD values, with different appearance on
pause (two supplied screenshots). UI integration FAILS; do not label the HUD
candidate game-validated. Reports do not count widgets or identify late stock
HUD recreation. A one-shot swap preceding subsequent stock widget creation is
a plausible lifecycle hypothesis, not a proven cause or proof the loader's
activation timing is irrelevant. Do not add speculative loader delays/Pawn gates.

Evidence: artifacts/autoload-candidate/c7/boat-hud-entry-only/runtime-result.json.
Game still running during read; no installed file changed. Await closed game
before hash-guarded disabling of only the new HUD sidecar to recover the prior
autoload configuration. Preserve colleague-owned HUD containers/source.
Public installation status matched C7 loader hashes/fingerprint. Bounded
outside-sandbox read-only zlib/FString decoding passed stable hashes and bounds;
1/2 public families, existing decoder gap, no failures or tool-source inspection.

## C8 cleanup: prepared, not installed

User authorized removing experimental/debug baggage as the first release-prep
step, not expanding the feature set. Removed runtime AssetRegistry queries,
loading-state probes, empty/wildcard/PAK directory diagnostics, report arrays,
diagnostic descriptor capture, SaveGame checkpoints and BP_AutoLoaderReport.
Removed the unused anchor/provider experiment and editor native DirectoryExists
mirror, plus the generator's AssetRegistry module dependency. The shared header
still supplies semantic string/array/soft-path pin names; it does not imply a
runtime AssetRegistry dependency. Canary generation is now -GenerateCanary only.

Worker now discovers only *.autoload, parses each descriptor, applies phase and
three-file gates, loads/casts the selected class, checks existing instances once
and attempts at most one spawn. Removed the deliberate second spawn attempt
and end-of-probe counts. Invalid descriptor/class/family ends only that candidate's
loop body. Parser validation and 24 editor Blueprint-VM tests remain unchanged
(only the repeated empty literal was named). There is no replacement user-facing
diagnostic channel yet; error messaging belongs to the next scoped step.

Controller caches the existing or freshly created worker in an Actor reference.
While valid, Tick only tests that reference; it does not get the world name,
check readiness or scan all actors. Invalid reference reopens the existing
readiness/discovery path. This does NOT disable Tick entirely, cache a world by
name, impose a delay, restore Pawn gating or alter DML widget cooperation.
Teardown/travel/cache behavior remains a new game-validation gate.

Build-final.log, generate.log and cook.log under artifacts/autoload-candidate/c8
passed outside sandbox; 24/24 VM cases, generation/cook 0 errors/0 warnings.
Initial compile rejected Content variable shadowing a shared pin identity;
renamed before successful build. No .NET crash or bad installation. Previous
generated Content and the two pre-cleanup generator sources are preserved under
c8/pre-c8-content and c8/source-before-cleanup; C7 release/install evidence stays
intact. No installed mod, sidecar, diagnostic save or user save was deleted.

Release manifest: artifacts/autoload-candidate/c8/release-loader/release-manifest.json.
Artifact 25056839-c8 / modVersion candidate-c8-cleanup; same reviewed Steam
25056839/executable fingerprint. Exact four-package verification passed:
artifacts/container-checks/1466b51f9e5e4d0692cdfb03c0fde286/verification.json.
UCAS is 31,121 bytes (C7 56,525); ZIP is 9,161 bytes (C7 13,580).
Returned worker JSON run 20260905T065901751Z-ad980974 and controller run
20260905T065903947Z-fd32107e were checked for required loader/cache/DML calls and
absence of SaveGame, AssetRegistry, report, canary and test-default references.
Worker contains one BeginDeferredActorSpawnFromClass call. PS5.1 common release
ValidateOnly passed; no installation or game launch was performed this step.

Tool report: fingerprint/status, exact container verify, isolated Mod inspection
and manifest validation passed (5/9 operation families public). Existing
experimental build/generation/cook/packaging route remains the documented gap;
no new tool or tool-source inspection. Static success is not game validation.
Next: install the exact C8 artifact for lifecycle tests, retaining C7 rollback;
do not interpret old C7 diagnostic slots as new C8 output. A separately installed
canary may still write its own marker, but C8 loader itself writes none.

### C8 installation: runtime validation pending

User authorized installation on 2026-09-05. Common PS5.1 installer installed
25056839-c8 at 08:08 UTC after closed-game and matching Steam 25056839/executable
fingerprint gates, preserving the exact C7 predecessor. Installation evidence:
artifacts/installations/VoyageAutoLoader/20260905-080810-25056839-c8-8a5b3323/install-manifest.json.
Independent status readback at 08:08:23 UTC confirms all four installed hashes
and fingerprint match; game was closed. No game launch, DML/sidecar/HUD edits,
save deletion or source rebuild. Prior C7 report slots are not C8 evidence.
Next user test: menu DML console/list, save load, return to main menu and check
console again, reload save, then exit. This is lifecycle smoke coverage, not
proof of late actor counts or which loader created a DML-shared canary.

Tool report: fingerprint, installation status, recoverable install all passed
through documented entry points (3/3 operation families); no failures, fallback,
internal inspection or new reusable gap. Real-game result remains pending.

### C8 runtime failure: return to main menu hangs

User reported on 2026-09-05 that returning from gameplay to the main menu hung
the game. Supplied screenshot confirms DML list responded with DonkLiftKeyboardControl
and AutoLoaderCanary; it does not prove actor counts or successful return travel.
Repeat save-load step was blocked. Read-only status at 08:31:40 UTC found no
running game and confirmed C8 installed hashes and game fingerprint unchanged.
No installed files changed during this diagnosis. C8 is not release-ready.
Cause is unproven: new worker cache and existing widget teardown/recreation path
are investigation candidates, not an established diagnosis. C7 repeat-travel
coverage was incomplete, so this is not yet proof of a C8-specific regression.
Next discriminating test is exact C7 restoration with all other mods unchanged,
then the same gameplay-to-menu transition; do not layer speculative fixes on C8.

### C7 restored for controlled comparison

User authorized rollback. Common PS5.1 restore completed at 2026-09-05 08:35 UTC:
three C7 container files restored, only the newly installed C8 provenance ZIP
removed from Paks (source C8 archive remains under artifacts). Evidence:
artifacts/installations/VoyageAutoLoader/20260905-080810-25056839-c8-8a5b3323/restore-manifest-20260905-083510-5b7ed512.json.
Independent status against the C7 install manifest confirms installed hashes
and game fingerprint match, game closed. DML, other mods, sidecars and saves
unchanged. No game launch. Same gameplay-to-main-menu comparison is pending.
Tool report: documented restore and status passed (2/2 families); no failure,
fallback, internal inspection or new gap. No runtime success claimed.

### C7 menu failure and HUD-sidecar isolation

User reported C7 menu options did not work and Alt+F4 was required to exit.
This differs from the reported C8 hang; neither proves the cause. User approved
disabling only HUD autoload. At 2026-09-05T08:39:28.6164327Z, after public status
confirmed closed game and exact C7/fingerprint match, moved the exact 115-byte
BoatHUDTotalResources_P.autoload out of Paks to
artifacts/autoload-candidate/c7/boat-hud-entry-only/BoatHUDTotalResources_P.autoload.disabled-backup.
SHA256 before/after: 55EBA1685162E0FD8AF4E39094731FD0EF838FE4E0FD0C9A12028A53046064CB.
Original path absent; all 21 additional container files hash-identical before/after.
C7, DML, canary and HUD containers unchanged; no game launch or save mutation.
Restore only this exact sidecar from its backup after closed-game, expected-hash
and absent-destination gates. Next test: same save, pause menu, return to main
menu and normal exit. Sidecar absence disables our discovery, not hypothetical
activation by another loader or persisted actor; interpret runtime accordingly.
Tool report: public installation status succeeded; sidecar-only reversible move
used the already documented installer gap (1/2 families), no tool-source inspection
or failure. Runtime comparison pending.

### HUD-disabled comparison: user PASS

User confirmed "everything OK" for the requested same-save pause, return-to-menu
and normal-exit test after disabling only the HUD sidecar. C7 had failed with HUD
autoload enabled and passed with it disabled; this implicates HUD activation or
its interaction with the loader/UI lifecycle, not a proven HUD-internal cause.
C8-specific regression remains unproven; retest C8 with the same disabled sidecar
before accepting its cleanup. Keep C7 installed and HUD sidecar disabled until
that next installation is authorized. No installed files changed on this result.

### C8 without HUD: installed for pre-commit test

User requested testing before committing the checkpoint. At 2026-09-05 09:01 UTC
installed the exact existing C8 artifact with HUD sidecar absent and C7/hash/game
closed gates passed. Installation evidence:
artifacts/installations/VoyageAutoLoader/20260905-090131-25056839-c8-506e5603/install-manifest.json.
This new installation manifest is the current rollback anchor to C7, not the
previous already-restored C8 installation. Independent status confirms all C8
hashes/fingerprint match and HUD sidecar remains absent. No other mod/config/save
changes, game launch or commit. User lifecycle test pending.
Tool report: public status and installer passed (2/2 operation families), no
failure, fallback, source inspection or new gap.

### C8 lifecycle retest: user PASS and source checkpoint

User confirmed "Works" after the requested C8-without-HUD sequence: menu, save
load, return to menu, repeat save load and normal exit, including menu DML check.
This accepts that scoped lifecycle smoke test, not late actor counts, strict
activation order, arbitrary entry mods or HUD compatibility. C7 also passed
without the HUD sidecar. The HUD-enabled failures remain documented and unresolved.

User authorized a local source checkpoint. Commit scope is this mod and required
shared graph identities, not HUD, other mod work or uncommitted tool development.
Generated assets, cooked payloads, backups and logs remain ignored. The runtime
generator/parser are unchanged since the tested C8 artifact. Pre-commit hygiene
adds missing WorldSettings provenance comments and removes editor-generated
Android File Server configuration/token (not used by the Windows mod payload).
Source checkpoint is not a public release; producer tooling and wider validation
remain separate follow-up work. Tool report: scoped Git review/checks and user
runtime evidence; no rebuild, installation, game launch or new asset-tool gap.
