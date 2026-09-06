# Harpoon Cannon research and implementation backlog

## Active restart: 2026-09-05 Pacific / 2026-09-06 UTC

This section supersedes historical present-tense installation
and version claims below. Resume here after compaction, then read
`mods/HarpoonCannon/RESEARCH.md` and the relevant durable shared findings.

### Active: HC02 installed; natural interaction hints game-validated

User screenshot `C:/Users/IH/AppData/Local/Temp/codex-clipboard-22e88fd6-01ed-41a7-8fbe-f4631d7aa353.png`
shows the spawned Photo Drone visibly on the deck under the observer marker.
At 21.406092 observation seconds, initial spawn is VALID, current sample ALIVE,
camera distance 219.901586 cm, Actor hidden NO and Actor collision ON.
Stock HUD displays E Loot (Photo Drone), F Enter (Photo Drone), and mouse
Grab (Photo Drone). Natural target acquisition and stock action-hint rendering
are therefore game-validated for this unchanged standalone spawn setup, without
manual registration, item assignment, recharge or collision mutation.

This is not evidence that Enter succeeded, resource gating passed, or exit,
cleanup, travel/save, or attachment to the Harpoon shell works. The screenshot
is mid-observation, not DONE. Do not instruct the user to press F in HC02:
its unconditional timed cleanup can destroy a possessed test Drone.
Next candidate must first make cleanup safe for occupancy, then test native
entry/exit; preserve this successful stock creation/acquisition baseline.
Only documentation updated after this observation; installed files unchanged.

User authorized installation. Unchanged HC02 candidate01 installed at
2026-09-06 03:28:09 UTC through Install-VoyageRelease -AllowDirtySource.
HC01 matched before replacement, game absent; immediate installer process gate
passed. Recovery manifest:
`artifacts/installations/HarpoonCannonLifecycleProbe/20260906-032809-hc02-candidate-01-46805ed4/install-manifest.json`.
Its previous-files folder preserves the HC01 triplet and descriptor; HC02's
new versioned ZIP had no predecessor. The older HC01 ZIP remains provenance.
Public readback at 03:28:21 UTC: all five HC02 files match, zero mismatches,
current game fingerprint matches, process absent. No other mod files changed.
The requested acquisition screenshot was received as recorded above. Do not
enter, loot or manually save with HC02; no new package has been installed.

### HC02 preparation following accepted HC01 observation

User authorized continuing. Prepared in the same isolated AutoloadProbe source
scope; no model, native mirror, shell or installed-file changes authorized here.
The Drone source class, deferred creation, transform, activation/attachment and
item setup remain exactly the HC01 path. HC02 changes the observation window
from 8 to 60 seconds and adds observer-only instrumentation: whole-Actor bHidden
(read-only reflected Engine property), GetActorEnableCollision, and an
observer-owned TextRender label 45 cm above the Drone's origin. No component
collision, visibility, material, resource or registration fix is applied.

User test: approach the label and aim at the Drone; screenshot stock action
hints or their absence while HC02 is observing. Do not press Enter/Loot: timed
cleanup is still active. Whole-Actor flags do not prove mesh visibility or
per-channel interaction trace responses. DONE remains pre-cleanup evidence.
This is natural action-acquisition observation, not an entry/exit test.

Current source inspection confirms DroneModuleComponent already references
DA_CameraDrone and configures DefaultResourceAmount/MaxResourceAmount=100,
plus MaxResources Electricity=100. Runtime charge/initialization is NOT proven
by these defaults; no speculative item initialization was added. VehicleMesh
has Interact overlap, while ActiveCollisionSphere explicitly ignores Interactive
and starts NoCollision. Those are distinct components/channels, not grounds to
toggle collision blindly. GetInteractiveProvidedActions delegates to
GetInteractActions, as the new focused summary confirms.

Candidate `artifacts/harpoon-cannon/hc02-candidate-01` is prepared and now installed.
Build, Blueprint compile, exact one-package cook, bounded retoc, exact inventory,
new common manifest producer/ValidateOnly, and static cooked checks passed on
the first build invocation. Source hashes match build-provenance.json. Eight
TextRender components, 60-second termination and read-only Drone diagnostics
were recovered from the actual candidate, not the installed HC01. Cooked JSON
SHA `5575D9C460AF2F3284CD5CDB0A04528D1B6D331796B373E0E2EC36CB4D191784`.
Verification report: `artifacts/container-checks/0840df6ac97140b1811414e911bfcf56/verification.json`.
Release identity: candidate directory's immutable release-manifest.json;
installation must use it with AllowDirtySource only after authorization.
Public HC01 readback at 03:24:24 UTC: game absent, all five files match, current
fingerprint matches. This does not replace the next install's live process gate.
Build-Probe now calls New-VoyageReleaseManifest instead of hand-authoring schema2;
separate build-provenance.json retains source/engine/mapping and gate evidence.
HC01 was preserved until the authorized closed-game replacement recorded above.
Tool findings and outcome are owned by `docs/harpoon-cannon-hc02-tool-report.md`.

### HC01 checkpoint and evidence

User renewed implementation scope: autoload may create new actors; prepare
the next experiment. Ownership is the separate `mods/HarpoonCannon/AutoloadProbe`
UE5.8.2 editor-only project. Do not rebuild old UE5.7 native mirrors or modify
model/shell. User subsequently authorized installation of the prepared HC01.

**Installed 2026-09-06 02:23:30 UTC:** unchanged `hc01-candidate-05`, through
Install-VoyageRelease with AllowDirtySource. Immediate process gate passed.
Installation/recovery manifest:
`artifacts/installations/HarpoonCannonLifecycleProbe/20260906-022330-hc01-candidate-05-4bafd412/install-manifest.json`.
Independent public status at 02:23:40 UTC: five files match, zero mismatches,
game fingerprint matches, process absent. The five owned destinations were
previously absent; recovery removes this probe through the paired restore tool,
not through a fictitious predecessor backup. Other mods unchanged.
**Runtime observation received:** user screenshot
`C:/Users/IH/AppData/Local/Temp/codex-clipboard-8e14d4f5-a2e2-42ad-b6ec-ef0c3eb52d1d.png`
shows HC01 DONE, Initial spawn VALID, Last independent sample ALIVE,
Observation seconds 8.002636, Drone distance (cm) 455.186035 (about 4.55 m).
The observer and its text are visibly live and reached the terminal sample.
This validates initial creation and reference validity through that observation
window for the standalone autoload path; it does not validate Drone interaction,
physical visibility, native initialization completeness, persistence, or shell
attachment. DONE explicitly says cleanup requested, not cleanup verified.
The screenshot is not evidence of post-cleanup destruction or crash-free travel.

The broad hypothesis that directly spawned stock Drone necessarily disappears
within a few seconds is contradicted for this setup. The old build/shell and
latent marker remain a separate unresolved path, not a proven shell defect.
Next in-scope investigation: action acquisition/registration and native item-drop
initialization before choosing a one-variable interaction probe. Keep HC01
unchanged until a separately prepared checkpoint and closed-game install gate.
No game files, model or gameplay source changed when recording this result.

HC01 creates one current-stock CameraDrone after a valid player/camera exists,
using only Engine APIs and a soft class load. No SetDroneActive, attachment,
construction adapter, interaction interface, possession or HUD replacement.
Independent Tick sampling displays initial validity, current validity, elapsed
observation seconds and camera distance on a world-space TextRender board.
At >=8 seconds it freezes the final sample and requests destruction of only
its own spawned Drone. No latent Delay and no disappearing-model marker.
The board continues following the camera so the user can take one screenshot;
no stopwatch or reading invisible properties is required.

Interpretation: an incrementing clock proves the observer's continuation;
initial VALID followed by INVALID discriminates lifetime loss; ALIVE at DONE
establishes survival only for this standalone path. This is not an exact A/B
against the old build's shell (build, activation and attachment differ), so
survival cannot establish the shell as the sole historical cause. A crash or
absent board requires stopping and diagnosis, not invisible field tweaking.
Do not claim Enter/HUD solved from this test. Game is disposable for the test;
do not enter, loot or manually save with the probe. Native persistence/autosave
behavior and early-world teardown are not yet runtime-validated.

Preparation failures retained under ignored artifacts:
- `autoload-lifecycle-01/build.log`: own UE5.8 TObjectPtr/auto-pointer compile
  error, corrected with explicit UEdGraph/UEdGraphNode pointer conversions.
- `hc01-candidate-01`: own wrapper treated JSON stdout as a PS object; corrected
  with ConvertFrom-Json. No game or tool defect, no package produced.
- `hc01-candidate-02/generate-unreal.log`: Blueprint SetText's by-ref Value
  rejects an unwired default FText. Changed generator to wire Conv_StringToText.
  C++ build passed; generation correctly stopped before cook/package.
- Candidate03 exposed reflected-but-not-BlueprintCallable SetText; replaced
  with K2_SetText and added a callable-flag assertion.
- Candidate04 prepared successfully; unused AndroidFileServer created a source
  config. Disabled the plugin and removed only that generated config.
- **Prepared checkpoint: `artifacts/harpoon-cannon/hc01-candidate-05`.** Build,
  graph compile, exact cook, bounded retoc, one-package inventory, schema2
  ValidateOnly and static cooked assertions passed. Source hash readback matches.
  See `docs/harpoon-cannon-hc01-tool-report.md`. This is HC01's candidate05,
  unrelated to the old crashing fabrication operator package-05.
- Before installation, public status at 02:15:02 UTC:
  game absent, no Harpoon files installed. Do not reuse that process observation
  as a future install gate. When explicitly authorized, use the candidate's
  release-manifest through the common installer with AllowDirtySource; preserve
  its returned installation manifest for removal/recovery.
- Candidate05 hashes: pak `75E7144577253917F6DA7312EF5E585B12FB728226A22B0938323751A6B555CD`;
  ucas `726F4159BFE46F223900F4A91D1FBCAA70FBB82A7E22039D6452D2427344C04B`;
  utoc `9969466DDE61E432A5CBA3309B65F905670625F48E9251B9309DC854F76ED29E`;
  descriptor `61CB50478478F07439F3FAB6DC973E3E0CBEF64B11DE8D27B66B5222A9094A8A`.
  ZIP `31F2CBEF9B82F1BE61FE9D232ED3341B2B8583D86A36C118BBCE994ED94C2093`.

Pipeline task reports HC-R1/R2/R3 fixed locally and uncommitted: optional-only
pseudocode failure no longer blocks proven JSON; -AsJson is compact; Inspector
defaults to physically isolated Game containers. Reuse through documented
interfaces on the next corresponding request; these handoffs are not a local
retest. The new probe is user-requested owner discrimination, not a workaround
for the now-resolved turret inspection failure.

### Pre-experiment research snapshot

- Fresh public fingerprint: Steam `25056839`, executable SHA-256
  `CA84428CF4562C703BEDFF053DB727D14CC70C593451C09BE75A92828EFD9933`.
  Reviewed mappings resolve successfully; no dumper or mapping generation.
- Public installation status found no Harpoon/CannonPlacementProbe files in
  top-level Paks and no unscanned subdirectories. The game was absent at
  01:35 UTC and present at 01:36 UTC. This is a filesystem/process snapshot,
  not a runtime mount or other mods' compatibility guarantee. Nothing installed,
  restored, deleted, built, generated or cooked in this iteration.
- Current Harpoon source still selects the delayed stock-Drone validity marker.
  `Voyage.uproject` still names UE 5.7 and `GAME_DERIVED_SOURCES.md` still gates
  build 23962331. Neither is renewed merely by obtaining current mappings.
- Last gameplay stopping point: fabrication completed without a crash; the
  diagnostic barrel disappeared; entry/HUD were absent. Immediate spawn validity
  was demonstrated on the old build. Delayed survival and its failure cause
  were NOT independently demonstrated. See corrected interpretation below.
- Revalidated current stock data: Cyclone item/medium-child/item-component link,
  Telescope FOV and Binoculars camera/HUD consumers, Drone action/deployment
  graph, and three identifiable shark Blueprint/data pairs. Native mirror
  layouts, fabrication/attachment lifecycle, HUD declaring interface, and
  save/load are still pending current-build validation.
- Stock turret inspection is blocked by the Inspector pseudocode formatter,
  not an established game-asset parsing incompatibility. No failed staging JSON
  was used as research input. The bounded failure and two interface usability
  issues are recorded in `docs/harpoon-cannon-tool-report-2026-09-05.md` and
  linked from the shared toolchain backlog.

Current research order:

1. Obtain a supported successful stock-turret JSON inspection after the
   formatter failure is addressed; determine manual-control versus autonomous
   ownership from its actual graph, not its name. Do not add a runtime probe
   just to work around this analysis-tool failure.
2. Audit Drone's native creation/registration and the old marker's latent
   continuation independently if Drone remains a candidate. Do not repeat
   activation, offset, collision or guessed-interface variations.
3. Compare operator choices only after those owner contracts are clear. Keep
   Cyclone shell and Telescope optical consumers separate; preserve model and
   base-mesh simple collision. Renew every serialized mirror before any future
   build, then validate entry/exit before aim, HUD or shark tracing.

Documentation maintenance: replaced the mod's accumulated chronological rules
with stable ownership/gates and a pointer here. Historical experiments remain
below, including recovery evidence; this removes contradictory mandatory
"currently installed" states without deleting the safety contracts or history.

## Status and scope

The following chronological sections describe build 23962331 unless explicitly
stated otherwise. They are not current-build compatibility declarations.

Research, isolated implementation, and reversible real-game construction
probes are active. Generic placement, corrected placeholder rendering,
construction/completed carrier attachment, and completed-object dismantling
are game-validated. Cancellation of unfinished construction is also
game-validated after adding simple collision to the base mesh copied by the
temporary `BP_Location_Fabricator`; enabling overlap generation on the shell
mount remains a confirmed no-op. Migration of the
disposable adapter from Cyclone to Whisper is rejected:
overriding Whisper's small-turbine Blueprint replaces the parent package of the
untouched Cyclone Blueprint and crashes async loading. All Harpoon container
files from those rejected candidates were removed. The new leaf Cyclone rough
proxy candidate now game-validates startup, construction-menu loading, completed
construction, and assembly of the three visual mesh groups.

The intended object is a fabricated/placed stationary cannon that:

- uses the same module/item placement family as the Cyclone wind turbine;
- is fixed to its supporting surface;
- lets the character enter and exit through the vehicle interaction lifecycle;
- follows mouse aim with a cannon optical axis;
- switches to an approximately 5x sight view while occupied;
- traces the optical axis and, when the hit object is a shark, shows distance
  on the HUD.

Construction UI and unlock progression are explicitly deferred. This research
may identify the data contract they will eventually need, but must not build or
patch that path yet.

## Locked design decisions

- The mod and object name is `HarpoonCannon`. Sharks are the initial target
  family, but generic aiming, tracing, distance, and HUD layers must allow
  explicit future wildlife target sets without a redesign.
- Every shark variant counts. The filter must cover the complete current-build
  inventory and be re-inventoried after a game update rather than assuming the
  player has already encountered every variant.
- Displayed distance is from the mounted character to the optical trace impact
  point. Approximate gameplay distance is sufficient; no ballistic or
  scientific correction is required.
- The cannon has unlimited yaw through a full 360 degrees and pitch constrained
  to 50 degrees down and 10 degrees up.
- Entering the cannon must switch to a dedicated first-person optical view and
  a cannon HUD. The forklift's third-person camera and vehicle HUD are behavior
  references for possession lifecycle only, not visual parents for the result.

## Compaction and coordination rule

After every context compaction, reread the active restart section, locked design
decisions, `mods/HarpoonCannon/RESEARCH.md`, and relevant historical experiments plus
`docs/vehicle-and-hud-modding-patterns.md`, the relevant sections of
`docs/game-architecture-observations.md`, and `docs/research-pitfalls.md`
before continuing.

Keep this work independent from active DonkLift source/generator work. Do not
modify the installed game, build/cook a mod, or edit DonkLift files during the
research-only phase. Keep version-derived inspection output under ignored
`artifacts/cannon-research/`; commit only durable documentation or reusable
methods when separately authorized.

## Current version gate

Read-only fingerprint captured on 2026-08-29 Pacific time:

- Steam build: `23962331`;
- executable SHA-256:
  `6A9AE86E5CE5D7D1B6555F579091AAB1E0E67FF7A96276FA2570052F99102E8D`;
- the fingerprint matches the current DonkLift provenance registry.

The facts below are static inspection results for this exact build. They are
not real-game validation of a cannon.

## Implementation checkpoint 0: isolated asset scaffold

Real code work began in the isolated
`mods/HarpoonCannon/` Unreal project. Its owned paths do not overlap
the active DonkLift task. That task was explicitly notified that this work owns
only the new mod directory and this backlog.

The first generator checkpoint creates:

- `BP_HarpoonCannon`, derived from the editor-only exact-identity
  mirror of `/Script/Voyage.VoyageVehiclePawn`;
- component hierarchy `MountRoot -> YawPivot -> PitchPivot -> OpticalAxis ->
  OpticalCamera`, plus a separate placement collision probe;
- camera FOV `22.619865` degrees, corresponding to a 5x rectilinear view from a
  90-degree base FOV;
- `WBP_HarpoonCannon`, derived from the exact-identity mirror of
  `/Script/Voyage.VoyageBaseUserWidget`, with a centered reticle and a hidden
  shark-distance text element;
- named constants for the -50/+10 pitch contract, full yaw, all three
  current-build shark data identities, asset paths, component identities, and
  visual tuning.

Static verification on 2026-08-29 Pacific time:

- Unreal Engine `5.7.4` UHT/native build: succeeded;
- clean UHT/native rebuild after the `HarpoonCannon` rename: succeeded;
- `GenerateHarpoonCannon` commandlet after the rename: succeeded;
- generated package identities are `/Game/Mods/HarpoonCannon/BP_HarpoonCannon`
  and `/Game/Mods/HarpoonCannon/WBP_HarpoonCannon`;
- both generated Blueprints compiled and saved with `0 error(s), 0 warning(s)`;
- generated assets remain under ignored `Content`; no cook, package, install,
  or installed-game mutation occurred.

This checkpoint intentionally does not claim placement, welding, persistence,
entry/exit, aim input, trace, shark classification, distance updates, or HUD
selection. `GetHUDOverrideWidget` remains unbound until its exact native owner
and runtime lifecycle are confirmed.

## Placement probe preparation

The first reversible experiment temporarily places a generated child of
`BP_HarpoonCannon` at Cyclone's existing dropped-actor package path. It changes
only the spawned actor while deliberately retaining Cyclone's recipe, unlock,
preview, and generic placement flow. This is a disposable architecture probe,
not the production Harpoon Cannon item identity.

The first visual placeholder attempted to use `/Engine/BasicShapes/Cube`.
Current shipping-container inventory contains no such package. Cooking the
Blueprint without references would therefore leave an unresolved visual asset;
cooking the Engine package and its material dependencies would make the probe
unnecessarily broad. This path was rejected before cook or installation.

An intermediate attempt used Cyclone's shipping mesh
`/Game/AssetSets/Modules/WindMill/Stationary/V1/SM_Windmill_Stationary_Base_02_Addition`.
Package inventory confirmed that exact game asset, and generation succeeded
against an empty editor object with the same identity. However, the narrow cook
still validates the referenced static mesh and stopped with
`Bad MeshDescription`; `CookSinglePackageNoRefs` does not make an invalid hard
reference cookable. This empty-placeholder technique is therefore also a bad
path, even when the referenced runtime package exists.

The next narrow implementation is fully self-contained: the generator builds a
small valid cube mesh at
`/Game/Mods/HarpoonCannon/SM_HarpoonCannonPlaceholder`. That mod-authored mesh
is cooked and packaged alongside the two probe Blueprints. It adds one tiny
package but avoids both copied game content and absent Engine dependencies.
The first generator run crashed because `FMeshDescriptionBuilder` does not
register the mandatory static-mesh attributes itself; constructing an empty
`FMeshDescription` and calling `SetMeshDescription` is insufficient. The
generator now explicitly calls `FStaticMeshAttributes::Register()` before
using the builder. This is a tool implementation finding, not evidence against
the placement architecture.

Placement-probe preparation completed on 2026-08-29 Pacific time:

- the editor generator rebuilt successfully and generated the valid cube mesh,
  cannon pawn, optical HUD skeleton, and temporary Cyclone-path child;
- three independent `CookSinglePackageNoRefs` cooks succeeded for the cube
  mesh, cannon pawn, and temporary Cyclone-path child;
- the HUD was intentionally not cooked because this experiment tests placement
  only;
- `retoc to-zen` and `retoc verify` succeeded;
- semantic inventory contains exactly these three packages and no editor-only
  module binaries or HUD assets;
- the disposable container remains only under ignored
  `artifacts/harpoon-cannon/placement-probe-package-01/package`; it has not been
  installed into Voyage.

Prepared container hashes:

- `HarpoonCannonPlacementProbe_P.pak`:
  `75E7144577253917F6DA7312EF5E585B12FB728226A22B0938323751A6B555CD`;
- `HarpoonCannonPlacementProbe_P.ucas`:
  `10F6C461DAAD8F67A8056CEF54CEF9CE70FA43C1C2D95C74069B80285E28BEA8`;
- `HarpoonCannonPlacementProbe_P.utoc`:
  `C36CDB2530CBC540DDBB8AD8F6AF4B4C349FF837E83C5B8A8A489C1DAA68FB09`.

The user explicitly confirmed that Voyage was closed, and the placement probe
was installed on 2026-08-29 Pacific time as exactly these new files under the
game's `Voyage/Content/Paks` directory:

- `HarpoonCannonPlacementProbe_P.pak`;
- `HarpoonCannonPlacementProbe_P.ucas`;
- `HarpoonCannonPlacementProbe_P.utoc`.

No same-name targets existed before installation, so no installed file was
overwritten and no backup was needed. Post-copy SHA-256 readback matched all
three prepared-container hashes above. This proves installation integrity only;
real-game placement validation remains pending.

Expected discriminating outcomes when the probe is eventually installed with
the game closed and tested:

1. Cyclone preview and placement show the conspicuous base/turret/barrel shape:
   the generic module item path can instantiate this vehicle pawn.
2. Preview appears but placement is rejected: the pawn is missing a placement
   collision/component contract rather than the item-to-actor link.
3. The original Cyclone remains: the override container/path did not win.
4. The game crashes or refuses to load the class: the current native parent or
   generated-class assumption is wrong; remove the probe and return to static
   architecture evidence.

## Real-game placement result

The first installed placement probe was tested in game on 2026-08-29 Pacific
time. User-provided screenshots show both the red construction preview and the
constructed blockout on the ship deck. The user reported that preview and
construction were functionally working.

This validates for the current fingerprint that:

- the custom IoStore container wins at Cyclone's dropped-actor package path;
- Cyclone's existing item/recipe flow accepts the generated
  `VoyageVehiclePawn` child as its preview actor;
- the same flow completes construction and leaves the generated pawn on the
  selected ship surface;
- the leading one-pawn architecture is viable enough to continue; the
  two-actor module/operator fallback is not justified by placement alone.

This result does not yet prove attachment during ship movement, persistence
across save/load, destruction/refund behavior, or entry/possession.

The procedural cube's triangle winding was visibly reversed: outside faces
were culled while interior-facing surfaces rendered. This is confined to the
mod-authored visual marker. The generator now reverses every triangle winding;
the installed probe must not be replaced until Voyage is closed again.

The corrected-winding probe was rebuilt while the user continued playing,
without touching the installed files. Native build, commandlet generation,
three narrow cooks, `retoc to-zen`, exact three-package inventory, and
`retoc verify` all succeeded. The replacement candidate is under ignored
`artifacts/harpoon-cannon/placement-probe-package-02/package` with hashes:

- `.pak`: `75E7144577253917F6DA7312EF5E585B12FB728226A22B0938323751A6B555CD`;
- `.ucas`: `B12BC2904B1BB399C6707BB08CFB9EA76C952EC7AC7736953A1D361B01FD9F44`;
- `.utoc`: `95F03BF07DC36546315323E8A2BAA8B866C9DBE98B49FD80069EA9EDCFEBD18E`.

After the user confirmed that Voyage was closed, package-02 replaced the three
installed probe files. Before replacement, all installed hashes matched the
preserved package-01 baseline. The replacement command retained package-01 as
the recovery source and would have restored all three files on any copy or
readback failure; no rollback was needed. Installed package-02 readback hashes
match the corrected candidate above.

The next combined real-game check is:

1. verify that preview and constructed exterior faces now render normally;
2. move the ship and observe whether the constructed pawn follows the deck;
3. save and reload, then check whether the object still exists in the same
   relative location.

### Corrected-mesh attachment result

The package-02 game test confirmed that the corrected exterior faces render
normally. Placement still succeeds on the ship, but the user observed that the
constructed pawn appears to remain in global/world space rather than following
the ship. This is preliminary until the exact motion symptom is recorded, but
it clearly invalidates the assumption that successful module-item placement
automatically attaches an arbitrary `VoyageVehiclePawn` to the selected
carrier.

Placement-target selection and carrier attachment are therefore separate
contracts. The strongest next hypothesis is the missing Cyclone-owned
`VoyageDynamicCollisionComponent` with `bAutoWeld=true`, possibly together with
its collision-bearing primitive/component registration lifecycle. Do not add
several Cyclone module/persistence/destruction components at once: the next
probe should isolate dynamic collision/auto-weld, and only after the user's
motion observation distinguishes a fixed world transform from physics drift.

Save/load persistence for package-02 has not yet been reported.

The user then supplied the decisive motion symptom: while the ship translates,
the constructed pawn moves in the opposite direction relative to the deck,
which means it remains at the same world-space transform. It also ignores ship
pitch/roll and visually bobs above the moving deck. This confirms that the pawn
is neither attached to the carrier transform nor welded through the ship's
motion lifecycle. It does not look like an unconstrained physics body falling
or drifting under forces.

The next probe is therefore locked to one architectural variable: add the exact
current-build `VoyageDynamicCollisionComponent` contract with `bAutoWeld=true`
and retain the existing collision-bearing root/blockout. Do not combine this
with persistence, destruction, module workload, possession, camera, or HUD.

Current mapping inspection confirms the exact minimal mirror contract:

- `/Script/Voyage.VoyageDynamicCollisionComponent` derives directly from
  `/Script/Engine.ActorComponent`;
- it has exactly two own mapped properties, in order: `bAutoWeld` and
  `bAutoweldIgnoreNormal`;
- Cyclone adds this non-scene component as an SCS root node and serializes only
  `bAutoWeld=true`;
- inspected forklift and JetSki instances do not serialize either flag, which
  is consistent with their non-welded vehicle behavior.

The attachment probe mirrors those two fields and adds the component as a
separate SCS root with `bAutoWeld=true` and the default
`bAutoweldIgnoreNormal=false`. No other gameplay component or callback changes
in this probe.

The isolated attachment candidate then passed the complete local gate:

- native/UHT build succeeded;
- commandlet generated and compiled all assets with `0 errors / 0 warnings`;
- the single-process exact three-package cook succeeded;
- cooked `BP_HarpoonCannon` contains the
  `/Script/Voyage.VoyageDynamicCollisionComponent` reference and generated
  component identity;
- exact three-package IoStore inventory and `retoc verify` succeeded.

Prepared attachment-probe container:

- ignored root: `artifacts/harpoon-cannon/attachment-probe-package-01/package`;
- `.pak`: `75E7144577253917F6DA7312EF5E585B12FB728226A22B0938323751A6B555CD`;
- `.ucas`: `E1BB90A493B90B877C13EFBD27A2C63CFC7748C1874D9EEA6009A62B8B056343`;
- `.utoc`: `8DBA412CFE18FC2BB1A21876E6559EF659498A46E87C1DAC73370B8B19D3F8E8`.

It has not replaced the installed package-02. Installation remains gated on
the game being closed.

The user confirmed that Voyage was closed. Before replacement, all installed
hashes matched the preserved package-02 baseline. The attachment probe then
replaced the same three container files with automatic package-02 rollback on
failure; no rollback was needed, and installed SHA-256 readback matches the
prepared attachment candidate.

The active game experiment now tests only whether a newly constructed object
follows ship translation and pitch/roll. Preview, visual winding, persistence,
possession, camera, HUD, and aiming are not variables in this probe.

### Auto-weld no-op

The real-game attachment test failed: a newly constructed object still lives
independently in world space and does not follow ship translation or rocking.
Therefore `VoyageDynamicCollisionComponent` with Cyclone's serialized
`bAutoWeld=true` is necessary-looking static evidence but is not sufficient on
an otherwise plain `VoyageVehiclePawn`.

Do not respond by toggling `bAutoweldIgnoreNormal` or adding all Cyclone
components blindly. Cyclone leaves that second flag at its default, and the
no-op now points to a missing owner/registration lifecycle. The next static
split is between native `VoyageModuleActor` ownership and registration through
`VoyageCustomModuleComponent`. If attachment is class-owned, the two-actor
module/operator fallback becomes justified; if component-owned, the smallest
exact module-component contract should be probed first.

### Attachment owner split

Current-build mapping and asset comparison narrow the next experiment further:

- `VoyageCustomModuleComponent` derives from `VoyageModuleComponent`; its own
  fields describe resource conversion and module operation, while the base
  component owns module registration, item/config data, sockets, resources,
  state, and subsystem references;
- the same custom module component is present on the mobile forklift, JetSki,
  and GyroCopter, so adding it to another `VoyageVehiclePawn` is not a
  discriminating attachment experiment;
- `VoyageModuleActor` derives directly from `Actor` and owns four native
  references: `bCollectable`, `ModuleComponent`, `PersistentComponent`, and
  `DestructibleObjectComponent`;
- after the pawn-level auto-weld no-op, Cyclone's remaining structurally unique
  attachment candidate is therefore the native `VoyageModuleActor` owner
  lifecycle, not another serialized module-resource flag.

The next probe adopts the previously deferred two-actor boundary without yet
adding possession or input behavior. The temporary Cyclone-path actor becomes
a minimal `VoyageModuleActor` shell with a collision-bearing box root and
`VoyageDynamicCollisionComponent(bAutoWeld=true)`. The existing
`BP_HarpoonCannon` vehicle pawn becomes a `ChildActorComponent` of that shell.
The auto-weld component is removed from the child pawn so the one changed
architectural variable is its owner: module shell versus vehicle pawn.

Discriminating outcomes:

1. The shell follows ship translation and rocking, and the child cannon follows
   it: native `VoyageModuleActor` owns the missing attachment lifecycle and the
   production architecture should remain module shell plus operator pawn.
2. The complete pair remains world-fixed: native actor ownership plus a
   collision-bearing owner primitive is still insufficient; stop package
   iteration and locate the placement result's carrier-registration callback.
3. The shell attaches but the child does not follow it: the module boundary is
   correct, but `ChildActorComponent` is unsuitable for the operator link and
   should be replaced by an explicit runtime spawn/attach contract.

The module-shell candidate passed its complete local gate on 2026-08-29
Pacific time:

- current installed fingerprint still matches Steam build `23962331` and the
  registered executable SHA-256;
- UHT/native `VoyageEditor` build succeeded;
- commandlet generation compiled and saved all assets with `0 errors / 0
  warnings` after using the project-local DDC;
- one-process exact three-package cook succeeded;
- cooked shell strings confirm `VoyageModuleActor`,
  `VoyageDynamicCollisionComponent`, collision root `ModuleMountCollision`,
  child component `HarpoonOperator`, and child class `BP_HarpoonCannon_C`;
- cooked pawn strings confirm `VoyageVehiclePawn` and its placement collision,
  with no remaining dynamic-collision/auto-weld component identity;
- exact three-package IoStore inventory and `retoc verify` succeeded.

Prepared but not installed candidate:

- ignored root:
  `artifacts/harpoon-cannon/module-shell-attachment-probe-package-01/package`;
- `.pak`: `75E7144577253917F6DA7312EF5E585B12FB728226A22B0938323751A6B555CD`;
- `.ucas`: `CED612AF4908AF2E1F5F07DA9EF1B4A9FC9398E0F0E73259A2C0C5892ADFE39D`;
- `.utoc`: `61AA0D322BE96E8A1BC6999060F2E4FB7A9ACC9BF8F826960ECEC890039C9AF2`.

The user newly confirmed that Voyage was closed. Process readback found no
running shipping executable, and all three installed hashes matched the
confirmed pawn-level auto-weld no-op before replacement. That baseline was
copied to ignored
`artifacts/harpoon-cannon/installed-backups/before-module-shell-attachment-probe-01`
and hash-verified before installation.

The module-shell candidate then replaced exactly the three
`HarpoonCannonPlacementProbe_P` container files. Automatic rollback was armed
for any copy or readback failure; no rollback was needed. Installed SHA-256
readback matches the prepared candidate:

- `.pak`: `75E7144577253917F6DA7312EF5E585B12FB728226A22B0938323751A6B555CD`;
- `.ucas`: `CED612AF4908AF2E1F5F07DA9EF1B4A9FC9398E0F0E73259A2C0C5892ADFE39D`;
- `.utoc`: `61AA0D322BE96E8A1BC6999060F2E4FB7A9ACC9BF8F826960ECEC890039C9AF2`.

The active game test is still attachment-only: construct a fresh object, move
the ship translationally, then observe pitch/roll. Persistence, entry, camera,
HUD, aiming, and firing are not variables in this package.

### Module-shell preview crash

The module-shell candidate crashed Voyage as soon as the user selected Cyclone
in the construction menu, before placement could be attempted. The fresh crash
record is
`UECC-Windows-032AEE24435C54002920D6BBCE9ACCF5_0000`; it reports a GameThread
`EXCEPTION_ACCESS_VIOLATION` reading address `0xF`, 38 seconds after process
start. Shipping symbols are unavailable, so the raw stack does not identify a
named function.

This invalidates the packaged shell as a loadable preview actor. It does not
yet distinguish between two causes introduced together:

1. the editor-only `VoyageModuleActor` mirror omitted its four known native
   properties, producing an unsafe cooked parent/CDO schema;
2. preview construction of a `ChildActorComponent` containing a vehicle pawn
   is invalid in this module placement lifecycle.

Do not reinstall or extend this candidate. The game process was confirmed
stopped and the three installed files were immediately restored from the
hash-verified pre-probe backup. Installed readback again matches the previous
pawn-level auto-weld package:

- `.pak`: `75E7144577253917F6DA7312EF5E585B12FB728226A22B0938323751A6B555CD`;
- `.ucas`: `E1BB90A493B90B877C13EFBD27A2C63CFC7748C1874D9EEA6009A62B8B056343`;
- `.utoc`: `8DBA412CFE18FC2BB1A21876E6559EF659498A46E87C1DAC73370B8B19D3F8E8`.

The next work is static and must split those causes before another game test:
mirror the exact `VoyageModuleActor` property schema and determine whether a
shell-only preview can be generated with direct visual components. Do not put
the operator child pawn back into the next package until that shell alone
loads, previews, places, and follows the carrier.

### Native Cyclone and auto-weld disassembly result

A targeted current-build disassembly was performed against Steam build
`23962331`, executable SHA-256
`6A9AE86E5CE5D7D1B6555F579091AAB1E0E67FF7A96276FA2570052F99102E8D`.
The investigation was limited to the native constructors and the callers of
Cyclone's module and dynamic-collision contracts; it did not scan or modify the
running game.

The original Cyclone depends on native constructor state that the crashing
editor-only shell did not reproduce:

- `AVoyageModuleActor` is `0x310` bytes in this build. Its native constructor
  creates three named default subobjects and stores them at the mapped fields:
  `ModuleComponent` at `+0x2F0`, `PersistentComponent` at `+0x2F8`, and
  `DestructibleObjectComponent` at `+0x300`; `bCollectable` is false at
  `+0x2E8`;
- the base turbine Blueprint does not add those components through SCS. Its
  generated class uses `ComponentClassOverrides` to replace the inherited
  native `ModuleComponent` template with `VoyageCustomModuleComponent`, while
  the CDO references all three native `RF_DefaultSubObject` instances;
- `UVoyageModuleComponent` is `0xDF8` bytes and has substantial native
  initialization/lifecycle code. During initialization it resolves and stores
  both `VoyageEffectSubsystem` and `VoyageModuleSubsystem`, then processes the
  owner, config/resources, destructible component, and optional child actors.
  It is not safely represented by a convenient empty component mirror.

This makes the first crash cause much stronger than a generic schema concern:
the probe Blueprint was authored and cooked against a parent CDO that lacked
the native default-subobject graph expected by the runtime parent. The child
vehicle pawn may still be independently unsafe in preview, but it is no longer
necessary to invoke it to explain the crash.

The auto-weld mechanism is also narrower than the earlier hypothesis:

- `UVoyageDynamicCollisionComponent` is a passive `0xC8` actor component. Its
  vtable has no component lifecycle overrides beyond destruction; the two
  booleans are data read by another owner;
- reflected `ActorHasAutoweld` and `AttachMeshComponentsFromActor*` belong to
  `VoyageEffectSubsystem`, not to `VoyageModuleActor` or the dynamic-collision
  component itself;
- the effect subsystem owns `DynamicCollisionComponents`,
  `AutoweldingActors`, `AutoweldPrimitives`, weld/unweld effects, and the
  auto-weld timer, movement threshold, collision-normal threshold, and weld
  chain behavior;
- one placement/runtime path checks the actor's runtime `Detachable` tag,
  registers the actor hierarchy with the effect subsystem, recursively walks
  attached child actors/components, and uses the internal `WeldBase`,
  `WeldChain`, `NoWeldChain`, and `AutoWelded` tags;
- `bAutoWeld` gates an auto-weld event inside that registration path. No
  ordinary `AttachToActor`/`AttachToComponent` call was found in this branch.
  The branch aggregates primitive meshes/collision into Voyage's dynamic weld
  system; the flag alone is not an actor-to-ship transform parent operation.

This narrows the pawn-level no-op without requiring another flag guess. The
probe copied the marker, but the native path shows that registration,
hierarchy/primitive eligibility, and the later auto-weld decision are separate
stages; the probe also did not reproduce Cyclone's native module CDO. Manually
toggling `bAutoweldIgnoreNormal` cannot repair a missing earlier stage and is
therefore not a useful next experiment.

The next runtime package must therefore be a shell-only contract test:

1. reproduce the native `VoyageModuleActor` default-subobject identities and
   its `ModuleComponent` class override closely enough that the generated CDO
   matches the original ownership model;
2. give the shell only direct visual/collision components plus the exact
   `VoyageDynamicCollisionComponent(bAutoWeld=true)` marker;
3. omit `ChildActorComponent`, possession, camera, input, HUD, persistence
   experiments, and firing;
4. first prove that selection/preview no longer crashes, then place a fresh
   shell and test ship translation and pitch/roll;
5. if that exact shell still remains world-fixed, stop package iteration and
   instrument or otherwise observe the `Detachable -> EffectSubsystem ->
   AutoWelded` registration result. Do not add runtime weld tags by hand before
   proving which producer failed.

### Shell-only native-contract probe installed

The isolated shell experiment was implemented against the unchanged gated
build fingerprint (`23962331`, executable SHA-256
`6A9AE86E5CE5D7D1B6555F579091AAB1E0E67FF7A96276FA2570052F99102E8D`).
The editor-only `VoyageModuleActor` mirror now reproduces its four mapped own
fields in order, keeps `bCollectable=false`, and creates these exact native
default-subobject names through `FObjectInitializer`:

- `ModuleComponent`;
- `PersistentComponent`;
- `DestructibleObjectComponent`.

The generated Cyclone replacement writes an
`FBPComponentClassOverride(ModuleComponent, VoyageCustomModuleComponent)`,
matching the original turbine ownership model. The shell contains direct
collision, base, yaw, pitch, turret, barrel, and
`VoyageDynamicCollisionComponent(bAutoWeld=true)` components. It contains no
`ChildActorComponent` and has no reference to `BP_HarpoonCannon`; the separate
pawn package remains in the exact three-asset inventory only as a future asset,
not as part of the shell hierarchy.

Native build, commandlet generation, one-process three-package cook,
`retoc to-zen`, `retoc verify`, and exact inventory validation all succeeded.
The cooked shell string inventory confirms the module actor, custom module,
persistent, destructible, dynamic-collision, and direct visual component
identities while finding neither child-actor nor cannon-pawn identities.

Prepared package:

- ignored root:
  `artifacts/harpoon-cannon/shell-only-native-contract-package-01/package`;
- `.pak`: `75E7144577253917F6DA7312EF5E585B12FB728226A22B0938323751A6B555CD`;
- `.ucas`: `5D054BACF6049C2D2873C76CFD315BFE756C25FDCF900C3EC64592F565C9D06A`;
- `.utoc`: `E1ECFD41909B56C87C1A2A6E7FC0E7304F28F6249A982E90607CE94C93C22A85`.

Before installation the game was confirmed closed. The user had manually
removed the previous HarpoonCannon container so he could keep playing while
repository-only work continued; only the independently owned DonkLift container
was present. Therefore there was no same-name installed package to back up or
overwrite. The three new files were copied as new files with fail-closed source
hash checks and post-copy SHA-256 readback. Installed hashes match the prepared
package exactly, and no DonkLift file was touched.

The active game test is deliberately narrow:

1. select Cyclone in construction and report immediately whether preview still
   crashes;
2. if preview loads, confirm the base/turret/barrel placeholder is visible;
3. place a fresh object on the ship, move the ship translationally, and observe
   whether the object keeps its deck-relative position;
4. observe ship pitch/roll or wave motion and report whether the object follows
   the deck orientation without visually hovering.

Do not interpret this package as an entry, camera, HUD, aiming, persistence, or
firing test. If the shell loads but remains world-fixed, the next step is the
already-identified runtime registration observation rather than another flag
guess.

### Shell-only native-contract game result

The installed shell-only package no longer crashes in Cyclone construction and
the completed object follows the ship deck correctly. This game-validates the
core stationary attachment boundary: a `VoyageModuleActor` with the native
named default-subobject graph, custom-module class override, direct primitive
hierarchy, and dynamic-collision marker enters the carrier-relative runtime
attachment lifecycle after construction.

Three construction-lifecycle defects remain and must be treated separately
from completed-object attachment:

- while construction is in progress, the object visibly floats above the deck
  as though it is not attached to the carrier preview;
- a completed HarpoonCannon cannot be dismantled;
- an unfinished HarpoonCannon construction cannot be cancelled.

The latter two failures affect both completed and incomplete construction
states, so they are stronger evidence for a missing generic construction or
destruction identity/configuration contract than for a mesh collision issue.
Do not alter auto-weld or the now-validated completed-object attachment path to
address them. The next static comparison must isolate the original Cyclone's
preview owner, cancellation action provider, dismantle/destruction component
configuration, item identity, and any interface or tag consumed by those
systems.

### Construction and dismantling item-identity candidate

Static comparison isolated one missing contract shared by both failed actions:
the generated shell's inherited `VoyageCustomModuleComponent.ItemAsset` was
empty, while the original Cyclone component points to
`/Game/Data/Assets/Modules/DA_Item_Module_WindTurbineMedium`. The stock
maintenance dismantle ability asks the target for its `VoyageItem` through
`VoyageItemInterface.GetItem()` before recycler validation. The Cyclone item
also explicitly has `bIsDismantlable=true`, an 8-second duration, and a
15-electricity cost. This makes missing item identity the smallest common
explanation for the absent completed-object dismantle action and the absent
unfinished-construction cancel action. It may also affect construction-phase
carrier registration, but that part remains a hypothesis until tested.

The first item-reference candidate was rejected before installation. Although
it compiled, cooked, packaged, and passed container verification, CDO
inspection decoded the serialized value as an empty `MaxResources` override
rather than `ItemAsset`. The editor mirror had omitted the 20 own fields of
`VoyageCustomModuleComponent`, shifting the inherited unversioned property
index. The rejected package remains only under ignored
`artifacts/harpoon-cannon/item-identity-candidate-package-01` and must never be
installed.

The corrected mirror now reproduces the exact 20-property derived-class prefix
and the exact eight-property `VoyageModuleComponent` prefix through
`ItemAsset`. Container inner types are editor-only alignment placeholders and
none of those derived fields is serialized. A project-local `VoyageItem` stub
at the exact Cyclone item path lets the generator author the reference; that
stub is deliberately excluded from cook and package so the game supplies the
real item asset and its dismantle settings.

The aligned candidate passed all local gates on 2026-08-30 Pacific time:

- native/UHT build and commandlet generation succeeded with `0 errors / 0
  warnings`;
- one-process exact three-package cook, `retoc to-zen`, `retoc verify`, and
  exact inventory validation succeeded;
- isolated CDO inspection with a freshly extracted base-game Cyclone item
  resolves `ModuleComponent.ItemAsset` to
  `VoyageItem'DA_Item_Module_WindTurbineMedium'` at
  `/Game/Data/Assets/Modules/DA_Item_Module_WindTurbineMedium.0`;
- no `MaxResources` or other accidental derived-property override is present;
- the editor-only item stub is absent from the three-asset player container;
- the shell still contains no `ChildActorComponent` or `BP_HarpoonCannon`
  reference, preserving the game-validated completed attachment boundary.

Prepared but not installed candidate:

- ignored root:
  `artifacts/harpoon-cannon/item-identity-aligned-package-01/package`;
- `.pak`: `75E7144577253917F6DA7312EF5E585B12FB728226A22B0938323751A6B555CD`;
- `.ucas`: `7EF20DADA5DBAF218A199133C2C3143DB7295E4964C45AF8CC26902C14C3A8B7`;
- `.utoc`: `61DF9B27418461E6221A36CD931B174FA0BDEBE66DEA27A3CD0CFB5AF941DFFA`.

The next game experiment changes only item identity. Rebuild a fresh object and
check, separately: construction-phase deck following, cancellation before
completion, dismantling after completion, and completed-object deck following.

The user explicitly confirmed that Voyage was closed. Pre-installation checks
then confirmed the gated game fingerprint and all three installed shell-only
baseline hashes. Those files were backed up under ignored
`artifacts/harpoon-cannon/installed-backups/before-item-identity-20260830-001421`
and replaced with the aligned candidate. Post-copy SHA-256 readback matches the
three prepared hashes above; rollback was not needed and no DonkLift file was
touched.

This checkpoint is now installed for the four-part game test above. It does not
yet validate that item identity fixes any of those behaviors.

### Item-identity game result

The installed aligned candidate was tested in game. Three of the four expected
behaviors now work:

- the unfinished object follows the moving/rocking ship deck;
- the completed object can be dismantled;
- the completed object retains the previously validated deck attachment.

This game-validates `VoyageCustomModuleComponent.ItemAsset` as part of both the
construction-phase carrier registration and completed-object dismantle
contracts. It also confirms that the aligned unversioned property mirror lands
on the intended runtime field.

Cancellation remains absent: approaching the unfinished HarpoonCannon does not
show the stock `Q Cancel (Self)` action that appears on other fabricating
objects. Therefore cancellation has a separate producer/target-acquisition
contract. The next static comparison must isolate incomplete-object collision
responses, interaction components/interfaces, and the provider that contributes
the self-cancel action. Do not modify the validated item identity or attachment
hierarchy while testing this path.

Static comparison supports one narrow collision hypothesis. The character-side
`InteractiveDetectorComponent` owns an `InteractCollision` primitive and native
`InteractBeginOverlap`/`InteractEndOverlap` handlers. The Harpoon shell has one
query-capable `ModuleMountCollision`, but it explicitly disables generated
overlaps; all three visual primitives also disable them. This is consistent
with line/tool traces finding the completed object for dismantling while the
proximity action provider never acquires the unfinished object. The original
Cyclone has overlapping-capable collision-bearing primitives in its inherited
small-turbine graph. Its separate `InteractiveConditionInterface` override only
special-cases dev-only PartId 99 and is not a good first cancellation fix.

The next candidate therefore changes exactly one runtime property:
`ModuleMountCollision.bGenerateOverlapEvents=true`. It retains the same box
extent, `BlockAllDynamic` profile, item identity, native module shell, and
attachment hierarchy. If `Q Cancel (Self)` appears, overlap acquisition was the
missing producer boundary. If it remains absent, do not add interfaces blindly;
inspect the detector's channel/filter and the generic self-cancel action owner.

The overlap candidate passed its local gate on 2026-08-30 Pacific time:

- native build, commandlet generation, one-process exact cook, packaging, and
  `retoc verify` succeeded;
- cooked CDO comparison shows the old explicit
  `ModuleMountCollision.bGenerateOverlapEvents=false` override is absent, so
  the primitive uses the native `true` default;
- the mount retains `QueryAndPhysics`, `BlockAllDynamic`, the same extent and
  transform, and the exact Cyclone `ItemAsset` reference;
- exact inventory remains three assets and excludes the editor-only item stub.

Prepared package:

- ignored root:
  `artifacts/harpoon-cannon/cancel-overlap-package-01/package`;
- `.pak`: `75E7144577253917F6DA7312EF5E585B12FB728226A22B0938323751A6B555CD`;
- `.ucas`: `C8A4EC7D56E18677AB08EBD4B3FF3FD242118A01850DC017A6A67E946A4C8B86`;
- `.utoc`: `CC0062581BAC0E89E7CCA65AF149DCBC9273E6B79897EFFD1F2513FABE146EFC`.

Installation remains gated on explicit confirmation that Voyage is closed.

### Migration from Cyclone to Whisper test identity

The user closed Voyage and requested that the complete disposable test adapter
move from Cyclone to Whisper. Static inspection confirms that Whisper is the
same relevant module family rather than a new architecture:

1. `DA_Skill_Engineering_Whisper_Wind_Turbine` unlocks
   `/Game/Data/Assets/Modules/DA_Item_Module_WindTurbine_Small`.
2. That item is a dismantlable `Module` item and its `DroppedActor` is
   `/Game/Blueprints/Modules/Generators/BP_Module_WindTurbine_Small`.
3. The actor derives directly from `/Script/Voyage.VoyageModuleActor` and is
   the reusable small-turbine parent already identified beneath Cyclone.

The generator, narrow cook inventory, and packaging contract now target those
two exact Whisper identities. The validated shell hierarchy, exact
`VoyageCustomModuleComponent.ItemAsset` alignment, and the single active
`ModuleMountCollision.bGenerateOverlapEvents=true` cancellation hypothesis are
unchanged. Whisper's own recipe and dismantle duration differ from Cyclone's,
but remain base-game item data and are not authored by the probe.

Historical Cyclone results above remain evidence for how the shell contract was
found; they must not be renamed retroactively. Once a Whisper-only container
replaces the installed Harpoon container, the medium-turbine actor package is
no longer overridden, so Cyclone should resolve to its original game asset.
Whisper then becomes the only construction-menu entry expected to instantiate
the Harpoon shell.

The Whisper overlap candidate passed the complete local gate on 2026-08-30
Pacific time:

- current fingerprint remained Steam build `23962331` with executable SHA-256
  `6A9AE86E5CE5D7D1B6555F579091AAB1E0E67FF7A96276FA2570052F99102E8D`;
- native build and commandlet generation succeeded with `0 errors / 0
  warnings`;
- the one-process exact three-package cook, `retoc to-zen`, `retoc verify`, and
  exact inventory check succeeded;
- inventory contains `BP_Module_WindTurbine_Small`, `BP_HarpoonCannon`, and
  `SM_HarpoonCannonPlaceholder`, with no medium-turbine package or editor-only
  item stub;
- isolated CDO inspection with a freshly extracted base-game Whisper item
  resolves `ModuleComponent.ItemAsset` to
  `/Game/Data/Assets/Modules/DA_Item_Module_WindTurbine_Small.0`;
- the shell parent remains `VoyageModuleActor`, the module override remains
  `VoyageCustomModuleComponent`, the mount remains `QueryAndPhysics` plus
  `BlockAllDynamic`, no explicit `bGenerateOverlapEvents=false` is serialized,
  and no neighboring `MaxResources` override appears.

Prepared package under ignored
`artifacts/harpoon-cannon/whisper-overlap-package-01/package`:

- `.pak`: `75E7144577253917F6DA7312EF5E585B12FB728226A22B0938323751A6B555CD`;
- `.ucas`: `B32DCD872BDD20C08691378AD2AD0CD944FA67536BE4112458F46186EA1EDAB9`;
- `.utoc`: `7915DDF90656FB334844752305524EFD45DFF24302303B56BADB0185C7904E05`.

Immediately before installation, Voyage was still closed, the fingerprint was
unchanged, and all three installed hashes matched the previous
item-identity-aligned Cyclone baseline. That baseline was copied and
hash-verified under ignored
`artifacts/harpoon-cannon/installed-backups/before-whisper-migration-20260830-0109`.
The Whisper candidate then replaced exactly the same three Harpoon container
files. Post-copy SHA-256 readback and a second `retoc verify` match the prepared
package; rollback was not needed, and no DonkLift file was touched.

The migration is installed but still needs this real-game validation:

1. open Cyclone in the construction menu and confirm it shows the original
   medium turbine rather than the Harpoon blockout;
2. open Whisper and confirm it now shows the Harpoon blockout without a crash;
3. place a fresh Whisper/Harpoon construction on the ship and pause it before
   completion;
4. approach it and check whether `Q Cancel (Self)` now appears;
5. quickly confirm unfinished deck following, completed deck following, and
   completed-object dismantling did not regress under Whisper's item identity.

### Rough-model visual checkpoint 1: prepared, installed, then rejected

A separate modeling task delivered the user-approved rough HarpoonCannon
direction as mod-authored OBJ source. The accepted source is now preserved
under `mods/HarpoonCannon/SourceAssets/` with these SHA-256 hashes:

- OBJ: `AADED3621C92225D5F4C6FC794C61B207FF3C8A5D1646077557E9AC97E1541A1`;
- MTL: `74B1F2434337D7AAA6B11EAAE83D24F518DC42EB3BE741DFBC0EAA6382BE5C89`;
- handoff JSON:
  `59078ABF3D9821BC927316904462B00DD95DDB8EF3C57F6C65768FC0DF0F4188`.

The generator parses that OBJ rather than importing an opaque cooked snapshot
and creates three mod-owned meshes on the existing visual ownership boundary:

- `SM_HarpoonCannonBase` contains the static deck mount;
- `SM_HarpoonCannonYawAssembly` contains the rotating base, yoke, retained
  mooring-device blockout, fairlead, cable-route cue, control grip, and status
  light;
- `SM_HarpoonCannonPitchAssembly` contains the trunnion, pressure launcher,
  guide, muzzle braces, and sight, with its local origin moved to the intended
  pitch pivot at `(-5, 0, 94)` cm.

The accepted OBJ's orange connector-proxy objects are excluded from all three
player meshes. Current-build extraction freshly confirms the stock identity
`/Game/AssetSets/Sockets/SM_Mooring_CableSocket_Out`. Both the shell and future
operator asset now add a visual-only `HarpoonLoadedConnector` component that
references that exact base-game mesh at relative location `(101, 0, 12)` from
the pitch pivot, scale `(2, 2, 2)`, roll `90`, with `NoCollision` and overlap
generation disabled. A valid editor-only stand-in at the same package identity
exists only to make the narrow cook resolve the hard reference and is excluded
from player inventory; no game-derived mesh is copied into the mod.

This checkpoint does not add cable gameplay. The OBJ cable path remains a
non-functional rough visual cue only; the production cable contract remains
the stock `BP_ModuleCable_Mooring` / `VoyageCableComponent`, and no duplicate
drum was introduced. Material slot names are preserved, but every generated
mesh currently uses Unreal's default surface material. Texturing is therefore
explicitly deferred.

The visual change preserves the validated gameplay shell:

- parent remains `VoyageModuleActor` with the custom-module class override;
- `ModuleComponent.ItemAsset` remains exactly
  `/Game/Data/Assets/Modules/DA_Item_Module_WindTurbine_Small.0`;
- `ModuleMountCollision` remains `QueryAndPhysics`, `BlockAllDynamic`, the same
  extent and transform, with no explicit `bGenerateOverlapEvents=false`;
- persistent, destructible, dynamic-collision, item, placement, cancellation,
  possession, input, camera, HUD, and save logic were not changed for the
  model.

The complete local gate passed on 2026-08-30 Pacific time: native build,
generation, one-process exact five-package cook, `retoc to-zen`, `retoc
verify`, exact inventory, isolated CDO inspection with base-game Whisper item
and stock-head packages, mesh-bound checks, and exclusion of orange proxy
material/geometry. Cooked mod-mesh bounds are consistent with the accepted
248 x 164 x 159 cm assembly after applying the pitch pivot and stock-head
transform.

Prepared package under ignored
`artifacts/harpoon-cannon/whisper-rough-visual-package-01/package`:

- `.pak`: `75E7144577253917F6DA7312EF5E585B12FB728226A22B0938323751A6B555CD`;
- `.ucas`: `18F4DF9BD1759D572D7CE3A22BBAEAB55C281373C9EC19203D08B7D50FC0B642`;
- `.utoc`: `018D1D5DAA1EE4F3FB800409A614F93AB192106FD85C6E748910A27C052CA437`.

The exact player inventory contains the Whisper shell, future cannon pawn, and
the three mod-authored meshes. It excludes the stock-head stand-in, Whisper
item stub, HUD, old placeholder cube, and all native/editor binaries.

The user explicitly confirmed the game was closed. The installer also verified
that no shipping process was running, the game fingerprint was unchanged, and
all three installed hashes matched the previous Whisper overlap baseline. That
baseline was copied and hash-verified under ignored
`artifacts/harpoon-cannon/installed-backups/before-rough-visual-20260830-0140`.
The visual candidate then replaced exactly the three Harpoon container files.
Post-copy SHA-256 readback matches the prepared hashes, and a second `retoc
verify` plus installed exact-five-asset inventory check passed; rollback was
not needed and no DonkLift file was touched.

The first launch with this visual candidate crashed after 36 seconds in the
async loading thread. The current crash context is
`UECC-Windows-ADB08480432481E3444096B896D85B6A_0000`, with `CrashType=Assert`
and this decisive serialization failure while loading the original Cyclone
child package:

`BP_Module_WindTurbine_Medium_New_C ... Bad export index 4607/19`.

This rejects the candidate before any visual validation. The failure is not
evidence about OBJ normals, scale, or model quality: runtime package loading
failed before the construction menu could be tested. At this point the
five-package container still combined three possible variables: additional
mesh packages, a direct base-game stock-head hard reference, and replacement
of Whisper's small-turbine parent package. Static build/cook success, exact
inventory, and `retoc verify` did not prove that the original medium-turbine
child could deserialize against that override.

With Voyage closed, the installed package was restored from the hash-verified
backup at
`artifacts/harpoon-cannon/installed-backups/before-rough-visual-20260830-0140`.
Installed readback now exactly matches the prior Whisper overlap baseline:

- `.pak`: `75E7144577253917F6DA7312EF5E585B12FB728226A22B0938323751A6B555CD`;
- `.ucas`: `B32DCD872BDD20C08691378AD2AD0CD944FA67536BE4112458F46186EA1EDAB9`;
- `.utoc`: `7915DDF90656FB334844752305524EFD45DFF24302303B56BADB0185C7904E05`.

Post-rollback `retoc verify` succeeded and inventory was again exactly the
Whisper shell, `BP_HarpoonCannon`, and `SM_HarpoonCannonPlaceholder`. A second
launch then crashed after 20 seconds with the identical error. Its crash
context is `UECC-Windows-3B9BC63E41E375CB5C21FDA5C956CD23_0000`.

This second result isolates the cause: neither the three added model packages
nor the stock-head reference is required for the failure. Replacing
`BP_Module_WindTurbine_Small` changes the serialized export contract of the
parent package while the original `BP_Module_WindTurbine_Medium_New` child
continues to address its expected parent exports. The Whisper parent-path
adapter is therefore structurally unsafe and must not be retried by changing
mesh inventory or references.

With Voyage stopped, all three exact Harpoon container files were removed from
the game's Paks directory after verifying their baseline hashes. No Harpoon or
`CannonPlacementProbe` file remains installed; the removed bytes are
recoverable from the ignored backup above. The next construction-menu adapter
must override a leaf package with no untouched children, or use a dedicated
Harpoon item/actor identity. Do not attempt to repair this by packaging a
snapshot of the original Cyclone child or reproducing its opaque export index
layout.

### Leaf Cyclone rough-proxy recovery candidate prepared

The recovery returns the disposable adapter to
`/Game/Blueprints/Modules/Generators/BP_Module_WindTurbine_Medium_New`. Current
inventory and the earlier successful shell tests establish this as the leaf
Cyclone actor path; replacing it does not alter an untouched child Blueprint's
parent export contract. The item reference returns to
`/Game/Data/Assets/Modules/DA_Item_Module_WindTurbineMedium.0`.

The validated shell contract is otherwise unchanged:

- parent remains `VoyageModuleActor` with the exact native default-subobject
  names and `VoyageCustomModuleComponent` class override;
- `ModuleMountCollision` remains `QueryAndPhysics`, `BlockAllDynamic`, with no
  explicit `bGenerateOverlapEvents=false`, preserving the active cancellation
  overlap experiment;
- `VoyageDynamicCollisionComponent.bAutoWeld=true` remains present;
- no child actor, possession, input, camera, HUD, trace, firing, or save change
  was introduced.

The first leaf visual checkpoint intentionally excludes the direct hard
reference to `/Game/AssetSets/Sockets/SM_Mooring_CableSocket_Out`. Instead, the
five mod-authored connector-proxy OBJ objects are included in the pitch mesh.
This keeps the accepted silhouette testable while reserving the stock connector
for a later one-variable experiment. Cooked inspection confirms that no
`HarpoonLoadedConnector` component and no stock-head package identity is
serialized.

The complete local gate passed on 2026-08-30 Pacific time:

- the installed fingerprint still matches Steam build `23962331` and executable
  SHA-256
  `6A9AE86E5CE5D7D1B6555F579091AAB1E0E67FF7A96276FA2570052F99102E8D`;
- UBT rebuilt the editor-only generator successfully;
- commandlet generation completed with `0 error(s), 0 warning(s)`;
- one-process exact five-package cook and IoStore build succeeded;
- `retoc verify` succeeded and inventory contains only the leaf Cyclone shell,
  `BP_HarpoonCannon`, and the base/yaw/pitch meshes;
- isolated CDO inspection resolves the shell parent to `VoyageModuleActor`,
  `ItemAsset` to the exact Cyclone item, `bAutoWeld=true`, pitch pivot Z to
  `94`, and finds no accidental `MaxResources` override or explicit overlap
  disable;
- no Whisper actor/item identity and no stock-head identity appears in cooked
  files;
- pitch-mesh bounds are origin `(23, 0, 20.5)`, extent `(124, 64, 44.5)`, and
  its material slots include `ConnectorProxy`; its cooked asset grows from the
  earlier proxy-excluding `98,850` bytes to `125,982` bytes.

Prepared package:

- ignored root:
  `artifacts/harpoon-cannon/cyclone-leaf-rough-proxy-01/container/package`;
- `.pak`: `75E7144577253917F6DA7312EF5E585B12FB728226A22B0938323751A6B555CD`;
- `.ucas`: `80956EB71051C7D86B5FC84B840100EDAAA12CE621502D807A24273F2157CAF0`;
- `.utoc`: `FAC4F2319A4AF3A26CD21FB1B9BD85D79A280FA07D9996094F2CE831551CD4F3`.

The user explicitly confirmed Voyage was closed, and the process check found no
shipping executable. No same-name Harpoon file existed before installation, so
the candidate was copied as exactly three new files. Source hashes, post-copy
readback, installed `retoc verify`, and exact five-asset inventory all match the
prepared package above. Automatic cleanup was armed but not needed.

The candidate is now installed. The first runtime check is load only: launch
the game and open Cyclone in the construction menu. Only if that succeeds
should visual scale/normals, unfinished cancellation, deck following, and
dismantling be tested.

### Leaf Cyclone rough-proxy runtime result

The user launched Voyage with the installed candidate, and the game remained
stable. He successfully constructed the object and supplied an in-game
screenshot of the completed HarpoonCannon on the ship deck. This validates the
leaf `BP_Module_WindTurbine_Medium_New` carrier against the earlier parent-path
failure and proves that all three generated base/yaw/pitch mesh packages load
and assemble through the shell.

The visible result contains the intended circular deck mount, rotating yoke,
pressure chamber, linear guide, sights, and connector-proxy region. No large
mesh group is missing, and the visible exterior does not show the systematic
inside-out face culling seen in the original procedural-cube probe. Dark faces
are consistent with the intentionally untextured default material and night
lighting; this screenshot alone does not prove final normals or shading on
every surface. Overall scale appears plausible beside the ship modules but is
not yet user-approved as final.

This result does not yet validate the active overlap hypothesis, deck-relative
motion of this new visual build, dismantling, save/load persistence, pivot
motion, or the future stock connector reference. Keep the installed package
unchanged for those observations. The next checks are:

1. move and rock the ship and confirm the completed object follows the deck;
2. confirm the completed object can still be dismantled;
3. place a fresh object, pause fabrication, approach it, and report whether
   `Q Cancel (Self)` appears;
4. if convenient, save/reload once and confirm the completed object persists.

The user then reported the decisive cancellation result from this unchanged
leaf package: an unfinished HarpoonCannon still cannot be removed and
`Q Cancel (Self)` still does not appear. Therefore
`ModuleMountCollision.bGenerateOverlapEvents=true` is not sufficient to enter
the stock self-cancel action path. Do not respond with another collider-only
change or blind channel/profile guesses. The next static task is to identify
the exact `Cancel (Self)` producer, how it acquires its target, and which
construction identity/interface/state its filter requires while fabrication is
incomplete.

### Exact unfinished-cancellation producer and collision boundary

Current-build Blueprint inspection identifies the stock action owner. Physical
placement does not fabricate the final module directly. `BP_Physical_Fabricator_Factory`
first spawns `/Game/Blueprints/Fabrication/BP_Location_Fabricator`, and that
temporary `VoyageFabricator` actor owns unfinished construction until completion.
Its `GetInteractiveProvidedActions(Character, Component)` implementation:

- returns an `IAV_Drop` action while its `CanCancel` flag is true;
- sets `bAppendTargetName=true`, which accounts for the visible `Cancel (Self)`
  form rather than requiring a Harpoon-specific label;
- binds completion to its own `OnCancel`, which calls
  `VoyageFabricator.CancelAndSpawnItemsInInventory()`, destroys the temporary
  fabricator actor, and updates the task count.

This proves that adding an interaction provider or cancel callback to the future
Harpoon module would target the wrong actor. The missing action is an acquisition
failure on the stock temporary fabricator, not missing cancel logic on the final
module.

`BP_Location_Fabricator.UpdateVisualizationItem` builds the temporary hologram
through `VoyageEffectSubsystem.AttachMeshComponentsFromItemActor`. It then
configures every returned `MeshComponent` for `OverlapAll`, specific channel
responses, begin/end overlap delegates, and generated overlaps. Non-mesh
components are not returned by this path. Consequently the Harpoon shell's
`ModuleMountCollision` box is not part of the unfinished actor even though it is
valid on the completed module.

The current Harpoon visual StaticMeshes were generated with
`FBuildMeshDescriptionsParams.bBuildSimpleCollision=false`. Cooked inspection
shows a `BodySetup` but no serialized `AggGeom` simple shapes on
`SM_HarpoonCannonBase`. The stock Cyclone base mesh, by comparison, serializes
multiple `AggGeom.BoxElems` and uses `CTF_UseSimpleAsComplex`. This is the first
hypothesis that explains both facts at once: the stock fabricator correctly
configures the cloned Harpoon mesh components, but those components have no
simple collision geometry on which its overlap/interaction acquisition can
operate.

The next discriminating package must change only collision data on one cloned
mesh. Add deterministic simple collision to `SM_HarpoonCannonBase` while
leaving its render vertices, materials, component transform/profile, the
validated module shell, item identity, attachment hierarchy, and the yaw/pitch
meshes unchanged. Do not add another actor interaction component or modify
`BP_Location_Fabricator`. Before runtime installation, cooked inspection must
prove that the base mesh now contains simple `AggGeom` while the other two
meshes do not. Runtime success criterion is specifically appearance and
operation of `Q Cancel (Self)` on a paused unfinished build.

That collision-only candidate is now prepared under ignored
`artifacts/harpoon-cannon/cancel-simple-collision-package-02/package`. The
initial use of `FBuildMeshDescriptionsParams.bBuildSimpleCollision=true` was
rejected before installation because cooked inspection still showed no
`AggGeom`; the option created/retained `BodySetup` but did not author a simple
shape for this generated mesh. The generator now deterministically derives one
axis-aligned box from the base mesh bounds, writes it to
`BodySetup.AggGeom.BoxElems`, and selects `CTF_UseSimpleAsComplex`.

The complete local gate passed:

- current fingerprint remains Steam build `23962331`, executable SHA-256
  `6A9AE86E5CE5D7D1B6555F579091AAB1E0E67FF7A96276FA2570052F99102E8D`;
- UBT build, commandlet generation, exact five-package cook, `retoc to-zen`,
  exact inventory, and `retoc verify` succeeded;
- cooked `SM_HarpoonCannonBase` contains exactly one simple box centered at
  `(0, 0, 6)` with dimensions `164 x 164 x 12` cm and
  `CTF_UseSimpleAsComplex`;
- cooked yaw and pitch meshes contain no `AggGeom`, preserving the one-variable
  boundary;
- isolated shell inspection still resolves parent `VoyageModuleActor`, exact
  Cyclone `ItemAsset`, `VoyageDynamicCollisionComponent.bAutoWeld=true`, the
  unchanged visual mesh references, and no neighboring `MaxResources`
  corruption.

Prepared container hashes:

- `.pak`: `75E7144577253917F6DA7312EF5E585B12FB728226A22B0938323751A6B555CD`;
- `.ucas`: `25C5CD17F7B12F6C3CAF06DD3A514E05743143F234A387AF92ACF042C910330B`;
- `.utoc`: `AEBA17E558F3209F095F663B3C372EEA9AA3E96AE43E8CE3A543B074F10E9429`.

The game process was absent immediately before installation. All three
installed files matched the unchanged leaf rough-proxy baseline and were copied
to the hash-verified ignored backup
`artifacts/harpoon-cannon/installed-backups/before-cancel-simple-collision-20260830-0322`.
The collision-only candidate then replaced exactly those three files. Installed
SHA-256 readback matches the prepared hashes above and installed `retoc verify`
succeeded; rollback was not needed.

The active runtime test is now only:

1. place a fresh Cyclone/Harpoon build and pause it before completion;
2. approach its base and verify whether `Q Cancel (Self)` appears and actually
   cancels/refunds the unfinished fabrication;
3. record whether the unfinished hologram remains a dense point/triangle grid
   or becomes a normal solid Voyage preview.

If Q still does not appear, keep this result separate from preview rendering and
inspect which cloned component the character detector selects. Do not add more
collision shapes or interaction components in the same follow-up.

### Unfinished cancellation game-validates simple mesh collision

The user tested a fresh unfinished HarpoonCannon with the installed
collision-only candidate and confirmed that `Q Cancel (Self)` now appears and
works. This game-validates the complete current-build cancellation chain:

1. `BP_Location_Fabricator`, not the future module actor, owns the unfinished
   self-cancel action and its refund/destruction behavior;
2. it clones mesh components from the item's dropped actor and configures those
   clones for its overlap/interaction channels;
3. the Harpoon shell's separate `ModuleMountCollision` box is not part of that
   cloned visualization, so changing its overlap flag was necessarily a no-op;
4. one simple collision box on the cloned `SM_HarpoonCannonBase` supplies the
   missing acquisition geometry without a custom action provider, interaction
   component, or modification of the stock fabricator.

Keep the base-mesh simple collision as a construction-lifecycle contract. Do
not remove it during later model revisions, and do not reintroduce the rejected
mount-overlap or custom-cancel approaches.

A fresh screenshot of the same collision-only package confirms that the
unfinished hologram remains almost entirely a green point/triangle topology
without filled surfaces or a solid silhouette. The base mesh with simple
collision is visually indistinguishable from the yaw/pitch meshes without
`AggGeom`. Therefore simple collision is sufficient for Q acquisition but
irrelevant to preview shading. Do not vary collision or overlap again for that
visual defect; the model branch owns a separate render-path experiment.

The user also reported that the current unfinished hologram is rendered as a
dense glowing point/triangle network rather than the usual solid Voyage
construction preview. Record whether this changes under the collision-only
candidate, but do not mix a normals/hard-edges change into the Q experiment.

### Enter/exit operator checkpoint prepared

The next logic-only checkpoint restores only external interaction, possession
entry, and a provisional direct exit marker. Camera activation, HUD selection,
mouse aim, optical trace, shark classification, distance display, firing, and
all model changes remain intentionally unbound.

Fresh inspection of the current-build stock forklift established the exact
external entry contract:

- a separate `UBoxComponent` uses the `Interactive` profile, query-only
  collision, object channel `GameTraceChannel2`, and blocks only the `Interact`
  trace channel; it does not generate overlaps;
- an `InteractiveObjectComponent` and the actor's implementation of
  `/Script/Voyage.InteractiveInterface` own the interaction identity;
- `InteractAction(MyCharacter, PartId, InteractIndex)` rejects an invalid
  character, requires part/index zero, obtains the character controller, casts
  it to `/Script/Voyage.VoyagePlayerController`, and calls inherited
  `/Script/Voyage.VoyageVehiclePawn.OnEnterVehicle`;
- the stock Blueprint's `GetInteractiveProvidedActions` returns false, so no
  custom external action-array producer is required for this checkpoint.

The stock forklift's `ExitAction` is a property of the derived native
`VoyageVehicleForkliftPawn`, not of base `VoyageVehiclePawn`. Therefore this
checkpoint deliberately binds a direct `E` key event to inherited
`OnExitVehicle()` as a visible possession-lifecycle marker. This is not yet the
final rebind-aware Voyage input/provider contract and must be replaced or
validated separately after basic entry/exit works.

The operator lifecycle avoids the previously crashing serialized
`ChildActorComponent` path. The game-validated module shell keeps sole ownership
of placement, attachment, visuals, item identity, persistence, dismantling, and
construction cancellation. Its tick checks for the runtime `Fabricated` actor
tag; only the completed shell spawns the invisible `BP_HarpoonCannon` operator,
stores it, attaches it to the shell with snap-to-target rules, and disables the
lifecycle tick. Preview/transient shells without that tag do not create an
operator. Shell destruction destroys a valid operator, while operator
destruction calls `OnExitVehicle()` first.

The operator contains only its scene hierarchy, exact external interaction box
and component, `OperatorCharacterLocation` tagged `ExitComponentTag`, and the
future yaw/pitch/optical-axis/camera scaffold. The optical camera remains
inactive and contributes no view or HUD behavior.

Several unsafe local candidates were rejected before installation:

- the generic graph-node helper called `PostPlacedNewNode()` before allocating
  pins; UE 5.7's `UK2Node_SpawnActorFromClass` immediately requires its scale
  pin and asserted in `EdGraphNode.h:586`. A spawn-specific helper now allocates
  pins before the post-placement callback;
- the first cooked operator used an empty mirror for
  `InteractiveObjectComponent`, whose runtime class has 20 own properties.
  CUE4Parse then read the inherited relative-location payload as an invalid
  boolean, proving the unversioned property indices were shifted. The mirror
  now reproduces the exact 20-property prefix; container inner types and enum
  members are alignment-only because none of those own fields is serialized;
- an editor unknown `Interactive` profile became `Custom`, and deferred profile
  assignment still collapsed to `Custom` during cook. The editor-only project
  now defines the current-build Voyage collision-channel names and exact stock
  interaction profile. These config definitions author the asset but are not
  shipped in the player container.

The corrected candidate passed the complete local gate against unchanged Steam
build `23962331` and executable SHA-256
`6A9AE86E5CE5D7D1B6555F579091AAB1E0E67FF7A96276FA2570052F99102E8D`:

- UHT/native build and commandlet generation succeeded with `0 errors / 0
  warnings`;
- one-process exact five-package cook, `retoc to-zen`, exact inventory, and
  `retoc verify` succeeded;
- cooked operator inspection resolves parent `VoyageVehiclePawn`, implemented
  `InteractiveInterface`, exact `InteractiveObjectComponent`, stock
  `Interactive` profile with object channel `GameTraceChannel2`, the
  `ExitComponentTag`, `OnEnterVehicle`, `OnExitVehicle`, and the direct `E`
  binding;
- decompiled operator bytecode contains the expected character-validity,
  part/index, controller-cast, enter, exit, and destroy-cleanup branches;
- decompiled shell bytecode contains the `Fabricated` gate, deferred spawn,
  stored operator reference, shell attachment, one-shot tick disable, and
  destroy cleanup;
- shell inspection still resolves the exact Cyclone `ItemAsset`,
  `VoyageDynamicCollisionComponent.bAutoWeld=true`, unchanged three visual
  component identities, and no neighboring `MaxResources` corruption;
- the base mesh still contains the game-validated `164 x 164 x 12` cm simple
  collision box and `CTF_UseSimpleAsComplex`, preserving unfinished Q cancel.

Prepared package:

- ignored root: `artifacts/harpoon-cannon/enter-exit-package-05/package`;
- `.pak`: `75E7144577253917F6DA7312EF5E585B12FB728226A22B0938323751A6B555CD`;
- `.ucas`: `2B83F121ABDF4F576BC608133517310FDC8058089AFFBE9C331AC4C1D3142E98`;
- `.utoc`: `756450ACD97E89F433C6BE7BAD04D6FD4F55A703A0F03ABA2E7CEC836D124690`.

The first runtime test must remain narrow: confirm startup/construction do not
regress, build a fresh object, approach it after completion, verify that the
standard external `E` interaction appears and enters the operator, then press
`E` again and verify exit returns control to the character. Do not interpret
unchanged third-person/default camera or absent cannon HUD as a failure of this
checkpoint; those layers are intentionally deferred. Installation remains
gated on explicit confirmation that Voyage is closed and a process-absent
readback.

The user then confirmed that Voyage was closed. A fresh process check found no
shipping executable, and the game fingerprint remained Steam build `23962331`
with executable SHA-256
`6A9AE86E5CE5D7D1B6555F579091AAB1E0E67FF7A96276FA2570052F99102E8D`.
All three installed files still matched the game-validated unfinished-cancel
baseline and were copied to the hash-verified ignored backup
`artifacts/harpoon-cannon/installed-backups/before-enter-exit-20260830-185222`.

The enter/exit candidate then replaced exactly the three Harpoon container
files. Automatic rollback was armed for any copy or readback failure; no
rollback was needed. Installed SHA-256 readback matches the prepared candidate:

- `.pak`: `75E7144577253917F6DA7312EF5E585B12FB728226A22B0938323751A6B555CD`;
- `.ucas`: `2B83F121ABDF4F576BC608133517310FDC8058089AFFBE9C331AC4C1D3142E98`;
- `.utoc`: `756450ACD97E89F433C6BE7BAD04D6FD4F55A703A0F03ABA2E7CEC836D124690`.

Installed `retoc verify` succeeded. The installed semantic inventory contains
exactly the leaf Cyclone shell, `BP_HarpoonCannon`, and the unchanged
base/yaw/pitch meshes. The package is now awaiting the narrow real-game
entry/exit test above; no runtime behavior from this checkpoint is yet
validated.

### Enter/exit operator checkpoint rejected at fabrication completion

The first runtime test rejected package-05 before external interaction could
be exercised. Voyage remained stable through construction, then crashed at the
instant fabrication completed, which is exactly when the new shell graph first
passes its `Fabricated` tag gate and attempts to create the operator pawn.

Fresh crash context
`UECC-Windows-1FDF13264958A56279B9599ECAEDB6A2_0000` reports a GameThread
`EXCEPTION_ACCESS_VIOLATION` reading null after 148 seconds. The shipping stack
has addresses but no symbols, so this result proves the completion-time
operator path is live and unsafe but does not yet distinguish deferred spawn,
finish-spawn, vehicle-pawn initialization, stored-reference assignment, or
shell attachment as the failing operation.

Do not extend this candidate with camera, HUD, input, aiming, or visual changes,
and do not retry the same spawn graph. Restore the game-validated simple-
collision/Q-cancel baseline as soon as the shipping process is absent. The next
logic investigation must split operator class construction from shell-driven
runtime spawn with a cheaper marker or a stock lifecycle producer before
another package is installed.

The first rollback attempt correctly stopped because a new responding Voyage
process had started after the crashed process. After the user exited that
second process, a fresh process check was absent. The installed package-05
hashes and all three files in
`artifacts/harpoon-cannon/installed-backups/before-enter-exit-20260830-185222`
matched their recorded values before replacement.

The backup then restored exactly the three installed Harpoon files. Automatic
recovery back to the rejected package was armed for any copy or readback
failure; it was not needed. Installed readback is again the game-validated
simple-collision/Q-cancel baseline:

- `.pak`: `75E7144577253917F6DA7312EF5E585B12FB728226A22B0938323751A6B555CD`;
- `.ucas`: `25C5CD17F7B12F6C3CAF06DD3A514E05743143F234A387AF92ACF042C910330B`;
- `.utoc`: `AEBA17E558F3209F095F663B3C372EEA9AA3E96AE43E8CE3A543B074F10E9429`.

Post-rollback `retoc verify` succeeded and the semantic inventory is again
exactly the leaf Cyclone shell, the inert future `BP_HarpoonCannon` asset, and
the unchanged base/yaw/pitch meshes. The installed game is safe for normal use;
entry/exit remains unimplemented.

### Native Actor completion-lifecycle split prepared

The next candidate isolates whether package-05 failed because the shell-driven
completion lifecycle is unsafe in general or because constructing the mirrored
`VoyageVehiclePawn` child is unsafe. It preserves the same `Fabricated` tag
gate, deferred `SpawnActor`, store, snap-to-shell attachment, one-shot tick
disable, and shell-destruction cleanup graph, but changes the spawned class and
stored-reference type to the native `/Script/Engine.Actor`. The generated
`BP_HarpoonCannon` remains in the exact inventory but is not referenced or
constructed by the shell.

This is intentionally not an entry/exit candidate. It has no visible marker and
adds no interaction, possession, input, camera, HUD, aiming, trace, firing, or
model behavior. Its discriminating runtime outcomes are:

1. construction completes without a crash: the shell completion-time
   spawn/store/attach lifecycle is viable, localizing package-05 to
   `BP_HarpoonCannon` / `VoyageVehiclePawn` construction or initialization;
2. construction crashes again at completion: the shell-driven spawn/attach
   lifecycle itself is unsafe and must be replaced before any vehicle class is
   investigated;
3. an earlier startup, preview, cancellation, attachment, or dismantle
   regression rejects the candidate independently of this split.

The marker passed the local gate against unchanged Steam build `23962331` and
executable SHA-256
`6A9AE86E5CE5D7D1B6555F579091AAB1E0E67FF7A96276FA2570052F99102E8D`:

- UBT/UHT and commandlet generation succeeded; the clean commandlet run
  completed with `0 errors / 0 warnings`;
- one-process exact five-package cook, IoStore build, exact inventory, and
  `retoc verify` succeeded;
- decompiled shell bytecode resolves the spawn class to
  `/Script/Engine.Actor`, stores `AActor* OperatorActor`, attaches that result
  to the shell, disables lifecycle tick, and destroys a valid marker with the
  shell; it contains no shell reference to `BP_HarpoonCannon`;
- isolated CDO inspection still resolves the exact Cyclone `ItemAsset`,
  `VoyageDynamicCollisionComponent.bAutoWeld=true`, and all three unchanged
  visual component identities, with no accidental `MaxResources` override;
- cooked base mesh still contains exactly one `164 x 164 x 12` cm simple box
  centered at `(0, 0, 6)` with `CTF_UseSimpleAsComplex`, preserving the
  game-validated unfinished-cancel acquisition contract.

Prepared package:

- ignored root:
  `artifacts/harpoon-cannon/native-actor-marker-01/container/package`;
- `.pak`: `75E7144577253917F6DA7312EF5E585B12FB728226A22B0938323751A6B555CD`;
- `.ucas`: `8024F6C5D37D5E8FDFE11C8013236FC6EA96DA0C4110EDD774A8220002D47A5E`;
- `.utoc`: `9F115A5407D535CBEA037CF741EE4928F17C3E329F7484322F01C49A1444C84A`.

Installation remains gated on explicit confirmation that Voyage is closed and
a fresh process-absent readback. The runtime test needs only a fresh completed
construction plus a short stability observation; no `E` interaction is
expected from this marker.

The user explicitly confirmed that Voyage was closed. A fresh process check was
absent, the fingerprint remained unchanged, and all three installed files still
matched the game-validated simple-collision/Q-cancel baseline. That baseline
was copied and hash-verified under ignored
`artifacts/harpoon-cannon/installed-backups/before-native-actor-marker-20260830-192945`.

The marker then replaced exactly the three installed Harpoon files. Automatic
rollback was armed for any copy or readback failure; it was not needed.
Installed SHA-256 readback matches the prepared candidate, installed `retoc
verify` succeeded, and the semantic inventory is exactly the expected five
packages. The active runtime test is now only: construct a fresh object, allow
fabrication to complete, and observe whether Voyage remains stable for a short
period. No external `E` interaction is expected from this marker.

### Native Actor completion lifecycle game-validates

The user completed a fresh construction with the installed native-Actor marker
and reported that Voyage remained stable. This game-validates the shell's
completion-time `Fabricated -> deferred spawn -> finish spawn -> store ->
attach -> disable tick` lifecycle with a native `AActor` result. It also proves
that the completion tag gate and shell event graph execute without requiring a
Voyage vehicle class.

Combined with package-05's crash at the same completion boundary, this
localizes the unsafe variable to construction or initialization of the
generated `BP_HarpoonCannon` / `VoyageVehiclePawn` operator. The shell-driven
spawn/attach architecture itself is no longer the leading fault, and changing
its timing, attachment rules, owner, or cleanup would not be a discriminating
next experiment.

The marker has no entry behavior. After the user exited, it was removed through
the game-validated Q-cancel baseline before the next logic experiment was
installed. The next static split must
separate the native `VoyageVehiclePawn` construction contract from the
generated operator Blueprint's SCS components, interface, and event graph; do
not add camera, HUD, input, aiming, trace, or visual variables yet.

### Minimal VoyageVehiclePawn Blueprint split prepared

The next one-variable candidate is prepared but not installed. Spawning the
native `/Script/Voyage.VoyageVehiclePawn` class directly was rejected as a
diagnostic because an abstract or otherwise non-spawnable runtime class could
return null and make the shell's following finish/attach operations fail for a
reason different from the package-05 crash.

Instead, the shell keeps the exact game-validated completion lifecycle and
spawns a generated `BP_HarpoonCannon` whose parent is the exact editor mirror
of `/Script/Voyage.VoyageVehiclePawn`, but whose generated content stops before
all Harpoon-specific SCS, interface, and event construction. Cooked inspection
shows only the automatically created `DefaultSceneRoot`; there is no external
interaction box or component, `InteractiveInterface`, optical hierarchy,
camera, axis pivots, event graph, or ubergraph.

The discriminating runtime outcomes are:

1. construction crashes at completion: the leading cause becomes the
   incomplete or mismatched native `VoyageVehiclePawn` CDO/default-subobject
   contract in the editor mirror;
2. construction completes and remains stable: the parent can construct, and
   package-05's fault lies in one of the removed custom SCS, interface, or
   event layers, which can then be restored incrementally;
3. any earlier construction, cancellation, attachment, or dismantling
   regression rejects the package independently of the operator split.

The candidate passed its local gate against the unchanged fingerprint:

- UBT/UHT and clean commandlet generation succeeded with `0 errors / 0
  warnings`;
- one-process exact five-package cook, IoStore build, exact inventory, and
  `retoc verify` succeeded;
- decompiled shell bytecode still spawns
  `/Game/Mods/HarpoonCannon/BP_HarpoonCannon.BP_HarpoonCannon_C` and preserves
  the validated finish/store/snap-attach/tick-disable/cleanup graph;
- decompiled operator output contains only a `VoyageVehiclePawn` parent,
  `SimpleConstructionScript`, and `DefaultSceneRoot`;
- isolated shell inspection preserves the exact Cyclone `ItemAsset`,
  `VoyageDynamicCollisionComponent.bAutoWeld=true`, and the unchanged three
  visual mesh components;
- isolated base-mesh inspection confirms exactly one simple collision box
  centered at `(0, 0, 6)`, sized `164 x 164 x 12` cm, with
  `CTF_UseSimpleAsComplex`, preserving the game-validated unfinished-cancel
  acquisition contract.

Prepared container:

- ignored root:
  `artifacts/harpoon-cannon/minimal-vehicle-marker-01/container/package`;
- `.pak`: `75E7144577253917F6DA7312EF5E585B12FB728226A22B0938323751A6B555CD`;
- `.ucas`: `D1F364E9313E773ACCF1AF3DFA0D4A5FFBB13E6A1ACC9A97C9507C807610DB65`;
- `.utoc`: `F1EF7B89473FB089B4F2EE95458A7BF75DF45BF0EE4CF922597FCD606D2FA7DA`.

The exact inventory contains the leaf Cyclone shell, minimal
`BP_HarpoonCannon`, and the unchanged base/yaw/pitch meshes.

The user then confirmed that Voyage was closed, and a fresh process check found
no shipping executable. Installed hashes still matched the game-validated
native-Actor marker. Before changing it, those three files were copied and
hash-verified under
`artifacts/harpoon-cannon/installed-backups/before-minimal-vehicle-marker-20260830-195403/native-actor-marker`.

The installer next restored the Q-cancel baseline from its earlier verified
backup, confirmed all three baseline hashes, and made a second fresh copy under
`artifacts/harpoon-cannon/installed-backups/before-minimal-vehicle-marker-20260830-195403/q-cancel-baseline`.
Only after that intermediate recovery gate did it install the minimal vehicle
candidate. Automatic failure recovery targeted that fresh Q baseline; rollback
was not needed.

Installed SHA-256 readback now matches the prepared candidate:

- `.pak`: `75E7144577253917F6DA7312EF5E585B12FB728226A22B0938323751A6B555CD`;
- `.ucas`: `D1F364E9313E773ACCF1AF3DFA0D4A5FFBB13E6A1ACC9A97C9507C807610DB65`;
- `.utoc`: `F1EF7B89473FB089B4F2EE95458A7BF75DF45BF0EE4CF922597FCD606D2FA7DA`.

Installed `retoc verify` succeeded and semantic inventory is exactly the five
expected packages. The active runtime test is only: construct a fresh
HarpoonCannon to completion and observe stability for 10-15 seconds. No `E`
interaction, possession, camera, HUD, aiming, trace, or firing behavior is
expected from this minimal marker.

### Minimal VoyageVehiclePawn Blueprint game-validates

The user completed a fresh HarpoonCannon construction with the installed
minimal Blueprint marker and reported that Voyage remained stable. This proves
that a generated Blueprint derived from the current editor mirror of
`VoyageVehiclePawn` can be constructed and initialized by the validated shell
lifecycle. The runtime native parent/CDO is therefore not independently
sufficient to explain package-05's completion-time crash.

Combined with the native-Actor result, the crash is now localized to at least
one layer removed from the full operator: its custom SCS components, exact
interaction component/interface, optical camera, or event graph. Do not spend
the next experiment reconstructing all native `VoyageVehiclePawn` default
subobjects, and do not alter the validated shell spawn/attach lifecycle. Split
the removed generated layers incrementally.

The minimal marker remains temporarily installed while Voyage is running. It
has no entry, possession, camera, HUD, aim, trace, or firing behavior. Restore
the Q-cancel baseline after the user exits before installing the next split.

### Passive transform-only operator split prepared

The next prepared candidate adds only an engine-native transform hierarchy to
the game-validated minimal vehicle Blueprint:

`MountRoot -> YawPivot -> PitchPivot -> OpticalAxis`.

All four nodes are plain `USceneComponent` instances. The intended pitch and
optical-axis relative locations are serialized, but there is no interaction
box, `InteractiveObjectComponent`, `InteractiveInterface`, character/exit
marker, camera, event graph, ubergraph, input, HUD, trace, firing, or model
component on the operator. The shell still uses the same validated completion-
time spawn/store/snap-attach/tick-disable/cleanup graph.

This split answers only whether a custom passive SCS hierarchy is safe on the
generated vehicle operator:

1. stable construction localizes package-05 further to interaction, camera, or
   event-graph layers;
2. a completion-time crash identifies custom SCS/root construction itself as
   unsafe and requires comparison with the runtime native scene-root graph
   before adding any gameplay component.

The candidate passed its complete local gate against the unchanged current-
build fingerprint:

- UBT/UHT and commandlet generation succeeded with `0 errors / 0 warnings`;
- one-process exact five-package cook, IoStore build, exact inventory, and
  `retoc verify` succeeded;
- cooked operator pseudocode derives from `VoyageVehiclePawn` and exposes only
  `MountRoot`, `YawPivot`, `PitchPivot`, and `OpticalAxis` plus its
  `SimpleConstructionScript`;
- cooked inspection finds no interaction, camera, or ubergraph/event-graph
  identity in the operator package;
- shell inspection preserves exact Cyclone `ItemAsset`, dynamic collision, and
  the unchanged three visual mesh identities;
- base-mesh inspection preserves exactly one simple collision box centered at
  `(0, 0, 6)`, sized `164 x 164 x 12` cm, with
  `CTF_UseSimpleAsComplex`.

Prepared but not installed package:

- ignored root:
  `artifacts/harpoon-cannon/passive-transform-marker-01/container/package`;
- `.pak`: `75E7144577253917F6DA7312EF5E585B12FB728226A22B0938323751A6B555CD`;
- `.ucas`: `AF274DEC5E413F7438376540D516B71F2E33AF20A89F9481BD4A7803FED3E3B5`;
- `.utoc`: `74340FBBF5015A3625949854243F12036438451BF87245FF60478076950CBCED`.

The user then confirmed that Voyage was closed. A fresh process check found no
shipping executable, the current fingerprint still matched, and installed
hashes identified the game-validated minimal vehicle marker exactly. Those
files were copied and hash-verified under
`artifacts/harpoon-cannon/installed-backups/before-passive-transform-marker-20260830-201504/minimal-vehicle-marker`.

Before installation, the Q-cancel baseline was restored and all three hashes
were checked again. A fresh rollback copy was written under
`artifacts/harpoon-cannon/installed-backups/before-passive-transform-marker-20260830-201504/q-cancel-baseline`.
The passive-transform candidate then replaced exactly the three Harpoon
container files; automatic failure recovery targeted that fresh baseline and
was not needed.

Installed SHA-256 readback matches the prepared package:

- `.pak`: `75E7144577253917F6DA7312EF5E585B12FB728226A22B0938323751A6B555CD`;
- `.ucas`: `AF274DEC5E413F7438376540D516B71F2E33AF20A89F9481BD4A7803FED3E3B5`;
- `.utoc`: `74340FBBF5015A3625949854243F12036438451BF87245FF60478076950CBCED`.

Installed `retoc verify` succeeded and semantic inventory is exactly the five
expected packages. The active runtime check remains construction-only:
construct one fresh object and observe stability for 10-15 seconds. No `E`
interaction is expected.

### Passive transform-only operator game-validates

The user completed a fresh construction with the installed passive-transform
marker and reported that Voyage did not crash. This game-validates a custom
operator SCS root and the complete plain-scene hierarchy
`MountRoot -> YawPivot -> PitchPivot -> OpticalAxis` under the generated
`VoyageVehiclePawn` child.

Package-05's crash is therefore not caused by adding a Blueprint-owned scene
root or nested yaw/pitch/optical transforms. The remaining introduced layers
are the game-specific `InteractiveObjectComponent`, interaction box/profile,
character exit marker, optical camera, `InteractiveInterface`, and the entry/
exit event graph. The shell lifecycle and passive operator hierarchy must stay
unchanged while those layers are split.

The passive-transform marker remains temporarily installed while Voyage is
running. Restore the Q-cancel baseline after the user exits before installing
another probe.

### InteractiveObjectComponent-only split prepared

The next candidate changes exactly one operator layer relative to the game-
validated passive marker: it adds one
`/Script/Voyage.InteractiveObjectComponent` named `OperatorInteraction` as a
child of `MountRoot`, at relative location `(-35, -70, 105)`.

It deliberately omits the interaction box/collision profile,
`InteractiveInterface`, all event graphs, character/exit marker, camera, HUD,
input, aim, trace, firing, and model components. It is therefore not expected
to display `E` or allow entry. Its sole purpose is to prove whether construction
of the exact game-specific interaction component is safe.

The candidate passed the complete local gate against the unchanged current-
build fingerprint:

- UBT/UHT and commandlet generation succeeded with `0 errors / 0 warnings`;
- one-process exact five-package cook, IoStore build, exact inventory, and
  `retoc verify` succeeded;
- cooked operator pseudocode contains the four game-validated plain scene
  components plus exactly one `UInteractiveObjectComponent`;
- the component template serializes only its inherited relative location and
  no own-field override, while its exact 20-property editor mirror keeps the
  unversioned indices aligned;
- cooked inspection finds no collision, interface, event/ubergraph, camera, or
  character-location identity in the operator package;
- shell CDO inspection preserves exact Cyclone `ItemAsset` and dynamic-
  collision identity without neighboring `MaxResources` corruption;
- the base mesh preserves its exact `164 x 164 x 12` cm simple collision box
  and `CTF_UseSimpleAsComplex` construction-cancel contract.

Prepared but not installed package:

- ignored root:
  `artifacts/harpoon-cannon/interactive-component-marker-01/container/package`;
- `.pak`: `75E7144577253917F6DA7312EF5E585B12FB728226A22B0938323751A6B555CD`;
- `.ucas`: `8A5E81FB4D7134FB3368B1F022D0AAFEC0710CAD32803853A467E7ECA1CE640B`;
- `.utoc`: `FD8CCDA3121002316529C5E9B706748C9FC7187385C6D805B48299B750EB2F93`.

The user then confirmed that Voyage was closed. Process and fingerprint gates
passed, and installed hashes identified the game-validated passive-transform
marker exactly. Those files were copied and hash-verified under
`artifacts/harpoon-cannon/installed-backups/before-interactive-component-marker-20260830-203939/passive-transform-marker`.

The installer restored and verified the Q-cancel baseline, then saved a fresh
rollback copy under
`artifacts/harpoon-cannon/installed-backups/before-interactive-component-marker-20260830-203939/q-cancel-baseline`.
Only then did it install the component-only candidate. Automatic rollback
targeted that fresh baseline and was not needed.

Installed SHA-256 readback matches the prepared package:

- `.pak`: `75E7144577253917F6DA7312EF5E585B12FB728226A22B0938323751A6B555CD`;
- `.ucas`: `8A5E81FB4D7134FB3368B1F022D0AAFEC0710CAD32803853A467E7ECA1CE640B`;
- `.utoc`: `FD8CCDA3121002316529C5E9B706748C9FC7187385C6D805B48299B750EB2F93`.

Installed `retoc verify` succeeded and semantic inventory is exactly the five
expected packages. The active test remains one fresh construction plus 10-15
seconds of stability observation. No `E` interaction is expected.

### InteractiveObjectComponent-only split game-validates

The user completed a fresh construction with the installed component-only
marker and reported that Voyage did not crash. This game-validates creation of
the exact `/Script/Voyage.InteractiveObjectComponent` on the generated
operator, including the current 20-property editor mirror alignment and its
attachment to the passive SCS root.

Package-05's crash is therefore not caused by the game-specific interaction
component alone. The remaining unvalidated layers are the interaction
primitive/profile, interface/event graph, character exit marker, and optical
camera. Keep the shell, passive transforms, and interaction component unchanged
while splitting those layers.

The component-only marker remains installed while Voyage is running. Restore
the Q-cancel baseline after exit before installing the next probe.

### Interaction box/profile split prepared

The next candidate changes exactly one operator layer relative to the game-
validated component-only marker: it adds `OperatorInteractionBox`, a native
`UBoxComponent`, as a child of `MountRoot`.

The cooked template preserves the stock-like external interaction contract:

- extent `(115, 95, 105)` and relative location `(-35, -70, 105)`;
- collision profile `Interactive`;
- object channel `GameTraceChannel2`;
- generated overlaps disabled;
- stock profile response table, with only the intended interaction trace
  response retained and ordinary world/pawn/camera channels ignored.

It still omits `InteractiveInterface`, event/ubergraph logic, character/exit
marker, optical camera, HUD, input, aim, trace, firing, and operator model
components. No `E` action is expected; this package only tests construction of
the configured primitive/profile layer.

The complete local gate passed against the unchanged fingerprint:

- UBT/UHT and commandlet generation succeeded with `0 errors / 0 warnings`;
- exact five-package cook, IoStore build, inventory, and `retoc verify`
  succeeded;
- cooked operator pseudocode contains only the previously validated hierarchy,
  exact interaction component, and one `UBoxComponent`;
- cooked inspection resolves `Interactive` rather than editor fallback
  `Custom`, with `GameTraceChannel2`, exact extent/location, and no generated
  overlaps;
- no interface, event/ubergraph, character-location, camera, or exit-tag
  identity is present;
- shell and base-mesh inspections preserve Cyclone item/dynamic-collision and
  the exact Q-cancel simple collision contract.

Prepared but not installed package:

- ignored root:
  `artifacts/harpoon-cannon/interaction-box-marker-01/container/package`;
- `.pak`: `75E7144577253917F6DA7312EF5E585B12FB728226A22B0938323751A6B555CD`;
- `.ucas`: `DBBD5291C78223217A5765E35843B76E3639989CD5B0551BDEB5BA4F39B9602B`;
- `.utoc`: `5E2FC82B4A38C17188B29565838BD0D87118580B15EFB6754334BBF6A9E83E02`.

The user then confirmed that Voyage was closed. Process/fingerprint gates
passed and installed hashes identified the game-validated component-only
marker. It was copied and hash-verified under
`artifacts/harpoon-cannon/installed-backups/before-interaction-box-marker-20260830-213914/interactive-component-marker`.

The installer restored and verified the Q-cancel baseline and saved a fresh
rollback copy under
`artifacts/harpoon-cannon/installed-backups/before-interaction-box-marker-20260830-213914/q-cancel-baseline`.
The box/profile candidate then replaced exactly the three Harpoon files;
automatic recovery targeted that fresh baseline and was not needed.

Installed SHA-256 readback matches the prepared package:

- `.pak`: `75E7144577253917F6DA7312EF5E585B12FB728226A22B0938323751A6B555CD`;
- `.ucas`: `DBBD5291C78223217A5765E35843B76E3639989CD5B0551BDEB5BA4F39B9602B`;
- `.utoc`: `5E2FC82B4A38C17188B29565838BD0D87118580B15EFB6754334BBF6A9E83E02`.

Installed `retoc verify` succeeded and semantic inventory is exactly the five
expected packages. The active test remains one fresh construction plus 10-15
seconds of stability observation. No `E` interaction is expected.

### Interaction box/profile split game-validates

The user completed a fresh construction with the installed box/profile marker
and reported that Voyage did not crash. This game-validates the complete
external interaction SCS set accumulated so far: passive transforms, exact
`InteractiveObjectComponent`, and `UBoxComponent` configured with Voyage's
`Interactive` profile and `GameTraceChannel2` object type.

Package-05's crash is therefore not caused by construction of the interaction
primitive or its collision-response table. The remaining unvalidated layers
are interface/event bytecode, character exit marker, and optical camera. Keep
the validated box/component hierarchy unchanged while splitting them.

The box/profile marker remains installed while Voyage is running. Restore the
Q-cancel baseline after exit before installing another probe.

### InteractiveInterface no-op event split prepared

The next candidate invokes exactly one additional generator operation:
`FBlueprintEditorUtils::ImplementNewInterface` for the exact
`/Script/Voyage.InteractiveInterface` identity. Unreal does not serialize this
as metadata alone. Cooked inspection proves that it necessarily produces all
of the following together:

- class interface metadata with `bImplementedByK2=true`;
- a generated `InteractAction(MyCharacter, PartId, InteractIndex)` override;
- a minimal `ExecuteUbergraph_BP_HarpoonCannon` and ubergraph frame used only
  to marshal those three parameters;
- an empty event body that immediately returns without calling
  `OnEnterVehicle`, `OnExitVehicle`, or any other function.

This means a crash would localize the problem to the exact interface identity/
signature plus Unreal's unavoidable no-op event wrapper, but could not split
those two serialized consequences further. A stable result would prove both
safe and leave the package-05 fault in our custom entry/exit bytecode or the
still-omitted character/camera components.

The candidate preserves the game-validated interaction component and box, but
still omits custom event nodes, character/exit marker, optical camera, HUD,
input, aim, trace, firing, and model components. `E` may now be discoverable
because the actor has the stock interface and collision identity, but invoking
it intentionally performs no action.

The complete local gate passed against the unchanged fingerprint:

- UBT/UHT and commandlet generation succeeded with `0 errors / 0 warnings`;
- exact cook, IoStore build, five-package inventory, and `retoc verify`
  succeeded;
- cooked pseudocode contains the exact interface plus the empty no-op
  `InteractAction` wrapper described above;
- no `OnEnterVehicle`, `OnExitVehicle`, character-location, camera, or exit-tag
  identity is present;
- shell and base-mesh inspection preserve the validated construction,
  attachment, dismantling, and Q-cancel contracts.

Prepared but not installed package:

- ignored root:
  `artifacts/harpoon-cannon/interaction-interface-marker-01/container/package`;
- `.pak`: `75E7144577253917F6DA7312EF5E585B12FB728226A22B0938323751A6B555CD`;
- `.ucas`: `CBF40BF0672ED936FC9CC5CFF46A67129FDDFB91E05BF0E3AF85440870B0456D`;
- `.utoc`: `94CE38C2AC9C0D16029C3A664F2FBF6B697A52F0A6F29B871C99EFA97257ACF6`.

The user then confirmed that Voyage was closed. Process/fingerprint gates
passed and installed hashes identified the game-validated box/profile marker.
It was copied and hash-verified under
`artifacts/harpoon-cannon/installed-backups/before-interaction-interface-marker-20260830-215254/interaction-box-marker`.

The installer restored and verified the Q-cancel baseline and saved a fresh
rollback copy under
`artifacts/harpoon-cannon/installed-backups/before-interaction-interface-marker-20260830-215254/q-cancel-baseline`.
The interface/no-op-event candidate then replaced exactly the three Harpoon
files; automatic recovery targeted that baseline and was not needed.

Installed SHA-256 readback matches the prepared package:

- `.pak`: `75E7144577253917F6DA7312EF5E585B12FB728226A22B0938323751A6B555CD`;
- `.ucas`: `CBF40BF0672ED936FC9CC5CFF46A67129FDDFB91E05BF0E3AF85440870B0456D`;
- `.utoc`: `94CE38C2AC9C0D16029C3A664F2FBF6B697A52F0A6F29B871C99EFA97257ACF6`.

Installed `retoc verify` succeeded and semantic inventory is exactly the five
expected packages. The active test is one fresh construction plus 10-15
seconds of stability. If `E` appears, pressing it once is safe but should be a
visible no-op; lack of entry is expected.

### InteractiveInterface no-op event split rejected at fabrication completion

The one-variable interface candidate crashed at the instant a fresh
HarpoonCannon finished fabrication. Fresh crash context
`UECC-Windows-2E8F3F9E45430E6AE79A88A26386F4E2_0000` reports a GameThread
`EXCEPTION_ACCESS_VIOLATION` reading null after 67 seconds. This is the same
live completion boundary exercised safely by all preceding splits, so the
result rejects the newly introduced interface layer rather than the shell
spawn/attach lifecycle, passive transform hierarchy,
`InteractiveObjectComponent`, or stock-like interaction box/profile.

Cooked inspection had already proved that this candidate contained no custom
entry/exit bytecode, character marker, camera, HUD, input, aim, trace, firing,
or model change. Its only unavoidable graph consequence was Unreal's empty
`InteractAction(MyCharacter, PartId, InteractIndex)` event wrapper and minimal
ubergraph. Therefore the crash is not evidence against the intended
`OnEnterVehicle` behavior; that code was absent.

The editor-only `/Script/Voyage.InteractiveInterface` mirror used to author
this package is now known to be incomplete. It declares only `InteractAction`,
while two independent stock implementers, JetSki and GyroCopter, expose at
least the same additional interface-shaped overrides:

- `InteractGetInventory(MyCharacter, PartId)`;
- `InteractActionPartial(MyCharacter, PartId, InteractIndex)`;
- `GetInteractiveProvidedActions(MyCharacter, Component, OutActions)`;
- `GetInteractiveProvidedGrabActions(MyCharacter, Component, OutActions)`.

The leading hypothesis is an unsafe native interface/function-table contract:
the generated class was authored against a partial interface identity and then
loaded by a runtime interface with more functions and referenced types. Do not
retry `ImplementNewInterface`, add custom interaction bytecode, or combine a
camera/character component until the exact complete current-build interface
function inventory, signatures, return types, flags, and referenced Voyage
types are reconstructed and statically checked against multiple stock
implementers.

The crashed process was absent before recovery. The installer restored exactly
the three game-validated simple-collision/Q-cancel baseline files from the
hash-verified backup at
`artifacts/harpoon-cannon/installed-backups/before-interaction-interface-marker-20260830-215254/q-cancel-baseline`.
Installed SHA-256 readback is again:

- `.pak`: `75E7144577253917F6DA7312EF5E585B12FB728226A22B0938323751A6B555CD`;
- `.ucas`: `25C5CD17F7B12F6C3CAF06DD3A514E05743143F234A387AF92ACF042C910330B`;
- `.utoc`: `AEBA17E558F3209F095F663B3C372EEA9AA3E96AE43E8CE3A543B074F10E9429`.

Post-rollback `retoc verify` succeeded, semantic inventory is again exactly the
five expected packages, and a fresh process/hash readback after compaction
confirmed the game remains stopped on that baseline. Entry/exit is still
unimplemented.

### Complete InteractiveInterface no-op split prepared

Lightweight comparison of three independent current-build stock implementers
(`BP_JetSki_Possessable`, `BP_GyroCopter_Possessable`, and
`BP_Location_Fabricator`) found the same six interface functions with matching
parameter names, types, return types, and Blueprint event flags:

- `GetInteractiveProvidedActions(APawn*, USceneComponent*,
  TArray<FPlayerInputInterfaceAction>&) -> bool`;
- `GetInteractiveProvidedGrabActions(APawn*, USceneComponent*,
  TArray<FPlayerInputInterfaceAction>&) -> bool`;
- `InteractAction(APawn*, int32 PartId, int32 InteractIndex) -> void`;
- `InteractActionPartial(APawn*, int32 PartId, int32 InteractIndex) -> bool`;
- `InteractGetInventory(APawn*, int32 PartId) ->
  UVoyageBaseInventoryComponent*`;
- `InteractWithGrab(APawn*, int32 PartId,
  EInteractWithGrabReturnValue&) -> void`.

A targeted executable string/registration search independently places those
same six reflected identities in the compact interaction-interface registration
region immediately before `UInteractiveInterface`. No seventh interface-shaped
function appears in that region; the later `InteractiveTextInterface` names
form a separate cluster. Current executable registration also establishes
`EInteractWithGrabReturnValue` as `Invalid`, `No`, `Yes`, and `YesNoDelay` in
that order.

The supporting signature types are now bounded rather than invented:

- mappings prove `VoyageBaseInventoryComponent : SceneComponent` with 35 own
  properties; Harpoon uses only an empty pointer-identity stand-in and never
  creates or serializes an instance;
- mappings prove `PlayerInputInterfaceAction` has 23 own reflected fields;
  stock cooked interface exports consistently record `0x290` bytes for the
  array element, including the six delegate-tail fields absent from the
  narrower DonkLift authoring mirror;
- Harpoon therefore uses an opaque, aligned `0x290`-byte signature-only
  stand-in. This checkpoint never reads, writes, constructs, or serializes an
  action value, avoiding guessed delegate signatures while preserving the
  exact reflected identity and cooked element size.

The rejected partial generated `Content` was first moved intact to ignored
`artifacts/harpoon-cannon/generated-checkpoints/interaction-interface-marker-01-generated`.
The corrected mirror then passed UHT/native build and generated all assets with
`0 errors / 0 warnings`. The exact five-package cook, IoStore build, inventory,
and `retoc verify` succeeded. Cooked binary identity checks find all six
functions and all three supporting Voyage type identities, while finding none
of `OnEnterVehicle`, `OnExitVehicle`, `OperatorCharacterLocation`,
`ExitComponentTag`, `OpticalCamera`, `GetHUDOverrideWidget`, look input, or the
Harpoon HUD. This preserves the one-variable boundary: only the formerly
partial interface contract changed relative to the crashing marker.

Prepared but not installed candidate:

- ignored root:
  `artifacts/harpoon-cannon/interaction-interface-complete-marker-01/container/package`;
- `.pak`: `75E7144577253917F6DA7312EF5E585B12FB728226A22B0938323751A6B555CD`;
- `.ucas`: `68443603B27F8EB7CB7D6554DB08675D27CDFF43059A9320DCC576D4A95BE597`;
- `.utoc`: `70795A7B532D1A054B5579806038AA42281A4FA5F0EAEB28A5BB615C5B20E309`.

The next runtime test, only after an explicit closed-game gate, remains one
fresh fabrication completion plus 10-15 seconds of stability. No entry, camera,
HUD, aim, trace, or firing behavior is expected. A stable result would validate
the complete interface metadata/wrappers and move the next split to custom
`InteractAction` bytecode. Another completion crash would reject the remaining
signature-only support-type assumptions and require native registration-field
inspection before any further package.

The user explicitly confirmed that Voyage was closed. A fresh process check
found no shipping executable, and the installed game fingerprint still matched
Steam build `23962331` plus the registered executable SHA-256. All three
installed Harpoon files matched the game-validated simple-collision/Q-cancel
baseline and were copied to the hash-verified rollback directory
`artifacts/harpoon-cannon/installed-backups/before-interaction-interface-complete-marker-20260830-230758/q-cancel-baseline`.

The complete-interface candidate then replaced exactly those three files.
Automatic recovery targeted the fresh baseline and was not needed. Installed
SHA-256 readback matches the prepared package, `retoc verify` succeeds, and
installed inventory is exactly the leaf Cyclone shell, `BP_HarpoonCannon`, and
the unchanged base/yaw/pitch meshes. The active runtime checkpoint is only a
fresh construction through fabrication completion plus 10-15 seconds of
stability. No `E` entry is expected from these no-op wrappers.

### Complete InteractiveInterface no-op split rejected

The complete six-function interface candidate also crashed exactly when a
fresh HarpoonCannon finished fabrication. Crash context
`UECC-Windows-F134B7BA4B3532B870756A83FFC09C2E_0000` records a GameThread
`EXCEPTION_ACCESS_VIOLATION` reading null after 46 seconds. Its native stack
hash (`C8E53CF7FA8B3CAFAAA7AF7E611C4BEE1B0C2D27`) differs from the rejected
partial-interface marker, but the live lifecycle boundary is identical.

This rejects the hypothesis that merely adding the five omitted functions to
the editor mirror makes `ImplementNewInterface` safe. It does not reject the
stock interaction/possession architecture: all custom entry/exit bytecode,
camera, HUD, input, aim, trace, firing, and model changes were absent. The
remaining unsafe boundary is authoring a generated class as a direct
implementer of a reconstructed native interface. Possible causes include a
still-inexact reflected signature/support type, function metadata that is not
visible in cooked stock overrides, or runtime behavior associated with direct
interface implementation. Do not iterate more guessed signatures or add
custom interaction bytecode on this class.

The crashed process and crash reporter were absent before recovery. Exactly
the three Harpoon files were restored from the hash-verified rollback at
`artifacts/harpoon-cannon/installed-backups/before-interaction-interface-complete-marker-20260830-230758/q-cancel-baseline`.
Installed SHA-256 readback is again:

- `.pak`: `75E7144577253917F6DA7312EF5E585B12FB728226A22B0938323751A6B555CD`;
- `.ucas`: `25C5CD17F7B12F6C3CAF06DD3A514E05743143F234A387AF92ACF042C910330B`;
- `.utoc`: `AEBA17E558F3209F095F663B3C372EEA9AA3E96AE43E8CE3A543B074F10E9429`.

Post-rollback `retoc verify` succeeded and semantic inventory is exactly the
five expected Q-baseline packages. The installed game is safe to launch, but
entry/exit remains unimplemented.

### Stock Camera Drone operator discriminator prepared

The Drone evidence provides a lighter alternative to more guessed interface
ABI work. `/Game/Blueprints/Vehicles/BP_CameraDrone` derives from native
`VoyageVehicleCameraDronePawn`; its Blueprint overrides
`GetInteractiveProvidedActions` while its own implemented-interface list does
not add `InteractiveInterface`. The interaction contract is therefore already
owned by its stock ancestry rather than reconstructed by the Harpoon editor
mirror.

The next one-variable candidate keeps the validated leaf Cyclone shell,
base/yaw/pitch model, pivots, item identity, attachment graph, dynamic
collision, and base-mesh simple collision unchanged. At fabrication completion
the existing shell lifecycle spawns the stock `BP_CameraDrone_C` class instead
of the generated `BP_HarpoonCannon_C` operator. The generated Harpoon operator
with the rejected interface mirror remains cooked for exact inventory
continuity but is not instantiated. An editor-only AActor Blueprint stub at
the exact Camera Drone package/class path supplies only the class reference
during authoring; that stub is excluded from cook and the player container, so
runtime resolves the original base-game class.

This is a disposable architecture discriminator, not the intended visible or
physical cannon operator. It must not be evaluated for model shape, movement,
camera framing, HUD, controls, or final interaction semantics. Its outcomes
are deliberately narrow:

1. fabrication remains stable and a stock Drone appears attached to the
   shell: the shell can host a stock-owned interaction/possession actor and
   the unsafe layer is our direct reconstructed-interface implementation;
2. fabrication crashes at the same boundary: spawning an interactive vehicle
   from this shell lifecycle is itself unsafe or the stock Drone has unmet
   owner/item assumptions, so direct shell-spawn needs a different split;
3. fabrication completes but the Drone immediately detaches, self-removes, or
   enters another invalid state: construction is safe, but Drone-specific
   lifecycle makes it unsuitable beyond this class-ownership discriminator.

Local gates passed against the unchanged Voyage fingerprint:

- UHT/UBT build and commandlet generation succeeded with `0 errors / 0
  warnings`;
- generated shell binary imports
  `/Game/Blueprints/Vehicles/BP_CameraDrone.BP_CameraDrone_C` and no longer
  selects `BP_HarpoonCannon_C` as its spawned class;
- the generated Harpoon operator still contains the complete-interface marker
  and no custom entry/exit, camera, HUD, aim, trace, or firing layer;
- exact five-package cook succeeded in one Unreal process;
- the Camera Drone reference stub, item stub, and HUD did not enter the
  container;
- exact inventory and `retoc verify` succeeded.

Prepared but not installed candidate:

- ignored root:
  `artifacts/harpoon-cannon/stock-camera-drone-operator-marker-01/container/package`;
- `.pak`: `75E7144577253917F6DA7312EF5E585B12FB728226A22B0938323751A6B555CD`;
- `.ucas`: `C7C5B11972F316BD351AFFFF137207AC52315FE2F3BB4CF45CAA9DC5E804497C`;
- `.utoc`: `F243C2B5C3AD66590D2A2CEFB70B289611198F343F6696EAB552DF0CCF87698A`.

The user explicitly confirmed Voyage was closed. A fresh process check found
no Voyage or crash-reporter process, and the installed fingerprint still
matched Steam build `23962331` plus the registered executable hash. All three
installed Harpoon files matched the verified Q-cancel baseline and were copied
with hash readback to:

`artifacts/harpoon-cannon/installed-backups/before-stock-camera-drone-operator-marker-20260830-2352/q-cancel-baseline`.

The stock-CameraDrone candidate then replaced exactly those three files with
automatic baseline rollback armed. No rollback was needed. Installed SHA-256
readback matches the prepared candidate:

- `.pak`: `75E7144577253917F6DA7312EF5E585B12FB728226A22B0938323751A6B555CD`;
- `.ucas`: `C7C5B11972F316BD351AFFFF137207AC52315FE2F3BB4CF45CAA9DC5E804497C`;
- `.utoc`: `F243C2B5C3AD66590D2A2CEFB70B289611198F343F6696EAB552DF0CCF87698A`.

Installed `retoc verify` succeeded and semantic inventory is exactly the five
expected Harpoon packages; the editor-only Camera Drone reference stub is not
present. The active runtime test is one fresh fabrication completion plus
10-15 seconds of stability. If completion succeeds, observe only whether a
stock Drone appears and remains attached to the shell. Do not test or evaluate
its movement, camera, HUD, input, or final Harpoon interaction behavior.

### Stock Camera Drone operator discriminator: stable but not visible

The user completed a fresh HarpoonCannon with the installed discriminator.
Voyage did not crash, but no stock Drone was visibly present after completion.
The Harpoon model was intentionally unchanged for this logic-only test; the
new model checkpoint from the visual branch was not integrated.

This validates that replacing the spawned-class import with the stock
`BP_CameraDrone_C` path removes the deterministic fabrication-completion crash.
It does not yet prove that a usable operator survives: the Drone may have
spawned hidden, immediately removed itself through its deployment/item
lifecycle, or failed to spawn while the shell graph continued. Before another
package, check whether approaching the completed cannon exposes the stock
`Enter` interaction. Presence of that action would prove a surviving hidden
interactive actor; its absence keeps spawn failure versus immediate teardown
unresolved. Do not press the action during this observation.

The user then confirmed the completed cannon exposes no entry action and no
other HUD message. Construction remains stable. This rejects the idea that
merely spawning the stock class yields an already usable hidden operator.

Static Drone bytecode explains the negative result without requiring another
broad search. `GetInteractiveProvidedActions` would return Loot and Enter
actions, but the actor's `ActiveCollisionSphere` is enabled only by
`SetDroneActive(true, UpdateDeploy)`. With `UpdateDeploy=true`, that function
also invokes `DeployDrone(true)`, which opens the Drone and updates its active
state. A plain shell `SpawnActor` never calls this stock deployment boundary,
so no interaction detector can reach the provider.

The next one-variable candidate should retain the exact stock Drone class and
call its exact Blueprint-owned `SetDroneActive(bool Active, bool UpdateDeploy)`
with both values true after a successful spawn and before attachment. The
function is owned by
`/Game/Blueprints/Vehicles/BP_CameraDrone.BP_CameraDrone_C`, not by the native
`VoyageVehicleCameraDronePawn`; author it through the existing editor-only
class-reference stub so the cooked shell imports the stock Blueprint function
owner. Do not invent the same function on the native mirror. Expected first
evidence is a visible deployed Drone and/or stock Loot/Enter HUD actions. Even
then, actual entry may remain disabled because the shell-spawned Drone has no
normal item/resource initialization; do not press an action in this probe.

### Stock Camera Drone activation discriminator prepared

The call-only candidate is now prepared without touching the running game.
The authoring stub defines the exact Blueprint-owned
`SetDroneActive(bool Active, bool UpdateDeploy)` signature only so the shell
can compile a call whose runtime owner is the original
`BP_CameraDrone_C`. Both arguments are fixed to true. No function with this
name was added to a `/Script/Voyage` mirror.

Relative to the installed stable-but-inactive candidate, the shell's
post-spawn graph adds only that stock activation call before storing and
attaching the operator. The shell, item identity, model geometry, pivots,
collision, attachment, destruction cleanup, generated Harpoon operator, and
all camera/HUD/input/aim/trace/firing layers are unchanged.

Local gates passed against the unchanged fingerprint:

- UHT/UBT and generation succeeded with `0 errors / 0 warnings`;
- generated/cooked shell binary contains the exact stock Camera Drone package,
  class, and `SetDroneActive` function identities;
- exact five-package cook completed in one Unreal process with `0 errors / 0
  warnings`;
- the editor-only Camera Drone stub, item stub, and HUD are absent from the
  player container;
- exact inventory and `retoc verify` succeeded.

Prepared but not installed candidate:

- ignored root:
  `artifacts/harpoon-cannon/stock-camera-drone-activation-marker-01/container/package`;
- `.pak`: `75E7144577253917F6DA7312EF5E585B12FB728226A22B0938323751A6B555CD`;
- `.ucas`: `BBD76A86CDEAB81FCC02D317235ED0CDBE1B79E1FA2E6C6090281626C7B7D4F0`;
- `.utoc`: `4E57FE8D3FA499ECF1D0AEC46D72AB0309139B96021766566BD4D7D52948B845`.

The user explicitly confirmed Voyage was closed. Process and fingerprint gates
passed, and all installed hashes matched the stable inactive-stock-Drone
candidate. That immediate predecessor was copied and hash-verified at:

`artifacts/harpoon-cannon/installed-backups/before-stock-camera-drone-activation-marker-20260831-0033/inactive-stock-drone`.

The activation candidate then replaced exactly the three Harpoon files with
automatic rollback to that predecessor armed. No rollback was needed.
Installed SHA-256 readback matches the prepared package:

- `.pak`: `75E7144577253917F6DA7312EF5E585B12FB728226A22B0938323751A6B555CD`;
- `.ucas`: `BBD76A86CDEAB81FCC02D317235ED0CDBE1B79E1FA2E6C6090281626C7B7D4F0`;
- `.utoc`: `4E57FE8D3FA499ECF1D0AEC46D72AB0309139B96021766566BD4D7D52948B845`.

Installed `retoc verify` succeeded and semantic inventory is exactly the five
expected packages with no Camera Drone stub. The active runtime observation is
one fresh fabrication completion plus 10-15 seconds, followed only by checking
whether the Drone becomes visible and whether stock Loot/Enter HUD actions
appear. Do not invoke either action in this checkpoint.

### Stock Camera Drone activation discriminator: stable but still occluded

The activation candidate also completed fabrication without a crash, but the
user saw no Drone and no interaction/HUD action. This proves the exact stock
activation call is runtime-safe in the shell path, but visual absence alone
does not reject a surviving actor: the shell spawns at its own transform and
then attaches with `SnapToTarget`, placing the Drone root at the center of the
large opaque Harpoon model. That same geometry can occlude both the Drone and
the interaction detector's trace.

The next smallest discriminator is spatial only. Preserve the same stock
class, activation call, attachment, and every Harpoon contract, then set the
attached Drone to an obvious relative offset 250 cm to the side and 250 cm
above the shell. A visible Drone or stock action there proves that spawn,
activation, and provider all survive; continued absence would strongly reject
the spawned-operator path rather than merely its placement inside the cannon.

### Stock Camera Drone spatial discriminator prepared, not installed

The one-variable spatial candidate is prepared while leaving the running game
and its installed activation package untouched. After the existing
`SnapToTarget` attachment and exact `SetDroneActive(true, true)` call, the shell
now calls `K2_SetActorRelativeLocation` with relative location
`(X=0,Y=-250,Z=250)`, sweep disabled, and teleport enabled. No class,
activation, collision, interface, model, shell lifecycle, camera, HUD, input,
or resource contract changed.

The first generation attempt was rejected locally before cook because
`FVector::ToString()` emitted spaces in the reflected vector default and the
Blueprint compiler could not import that pin value. The generator now uses a
named canonical reflected-vector literal. Preserve the failed output only as
ignored diagnostic evidence at
`artifacts/harpoon-cannon/generated-checkpoints/stock-camera-drone-offset-marker-01-invalid-vector-generated`;
do not treat it as a runtime candidate.

The corrected generator build and generation completed with zero errors and
zero warnings. The exact five-package cook completed with zero errors and zero
warnings. `retoc verify` and exact inventory validation passed. The prepared
container is:

`artifacts/harpoon-cannon/stock-camera-drone-offset-marker-01/container/package`

Prepared hashes:

- `.pak`: `75E7144577253917F6DA7312EF5E585B12FB728226A22B0938323751A6B555CD`
- `.ucas`: `862920A4E7C5144FEF44650C4979E1A87F03DAE64BC5ACCEF8FD9F60BCA55793`
- `.utoc`: `38F13E75E021C17536BDB5B2D9819092225D10C5B5BCD15BBB4A3D4852C787BC`

The container inventory is still exactly the Cyclone leaf, Harpoon shell, and
three Harpoon meshes. It contains no CameraDrone authoring stub, HUD package,
or item stub. Cooked shell markers include the stock CameraDrone class,
`SetDroneActive`, `K2_SetActorRelativeLocation`, `Fabricated`, and
`OperatorActor`.

This candidate is **not installed**. Wait for explicit closed-game
confirmation. Its first runtime observation is only whether fabrication stays
stable and whether the deliberately displaced Drone and/or a stock interaction
action becomes visible. Do not enter, loot, or otherwise invoke it in this
checkpoint.

The user then explicitly confirmed Voyage was closed. A fresh process gate
found neither Voyage nor CrashReportClient, and the installed fingerprint
still matched Steam build `23962331` and the registered executable SHA-256.
All three installed files matched the activation-only predecessor and were
copied with hash readback to:

`artifacts/harpoon-cannon/installed-backups/before-stock-camera-drone-offset-marker-20260831-0105/activated-stock-drone`.

The spatial candidate then replaced exactly those three files with automatic
rollback to that predecessor armed. No rollback was needed. Installed hashes
match the prepared candidate:

- `.pak`: `75E7144577253917F6DA7312EF5E585B12FB728226A22B0938323751A6B555CD`
- `.ucas`: `862920A4E7C5144FEF44650C4979E1A87F03DAE64BC5ACCEF8FD9F60BCA55793`
- `.utoc`: `38F13E75E021C17536BDB5B2D9819092225D10C5B5BCD15BBB4A3D4852C787BC`

Installed `retoc verify` and exact five-package inventory validation passed.
The active real-game checkpoint is now: complete one fresh construction, wait
10-15 seconds, then approach and look around the cannon for the intentionally
displaced stock Drone and any stock Loot/Enter HUD action. Do not invoke an
action even if it appears.

### Stock Camera Drone spatial discriminator: stable, no interaction

The fresh cannon completed without a crash, but no interaction hint appeared.
The user has not yet separately confirmed whether a displaced Drone mesh was
visible, so do not conflate the absent HUD action with proven actor absence.
The spatial offset nevertheless did not recover the interaction path.

The shipping build produced no usable gameplay log. Static review confirms the
spawn node already uses `AlwaysSpawn`, so another collision-handling experiment
would not discriminate the remaining hypotheses. The unresolved boundary is
now whether `SpawnActor` returns a valid stock actor at all, versus a valid
actor becoming unusable or invisible through its stock lifecycle immediately
after creation.

The next probe should expose that boundary directly in the same frame. Branch
on `IsValid(SpawnActor result)` before activation. On the valid branch,
temporarily hide only the shell's `HarpoonBarrelBody`, then continue the
unchanged stock activation, attachment, and offset path. On the invalid branch,
disable the one-shot shell tick without calling the stock actor. A missing
barrel proves immediate spawn success; an unchanged cannon proves the stock
class did not instantiate. This marker is diagnostic only and must not be
interpreted as a model change.

### Stock Camera Drone immediate spawn-validity marker prepared, not installed

The marker is now prepared without touching the running game or installed
spatial package. The previous valid spatial generated assets were preserved at
`artifacts/harpoon-cannon/generated-checkpoints/stock-camera-drone-offset-marker-01-generated`.

Two local authoring failures were rejected before cook:

- an initial editor launch used the unwritable default global DDC and stopped
  before the commandlet; all later generation uses the mod-local DDC;
- the first validity graph connected the new execution branch but omitted the
  stock activation call's object target and continuation. Blueprint compilation
  rejected it. Its ignored output is preserved at
  `artifacts/harpoon-cannon/generated-failures/stock-camera-drone-spawn-validity-marker-01-unbound-target`
  and must never be packaged.

After connecting those exact pins, UBT and generation succeeded. The generated
shell branches on immediate `IsValid(SpawnActor result)`. The true branch hides
`HarpoonBarrelBody`, then continues the unchanged `SetDroneActive(true, true)`,
attachment, and relative-offset path. The false branch disables the one-shot
shell tick and does not call the null actor. No model asset, component
transform, collision, item identity, shell attachment contract, camera, HUD,
input, resource, or persistence layer changed.

The exact five-package cook completed with `0 errors / 0 warnings`.
`retoc verify` and exact inventory validation passed. The editor-only stock
Drone and item reference stubs and the unused HUD remain outside the player
container. Cooked shell markers include `BP_CameraDrone_C`, `IsValid`,
`SetVisibility`, `HarpoonBarrelBody`, `SetDroneActive`,
`K2_SetActorRelativeLocation`, `Fabricated`, and `OperatorActor`.

Prepared but not installed container:

`artifacts/harpoon-cannon/stock-camera-drone-spawn-validity-marker-01/container/package`

Prepared hashes:

- `.pak`: `75E7144577253917F6DA7312EF5E585B12FB728226A22B0938323751A6B555CD`
- `.ucas`: `705DE7DAC3A43848364DF470197E710CD8BE42E06C3F9CFFDD2CB404144D8BDD`
- `.utoc`: `6B0A24037A570506BB794EBE7000C0EA2B3D11C2FFAA34D736EDAF2A4DBB8D57`

Installation remains gated on explicit closed-game confirmation. The runtime
observation is deliberately binary: after fresh fabrication, a missing barrel
means the stock actor existed immediately; a complete cannon means spawn
returned invalid. Do not test interaction, entry, movement, or Drone controls
in this checkpoint.

The user explicitly confirmed Voyage was closed. A fresh process gate found no
Voyage or CrashReportClient process, and the fingerprint still matched Steam
build `23962331` plus the registered executable SHA-256. All three installed
files matched the spatial predecessor and were copied with hash readback to:

`artifacts/harpoon-cannon/installed-backups/before-stock-camera-drone-spawn-validity-marker-20260831-0203/spatial-stock-drone`.

The immediate-validity marker then replaced exactly those three files with
automatic rollback to the spatial predecessor armed. No rollback was needed.
Installed hashes match the prepared candidate:

- `.pak`: `75E7144577253917F6DA7312EF5E585B12FB728226A22B0938323751A6B555CD`
- `.ucas`: `705DE7DAC3A43848364DF470197E710CD8BE42E06C3F9CFFDD2CB404144D8BDD`
- `.utoc`: `6B0A24037A570506BB794EBE7000C0EA2B3D11C2FFAA34D736EDAF2A4DBB8D57`

Installed `retoc verify` and exact five-package inventory validation passed.
The active test is now only the fresh-fabrication barrel marker. Do not invoke
any interaction even if one unexpectedly appears.

### Immediate spawn-validity marker: stock actor creation proven

The fresh cannon completed without a crash and its barrel disappeared after
fabrication. This is the exact true-branch marker, so the stock
`BP_CameraDrone_C` `SpawnActor` result is valid in the live shell graph.
Collision rejection, unresolved stock-class import, and null spawn are now
rejected explanations for the missing Drone and HUD interaction.

The user's screenshot confirms the exact visual signature: the complete
`HarpoonBarrelBody`/pitch assembly is absent while the base and yaw-owned
geometry remain. No displaced stock Drone is visible in the frame. Treat the
missing pitch assembly strictly as the deliberate logic marker, not as a model
regression.

The remaining boundary is post-spawn lifecycle: the valid actor may be
destroyed shortly afterward, or it may survive while remaining invisible and
unregistered with the interaction detector. The next smallest discriminator
should disable the shell's one-shot tick immediately, wait two seconds, then
recheck the stored operator reference. Restore `HarpoonBarrelBody` visibility
only when that delayed `IsValid` succeeds. A complete cannon after the delay
would support survival; a persistently missing barrel is ambiguous without
an independent continuation marker (corrected at the 2026-09-05 restart).
This remains a temporary
logic marker, not a visual-model change.

### Two-second stock actor validity marker prepared, not installed

The previous immediate-validity generated assets are preserved at
`artifacts/harpoon-cannon/generated-checkpoints/stock-camera-drone-spawn-validity-marker-01-generated`.
The new shell preserves the immediate spawn check and hidden-barrel marker. On
both immediate outcomes it disables the shell tick before starting a two-second
latent delay, preventing repeated operator spawns. After the delay it checks
the stored `OperatorActor`; only the valid branch restores
`HarpoonBarrelBody` visibility. The stock class, activation call, attachment,
spatial offset, model assets, collision, item identity, camera, HUD, input,
resources, and persistence remain unchanged.

UBT and generation succeeded with zero errors and zero warnings. The exact
five-package cook completed with `0 errors / 0 warnings`. `retoc verify` and
exact inventory validation passed. Cooked shell markers include
`BP_CameraDrone_C`, `Delay`, `IsValid`, `SetVisibility`, `HarpoonBarrelBody`,
`SetDroneActive`, `K2_SetActorRelativeLocation`, `Fabricated`, and
`OperatorActor`.

Prepared but not installed container:

`artifacts/harpoon-cannon/stock-camera-drone-delayed-validity-marker-01/container/package`

Prepared hashes:

- `.pak`: `75E7144577253917F6DA7312EF5E585B12FB728226A22B0938323751A6B555CD`
- `.ucas`: `81D83417B49A9757C607893D244E2FBF92C519C73CF18979E9F587FF7AE5356F`
- `.utoc`: `AF32780A8C76449566B503E4B2985EBBDA8E562DF0882BCDBC4FDCA0C445B65B`

Installation remains gated on explicit closed-game confirmation. The user only
needs to inspect a freshly completed cannon after more than two seconds: a
restored/full barrel would support survival, while a missing barrel alone does
not distinguish early invalidation from a missing delayed continuation. No HUD
or interaction check is part of this checkpoint.

The user explicitly confirmed Voyage was closed. A fresh process gate found no
Voyage or CrashReportClient process, and the installed fingerprint remained
Steam build `23962331` plus the registered executable SHA-256. The three
installed files matched the immediate-validity predecessor and were copied
with hash readback to:

`artifacts/harpoon-cannon/installed-backups/before-stock-camera-drone-delayed-validity-marker-20260831-0243/immediate-validity-marker`.

The delayed-validity candidate then replaced exactly those three files with
automatic rollback to the immediate marker armed. No rollback was needed.
Installed hashes match the prepared candidate:

- `.pak`: `75E7144577253917F6DA7312EF5E585B12FB728226A22B0938323751A6B555CD`
- `.ucas`: `81D83417B49A9757C607893D244E2FBF92C519C73CF18979E9F587FF7AE5356F`
- `.utoc`: `AF32780A8C76449566B503E4B2985EBBDA8E562DF0882BCDBC4FDCA0C445B65B`

Installed `retoc verify` and exact five-package inventory validation passed.
The active runtime test is only whether a freshly fabricated cannon has its
full barrel restored after two seconds. Do not test HUD or interaction.

### Delayed stock actor validity result: disappearance observed, cause unresolved

The user observed the complete barrel during fabrication and its disappearance
after fabrication completed. It did not return while he inspected the finished
object, and no Drone or Drone-related HUD appeared. No precise timing was
reported or required from the user.

Fresh cooked inspection of the installed shell confirmed that the candidate
contains the intended graph: immediate valid spawn hides
`HarpoonBarrelBody`, stores and activates the stock `BP_CameraDrone_C`,
attaches and offsets it, disables the shell tick, waits two seconds, and
restores the barrel only when the stored `OperatorActor` is still valid. The
persistent missing-barrel result does not independently prove that the latent
continuation ran or that its stored reference retained the intended actor.
Early invalidation is a hypothesis, not a demonstrated destruction time or
deployment-owner diagnosis. Do not continue adding visibility, collision, HUD,
or possession calls to this spawned stock Drone. Direct spawning remains an
unsuccessful operator approach, not proof that the stock class can never work.

### Game update hard stop

During the subsequent read-only operator-candidate inventory, the installed
game changed from Steam build `23962331` / executable SHA-256
`6A9AE86E5CE5D7D1B6555F579091AAB1E0E67FF7A96276FA2570052F99102E8D`
to Steam build `24990438` / executable SHA-256
`D9BF4C9624C60615198E62C87DA7792A9888AB02F7905AAAF1C9B02C7A9E524F`.
The base container inventory also changed. A fresh fingerprint independently
confirmed the new values.

This invalidated the evidence for the registered native mirrors, mappings,
inspections and prepared packages; it did not itself prove incompatibility.
The delayed marker was installed at that historical observation, not at the
2026-09-05 restart. Current mapping work uses `Get-VoyageMappings.ps1` first;
generation is only allowed after a genuinely unmatched fingerprint. Historical
structural findings require current-build revalidation before reuse.

## Build-pipeline handoff from DonkLift

The DonkLift task reported a game-validated pipeline improvement from commit
`dc1e859`: UE 5.7 accepts multiple exact packages as a `+`-joined value of one
`-Package=` argument under `CookSinglePackageNoRefs`. HarpoonCannon adopted this
inside its own cook script, reducing the three narrow cooks to one Unreal
process without using broad `CookDir` or changing the exact package inventory.
The Harpoon-specific implementation was then run against all three probe
packages: it completed successfully in one editor process, staged exactly six
`.uasset`/`.uexp` files, and took about 11 seconds wall-clock on this machine.

The full DonkLift player-release/ZIP pipeline is intentionally not copied yet.
HarpoonCannon remains a disposable gameplay probe with a temporary Cyclone
identity; release semantics would be premature. The clean-original extraction
pattern is retained as a future option if HarpoonCannon later needs to relocate
or inherit a shipping Blueprint snapshot.

## Confirmed current-build findings

### Cyclone item and actor chain

The visible skill name `Cyclone` resolves through this chain:

1. `DA_Skill_Engineering_Cyclone_Wind_Turbine` unlocks
   `DA_Item_Module_WindTurbineMedium`.
2. That item is category `Module`, tag `Module`, and its `DroppedActor` is
   `/Game/Blueprints/Modules/Generators/BP_Module_WindTurbine_Medium_New`.
3. `BP_Module_WindTurbine_Medium_New_C` is a thin child of
   `BP_Module_WindTurbine_Small_C`. It mainly replaces meshes, mass, item asset,
   and module name.
4. `BP_Module_WindTurbine_Small_C` derives from native
   `/Script/Voyage.VoyageModuleActor` and owns the reusable module behavior.

The Cyclone recipe is data-driven through the item asset: two turbine blades,
one connector port, one logic chip, and two alloy frames. The exact cannon
recipe can therefore remain a later data decision rather than an actor-logic
problem.

### Cyclone module contract

The base wind turbine supplies these relevant components/defaults:

- `VoyageCustomModuleComponent` with an item asset, active/auto-start config,
  resource handling, and module workload;
- `VoyagePersistentActorComponent`;
- `VoyageDestructibleObjectComponent`;
- `VoyageDynamicCollisionComponent` with `bAutoWeld=true`;
- ordinary collision-bearing base/support meshes;
- a `VoyageModuleSocketViewComponent` for its electrical output.

Its Blueprint logic computes wind-driven production and animation. Placement,
attachment, fabrication, persistence, and destruction are not implemented as
Cyclone-specific graph logic. This strongly indicates that those checks belong
to the generic item/module/components and collision defaults.

`PersistTranform()` returns false in the turbine Blueprint. Do not infer from
that alone that transform persistence is absent; attachment/persistence may be
owned by native module/component code.

### A module item can create a vehicle pawn directly

The forklift is decisive evidence against a forced two-actor architecture:

- `DA_Item_Module_Forklift` is also category `Module`, tag `Module`;
- its `DroppedActor` is
  `/Game/Blueprints/Vehicles/BP_Forklift_Possesable_C`;
- that actor derives from `VoyageVehicleForkliftPawn -> VoyageVehiclePawn ->
  Pawn`, not from `VoyageModuleActor`;
- nevertheless it owns `VoyageCustomModuleComponent`,
  `VoyagePersistentActorComponent`, `VoyageDestructibleObjectComponent`, and
  `InteractiveObjectComponent`.

Therefore the generic item/module path can instantiate a possessable pawn, but
the later attachment test proved that successful placement does not grant that
pawn the stationary module lifecycle. The current leading architecture is a
`VoyageModuleActor` shell plus a child `VoyageVehiclePawn` operator. The shell
owns placement, attachment, persistence, destruction, and structural
collision; the pawn owns entry, camera, input, aiming, trace, and HUD. The next
real-game probe must validate shell attachment before this boundary is treated
as proven.

### Vehicle entry, driver fixation, camera, and input

Static Blueprint inspection confirms the forklift interaction path:

- `InteractAction` validates the interacting pawn, interactive part/index, and
  player controller, then calls inherited `OnEnterVehicle(PlayerController)`;
- the vehicle retains the driver through `VoyageVehiclePawn.GetDriver()`;
- while occupied, the Blueprint moves the `VoyageCharacter` to its
  `CharacterLocation` transform and faces it to the seat rotation;
- exit calls inherited `OnExitVehicle()`.

`VoyageVehiclePawn` owns the reusable possession contract and exposes, among
other fields:

- `InputControls`, `DynamicPlayerInputComponent`, camera, spring arm, and
  camera attachment components;
- `bMountCharacterWithVehicle`, `bAutoPossess`,
  `bPreserveControlRotation`, driver class/reference, and
  `DriverEnterRelativeLocation`;
- look rates, zoom rate, camera pitch limits, and spring-arm min/max lengths;
- possession state and enter text.

The forklift already binds `IA_LookRight`, `IA_LookUp`, `IA_Zoom`, and a
vehicle exit action. Reusing the vehicle lifecycle is therefore much safer
than freezing the ordinary character and manually switching view targets.

### First-person camera boundary

`VoyageVehiclePawn` exposes both `bAllowFirstPersonCamera` and runtime
`bFirstPersonCamera`, but none of the three inspected vehicle Blueprints is a
validated first-person example:

- `BP_Forklift_Possesable` uses its spring-arm third-person camera and sets
  `bAllowFirstPersonCamera=false`;
- `BP_JetSki_Possessable` does the same;
- `BP_GyroCopter_Possessable` also uses a spring arm and sets the flag false.

Therefore those flags prove a native camera branch exists, but do not yet
prove its attachment, activation, possession, or control-rotation behavior.
The cannon must own a dedicated camera at the optical-axis transform and must
not inherit the forklift's third-person framing as its intended view. A later
game probe must determine whether setting the native first-person flags safely
activates that camera or whether the cannon Blueprint must explicitly make its
optical camera active while possessed.

### Brass Telescope camera and HUD contract

The following contract was completely traced on the now-superseded build
`23962331` and must be revalidated on `24990438` before implementation.

`Brass Telescope` resolves to
`/Game/Data/Assets/Equipment/Hands/DA_Equipment_Telescope`, whose equipment
class is `/Game/Blueprints/Equipment/BP_Equipment_Telescope_C`. The Telescope
Blueprint is a thin child of `BP_Equipment_Binoculars_C`; its meaningful
specializations are:

- `StateKey = Telescope`;
- `OverlayWidgetClass = BP_ToolsOverlay_Telescope_C`;
- `InitialLockFOV = 5` and the only selectable FOV is also `5` degrees;
- a scope-specific post-process dirt mask;
- `bMeasureDistance = false`;
- an otherwise empty `BP_ToolAbility_Telescope_C` derived from native
  `EquipmentAbilityComponent`.

The camera mode does not possess another Pawn and does not create another
camera. `BP_Equipment_Binoculars` gets the owning `VoyagePlayerController`,
casts its camera manager to `VoyagePlayerCameraManager`, then applies two
separately keyed overrides:

- `AddOverridePPBlends(StateKey, PostSettings, 1, 10, 10, 0)`;
- `SetLockedFOV(StateKey, VoyagePlayerCameraFOVOverride{FOV})`.

Exit is symmetric: `ClearOverridePPBlends(StateKey)` and
`ReleaseLockedFOV(StateKey)`. The mapped FOV override struct contains exactly
one float field named `FOV`. Telescope's literal 5-degree FOV is much stronger
than the requested 5x optical magnification; for a 90-degree base view, the
rectilinear 5x target remains approximately 22.62 degrees.

HUD creation is a separate layer. The equipment creates
`BP_ToolsOverlay_Telescope_C`, adds it to the owning player's screen, and calls
`SetPlayerGameStateWidgetVisibility(PlayerController, 2)`. Cleanup restores
visibility with value `0`, removes the overlay from its parent, clears the
stored widget, and unregisters damage/overlay delegates. The Telescope widget
itself is a plain `UserWidget`, not a `VoyageBaseUserWidget`; it contains a
material-backed `ScopeMask`, black outer borders, a fade image/animations, and
an `Unequip` input hint.

The start/end producer is also explicit. Telescope's draw and holster montages
contain five AnimNotifies. Each obtains overlay holster slot index `4`, casts
the slot actor to `/Game/Blueprints/Interfaces/BPI_EquipmentActions`, then
calls `StartEquipmentHUD`, `StartEquipmentFOV`, `EndEquipmentHUD`,
`EndEquipmentFOV`, or `EquipmentCleanUp`. Those montages and the equipment
slot are lifecycle producers, not required parts of Harpoon. A future
possessable Harpoon operator can invoke the same camera/HUD consumer contract
from possession and unpossession lifecycle events.

The stock Binoculars parent also demonstrates a 0.5-second distance update via
the character's `InteractiveDetectorPointerComponent`, but Telescope disables
it. Do not copy that trace for Harpoon: it originates from the character
pointer rather than the cannon's optical axis and does not apply the exact
shark whitelist.

The user also identified the deployable in-game `Drone`: it can be taken from
inventory, dropped into the world, then entered; control transfers to the drone
and the view is through its camera. This is a stronger reference than the
forklift for the combined world-interaction, possession handoff, remote actor,
and first-person camera lifecycle. Inspect its exact actor/item identities,
entry provider, possession owner, camera activation, HUD override, exit/recall,
and teardown independently. Do not assume it uses the same interface or copy
its inventory/deployment path into the stationary Harpoon shell.

Current-build inspection identifies the concrete Drone path as
`/Game/Blueprints/Vehicles/BP_CameraDrone`, derived from native
`/Script/Voyage.VoyageVehicleCameraDronePawn`. Its item is
`/Game/Data/Assets/Items/DA_CameraDrone`, and its HUD override is
`/Game/UI/Game/HUD/WBP_VoyageIngameDrone`. The Blueprint exposes
`InputAction_EnterVehicle`, `OnDroneExit`, `SetDroneActive`,
`ToggleFirstPersonComponentsVisibility`, and
`GetInteractiveProvidedActions`; it carries the stock vehicle exit action,
look inputs, zoom input, a camera component, and a 10-120 degree FOV range with
90 degrees as default. The Blueprint does not newly list
`InteractiveInterface`, so that contract is inherited through its specialized
native parent rather than re-authored at this layer. This makes Drone strong
evidence for later camera/HUD/possession behavior, but not a safe parent to
substitute before its native CDO/default-subobject contract is reconstructed.
The later runtime discriminator proved only immediate spawn validity; it did
not independently establish delayed continuation or early destruction. Direct
`BP_CameraDrone_C` spawning has not yielded a usable Harpoon operator and must
not be extended by more speculative activation/visibility changes.

### Per-actor full HUD override

The missing full-HUD selection path is now statically identified:

- `BP_JetSki_Possessable` implements
  `/Script/Voyage.VoyageActorWidgetInterface`;
- its Blueprint event `GetHUDOverrideWidget()` returns
  `/Game/UI/Game/HUD/WBP_VoyageIngameJetSki`;
- `BP_GyroCopter_Possessable` implements the same interface and returns
  `/Game/UI/Game/HUD/WBP_VoyageIngameGyroCopter`;
- the JetSki widget derives from native
  `VoyageIngameGenericVehicleWidget -> VoyageInGameVehicleWidget`.

This is strong current-build evidence for a supported per-actor replacement of
the complete in-game HUD during vehicle use. The leading cannon path is to
implement that interface and return a dedicated optical HUD class, allowing
Voyage's existing possession/HUD switch to own creation, replacement, pause
behavior, and teardown. It is not yet a runtime-validated lifecycle contract.
Before generator or mirror work, confirm the exact declaring UFunction owner;
the implemented-interface evidence points to `VoyageActorWidgetInterface`, but
the extracted Blueprint function export does not serialize a `SuperStruct`.

The mapped `VoyageIngameGenericVehicleWidget` only adds generic resource
display fields and is not an optic widget. It is useful as proof of vehicle HUD
lifecycle, not as a visual template that must be copied.

### Complete current-build shark identity

Current package inventory finds exactly three gameplay shark Blueprints and
their matching `VoyageNPCData` assets:

- `BP_NPC_Shark` -> `DA_NPC_Shark`;
- `BP_NPC_RamShark` -> `DA_NPC_RamShark`;
- `BP_NPC_Shark_Laser` -> `DA_NPC_LaserShark`.

The class hierarchy does not provide a safe single shark base:

- ordinary and laser sharks are sibling children of
  `BP_NPC_Simple_Base_C`;
- ram shark is a child of ordinary shark;
- `BP_NPC_Simple_Base_C` is a general NPC base and would overmatch unrelated
  creatures.

Each inspected shark has a `VoyageModuleComponent` named
`PlayerModuleComponent`. Its `ItemAsset` is the exact corresponding
`VoyageNPCData` object listed above. The leading classifier is therefore an
exact whitelist of those three data-asset identities obtained from the hit
actor's module component. This is safer than class-name or display-name text
and covers every shark package present in the fingerprinted build. After any
build change, rerun the shark package/data inventory and update the whitelist.

## Leading architecture hypothesis

Use two explicit runtime roles rather than one stationary vehicle pawn:

1. a `VoyageModuleActor` shell is the item's `DroppedActor` and owns item
   identity, construction, persistence, destruction, structural collision,
   dynamic collision/auto-weld, the fixed base, and deck attachment;
2. an operator object based on `VoyageVehiclePawn` (or the narrowest confirmed
   vehicle subclass retaining `OnEnterVehicle`/`OnExitVehicle`) owns character
   entry, possession, input, camera, aim, trace, and HUD;
3. the operator is created or exposed only after the shell's construction
   lifecycle is stable; the earlier child-actor crash does not validate a
   particular spawn/attachment mechanism;
4. the shell or attached operator provides separate yaw and pitch pivots, with
   unlimited wrapped yaw and pitch clamped to -50/+10 degrees;
5. a dedicated first-person camera attaches directly to the optical/pitch
   assembly, with the third-person spring arm absent or inactive while occupied;
6. an explicit optical-axis component owns both trace origin and forward vector;
7. `VoyageActorWidgetInterface.GetHUDOverrideWidget()` returns a dedicated
   first-person cannon HUD;
8. shark classification uses the current-build exact `VoyageNPCData` whitelist
   and distance from the mounted character to `Hit.ImpactPoint`.

Physical stationarity is now game-validated specifically for the completed
module shell. Do not transfer that conclusion to the construction preview or a
future operator actor until each follows ship translation and rocking in game.

For a true 5x rectilinear view, do not blindly divide an arbitrary FOV by five.
Derive the sight FOV from the base camera FOV using the tangent half-angle
relationship, then validate the perceived magnification in game. For example,
a 90-degree base FOV corresponds to about 22.62 degrees at 5x.

Distance is measured from the mounted character location to the trace impact
point. The HUD update rate can be lower than the mouse-aim update rate; aim,
target classification, distance calculation, and rendering remain separate
layers.

## Known deceptive or bad paths

- Do not clone only `BP_Module_WindTurbine_Medium_New`: it is mostly a visual
  child and does not own the generic placement contract.
- Do not assume multiple inheritance from both `VoyageModuleActor` and
  `VoyageVehiclePawn` is required. The forklift proves that module item data
  can instantiate a pawn carrying module components.
- Do not collapse back to one pawn merely because generic placement accepts
  it. The direct pawn remained world-fixed, and adding Cyclone's exact
  auto-weld component was a confirmed no-op; the native module shell is the
  game-validated structural owner, while its future operator link remains a
  separate experiment.
- Do not treat `VoyageDynamicCollisionComponent.bAutoWeld` as Unreal physics
  `WeldTo` or as actor transform parenting. Native code consumes it inside
  `VoyageEffectSubsystem` after runtime detachable/dynamic-collision
  registration.
- Do not manually add `Detachable`, `WeldBase`, `WeldChain`, or `AutoWelded` to
  compensate for a failed probe. They are runtime protocol state; first locate
  the missing producer/registration step.
- Do not cook another Blueprint against the current empty
  `VoyageModuleActor` mirror. The native parent constructor creates three
  required named default subobjects, and the Cyclone Blueprint overrides the
  inherited module-component class rather than recreating that graph through
  SCS.
- Do not reproduce vehicle entry by only attaching/freezing the ordinary
  character and changing view target; that would bypass verified driver,
  input, camera, exit, and HUD lifecycles.
- Do not reuse the forklift or JetSki camera setup as the cannon view. Their
  inspected defaults explicitly select third-person spring-arm behavior.
- Do not treat `bAllowFirstPersonCamera=true` alone as a proven solution. The
  native branch exists, but no inspected stock vehicle exercises it.
- Do not patch or hide the global game HUD when the actor-level
  `GetHUDOverrideWidget()` path can supply the full possession-scoped HUD.
- Do not identify a shark by display name, mesh name, localized text, or child
  index. Use a confirmed class/base class/interface or stable gameplay data
  identity.
- Do not classify every `BP_NPC_Simple_Base_C` as a shark; ordinary and laser
  sharks only meet at this general NPC base.
- Do not classify by a package/class name containing `Shark`. Use the exact
  current-build `VoyageNPCData` object whitelist and renew it after updates.
- Do not trace from the player camera if the rendered optic/barrel can diverge
  from it. The optical-axis component must own the trace.
- Do not treat a free `AddToViewport` overlay as a final HUD architecture. It
  can outlive possession or remain visible under pause/menu state.
- Do not directly inherit a Harpoon operator from the canonical
  `/Game/Blueprints/Vehicles/BP_Forklift_Possesable` path while DonkLift is
  installed. That path is itself a mod replacement, so Harpoon would acquire
  an undeclared cross-mod parent. If the Forklift contract survives current-
  build revalidation, use a Harpoon-owned, fingerprinted relocation of the
  clean stock original and never package the editor placeholder as its parent.
- Do not couple yaw/pitch input, target trace, and HUD mutation in one first
  experiment. Validate each producer/consumer boundary separately.

## Next discriminating research steps

Historical plan, superseded by the active restart section. In particular,
mapping generation is not the first step and build 24990438 is no longer the
installed fingerprint.

1. Obtain a fresh build-`24990438` `.usmap` through the temporary documented
   `VoyageMappingsDumper` workflow, then remove the dumper/UE4SS before any
   loader-free runtime test.
2. Re-run the exact Cyclone shell/item, Telescope/Binoculars, Forklift,
   `VoyagePlayerCameraManager`, `VoyageModuleActor`, interaction, HUD-interface,
   and shark identity inspections. Promote only unchanged contracts; revise
   every mirror/provenance entry that moved.
3. Remove or quarantine the old installed Harpoon container after explicit
   closed-game confirmation. Do not cook or install a new package until the
   source registry names the new fingerprint and all serialized native layouts
   used by the generator have been renewed.
4. Compare three operator-parent choices on the new build: a Harpoon-owned
   relocated clean Forklift parent, another lighter stock possessable, and a
   corrected minimal native vehicle mirror. Reject any canonical parent that
   creates a dependency on another installed mod.
5. After choosing the operator owner, retain Telescope's separated camera/HUD
   consumer pattern: possession entry, first-person optical camera, keyed
   camera-manager FOV/post-process, possession-scoped custom HUD, and symmetric
   exit cleanup. Validate each layer independently before shark tracing.
