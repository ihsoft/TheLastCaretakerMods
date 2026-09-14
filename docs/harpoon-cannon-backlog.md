# HarpoonCannon: restart / current state

## Current checkpoint — 2026-09-13

User accepted clean-hud-02 and requested baseline commit, then firing work.
Keep installed clean-hud-02 + visual-v5-palette-02 unchanged during research.
Current next step: stock turret StartFiring/ActivateAbility/CreateAbilityComponent
producer chain, projectile/hit consumer and model-defined muzzle. Separate first
shot/trajectory test from mooring cable, damage balance and ammunition persistence.
No firing implementation or new test installation yet; safety timeout remains20s.

### HUD cleanup checkpoint

CURRENT installed operator clean-hud-02 at05:12Z:
artifacts/harpoon-cannon/clean-hud-02/release-manifest.json.
RMB label is now exactly Toggle scope. Clean-hud-01 below is superseded and
removed via exact manifest; shell unchanged. Current operator recovery FIRST:
artifacts/installations/HarpoonCannonLifecycleProbe/20260914-051228-clean-hud-02-ac501b80/install-manifest.json.
Then current shell receipt below. Build/cook/tagged/container/readback passed;
clean HUD and circle are now user-validated. Twenty-second exit remains.

User accepted live-eye camera and explicitly accepted visual-v5-palette-02 as
working model. Geometry/material baseline registered by model owner. This is
visual/camera runtime acceptance, not firing/save/multiplayer validation.
Removed live eye observer graph/variables/text, hint-provider debug text; legacy
coordinator sink is retained privately but no longer added to viewport, so neither
top-right panel nor bottom test instructions render. Independent cooked summary
artifacts/asset-summaries/mod/20260914T050906652Z-ab672a52/summary.json confirms
no AddToViewport call in ModActor. Own action hints/stats/5x mask remain.
Added wide-only hollow-circle center marker; visual result still pending.
Native E/F8/20-second safety unchanged. User additionally requested RMB label
exactly 'Toggle scope'; clean-hud-02 is being prepared for this text change.
Superseded clean-hud-01 operator receipt:
artifacts/installations/HarpoonCannonLifecycleProbe/20260914-050918-clean-hud-01-32f6b592/install-manifest.json.
Current accepted shell receipt:
artifacts/installations/HarpoonCannonShellProbe/20260914-050913-visual-v5-palette-02-1f91521e/install-manifest.json.
Previous live-eye pair below removed5+4 files at05:08 through exact manifests.

### Accepted live first-person position + visual palette / recovery evidence

Accepted operator artifacts/harpoon-cannon/live-eye-01/release-manifest.json;
shell artifacts/harpoon-cannon/visual-v5-palette-02/release-manifest.json.
User requested next test after both camera proposal and model handoff. Operator
now copies observed exact FirstPersonCamera component world position on wide
entry/return and each occupied wide Tick BEFORE convergence trace. Enumeration
is restricted to original character CameraComponents and exact object name;
missing camera retains native eye fallback. No camera activation, re-possession,
character transform/visibility change. Station rotation/limits remain owned;
5x sight/mask unchanged. Diagnostic observer retained for this test.
Separate visual-only shell includes accepted v5 geometry and palette, with
unchanged roles/sight/collision. Model agent passed material/export/inline SM5/SM6
shader gates; runtime shaders/colors NOT yet validated. If model looks wrong,
diagnose shell separately from camera; accepted breech-shell-01 retained.
Build-Probe outside sandbox passed first attempt: build/generate/cook/tagged,
container and release gates. Common installers checked closed process and all
installed hashes. Config unchanged. Runtime camera and materials PENDING.
Next: exterior colored model screenshot; enter1x and rotate both axes, verify
no behind-head view; RMB5x/back checks mask and aim continuity; E exit/no save.
Restore after result OPERATOR FIRST:
artifacts/installations/HarpoonCannonLifecycleProbe/20260914-045629-live-eye-01-07d250b7/install-manifest.json
then SHELL:
artifacts/installations/HarpoonCannonShellProbe/20260914-045625-visual-v5-palette-02-46635e76/install-manifest.json.
Previous observer pair removed5+4 files at04:55 through exact manifests below;
accepted payloads retained. No other mods changed.

### Completed eye observer / recovery evidence

Superseded operator: artifacts/harpoon-cannon/eye-observer-02/release-manifest.json
SHA256 4044674E52504EE1C9947896730F0FBB26EF58A474F1B1B3FA1706BDDD161052.
User confirmed previous eye-parallax mask works; 1x camera is behind the head,
not accepted as first-person. Mouse appears to move the character: hypothesis,
not yet proof of eye-point motion. No further camera/body/mask changes in this test.
Read-only observer independently runs after occupied setup, resets per entry,
reports maximum root/native-eye drift in carrier space and live eye/camera/SK_Head
positions in character space. Enumerates actual CameraComponents; does not assume
first camera is first-person. Stock BP_FirstPersonCharacter_New SCS lists
ThirdPersonCamera; BP_Base_Character lists SK_Head. Native eye getter is not proven
to follow animated head. Head origin is likewise not an eye socket.
Next user check: two screenshots in1x, immediately after entry and after several
mouse rotations, then E exit. Twenty-second safeguard remains; no saving.
Build/cook/tagged/container and independent cooked function inspection passed;
runtime observation PENDING. Cooked summary:
artifacts/asset-summaries/mod/20260914T043159938Z-07ecfd0f/summary.json.
Installed pair exact recovery, OPERATOR FIRST:
artifacts/installations/HarpoonCannonLifecycleProbe/20260914-043315-eye-observer-02-514c7ed2/install-manifest.json
then SHELL:
artifacts/installations/HarpoonCannonShellProbe/20260914-043310-breech-shell-01-a6217b5e/install-manifest.json.
Previous pair below removed5+4 files via exact manifests at04:30Z. Recoverable
payloads retained. No unrelated mod/config changed. Game closed gates/readback passed.

### Superseded eye-parallax test / recovery evidence

eye-parallax-03 is superseded. Below receipts are recovery only.
User accepted RMB/basic functionality but rejected1x breech viewpoint and
reported5x mask absent. This test uses native original-character
GetActorEyesViewPoint location sampled on entry/return to1x, camera attached to
the same fixed cannon carrier. No character re-possession/body mutation. Eye
yaw/pitch drive wide camera separately from gun aim; trace max100000cm ignores
character/shell/self. Hit or end-point becomes target; two fixed geometry updates
account approximately for moving sight, inverse-transform into installation
coordinates and clamp yaw/pitch. On RMB to5x converge once more then snap camera
to sight, retaining gun angles. Returning1x starts gaze parallel to current gun.
Near targets, sector saturation, body clipping and real-game alignment unvalidated.
No firing/saving; E/F8/20-second exit and100%/35% sensitivity retained.

Mask diagnosis: cooked UImage Brush.ImageSize was0x0. Final candidate explicitly
serializes2048x512; cooked HUD confirmed. SetDesiredSizeOverride was rejected
during preparation: engine implementation only touches live Slate, not asset data.
Final build/cook/tagged/exact11-package/container and install readback gates pass.
Current operator artifacts/harpoon-cannon/eye-parallax-03/release-manifest.json
SHA256 D9647E9A8283C6129BE2468FDCCD42BE55E039B4A9D74BFF07746B3C2F48B231.
Same accepted shell breech-shell-01, config unchanged. RUNTIME PENDING.
After result/closed game restore OPERATOR FIRST:
artifacts/installations/HarpoonCannonLifecycleProbe/20260914-040639-eye-parallax-03-725fde68/install-manifest.json
then SHELL:
artifacts/installations/HarpoonCannonShellProbe/20260914-040635-breech-shell-01-0c7352bb/install-manifest.json
Previous RMB pair removed5+4 files at03:59 using exact manifests; no other mods
changed. Next check:1x genuinely from eyes; look at a distinct near/distant object,
RMB and compare centered object; mask visible; exit normal. No save.

### Superseded RMB test / recovery evidence

CURRENT: rmb-optics-01. Default1x/100% mouse, RMB toggles5x/35%, reticle/range/
name only5x; authored circular soft-edge black mask. Native E exit, shell,
sight transform, yaw +/-80 and20-second safety preserved. 1x retains accepted
breech camera (unaided mode), not character possession or a moved eye socket.
F9 removed. IA_HarpoonZoom maps RightMouseButton; GetProvidedActionsBP returns
exit+zoom through existing native hint widget. Authored2048x512 alpha mask in
ScaleToFill stays circular for landscape aspects up to4:1. Exact inventory11
packages (old9 plus action/texture), all tagged. Build/cook/container and
independent rmb-audit.ps1 passed. Gameplay/visual result PENDING.
Next: entry1x without crosshair; RMB hint/toggle,5x mask/reticle/slower mouse,
repeat toggle and E exit. Check character stats remain visible. No saving.

Operator artifacts/harpoon-cannon/rmb-optics-01/release-manifest.json
SHA256 E293D4EFCC778940853932B977D15CECFC587640E2CB48BC804EC834D171C973.
Unchanged shell breech-shell-01. Closed-process/readback install gates passed.
CURRENT restore OPERATOR FIRST:
artifacts/installations/HarpoonCannonLifecycleProbe/20260914-032947-rmb-optics-01-8f182125/install-manifest.json
then SHELL:
artifacts/installations/HarpoonCannonShellProbe/20260914-032941-breech-shell-01-f596e587/install-manifest.json
Config migrated after exact old hash guard; backup
artifacts/harpoon-cannon/rmb-optics-01/previous-HarpoonCannon.ini.
OpticsMousePercent=35 replaces legacy MousePercent (ignored); limits unchanged.
Config hash5AE1D940E971C1BF78ADA6B0563F539399DA234AA73AB31CFBE4100C321F427C.
Zoom-toggle-01 user accepted; prior pair removed5+4 at03:21 through exact
manifests below. Those paths are recovery evidence ONLY, not active targets.

### Accepted zoom comparison / recovery evidence

User accepted breech-shell-01/breech-station-01: works; screenshot shows a broad
solid strip along the bottom. Barrel/near-clip attribution is a hypothesis, not
confirmed geometry identity. Runtime settings reload was not separately tested.
Both completed test installations were restored at01:53/01:54 (5+4 removed,
zero predecessors). Config file retained. No unrelated mod changed.

New requested discriminator: F9 toggles1x/5x, entry resets5x. Only camera FOV
changes; baseline on-foot FOV versus existing angular5x. Camera transform,
articulation, sensitivity, range origin and native E exit unchanged. F9 polling
is deliberately a temporary diagnostic, not a new production action/HUD contract.
Zoom label updates in own HUD; independent sequence prevents trace misses from
blocking zoom. No model work needed. Build/generate/cook/tagged/retoc and prior
breech audit passed. New runtime test PENDING, no saving,20-second safety retained.

Installed with closed-process gates and matching readback:
- same shell artifacts/harpoon-cannon/breech-shell-01/release-manifest.json
- new operator artifacts/harpoon-cannon/zoom-toggle-01/release-manifest.json
  SHA2564EB6A24210AC2E2789BB0EADB58AA98A3BF3008ECAA2BB0FA69A0557834EB72B
After result/closed game restore OPERATOR FIRST:
artifacts/installations/HarpoonCannonLifecycleProbe/20260914-015928-zoom-toggle-01-40731e74/install-manifest.json
then SHELL:
artifacts/installations/HarpoonCannonShellProbe/20260914-015923-breech-shell-01-a8ddb459/install-manifest.json
Next user check: F9 changes view and label in both directions without changing
aim; compare bottom strip/barrel visibility at1x and5x; native E exit.
The following breech preparation section is retained recovery evidence, not
the active installation instruction. No commit requested.

The Git commit containing this baseline note captures the accepted HC33 source,
authored model inputs, shared generator dependencies, interaction observer and
consolidated documentation. Recovery ZIPs remain immutable ignored artifacts;
their older sourceCommit fields are original build provenance, not rewritten
to the baseline commit. This commit is not a new build or production release.
Unrelated and mixed-owner shared-document edits remain outside its scope.
Pre-commit whitespace gate removed trailing empty lines in six source/config/
ignore files only. No compiled statement or runtime value changed from HC33.

HC33 candidate01 is user-accepted: contextual entry, native vehicle handoff,
stationary deck-mounted operator, own first-person x5 camera, mouse, own HUD,
native exit, restored character actions, generic first-hit name/range.
Mouse scalar1.024 is 20% below HC32. Character stats intentionally remain.
Shark-specific HC33 testing was unavailable; historical HC27 validated Talon Shark.
This is stable operator/optics, NOT firing, cable or save-safe production gameplay.

The railgun model pair was subsequently user-accepted (including retained
non-centred articulation after exit), then removed. The breech-sight pair below
is now installed; its runtime result is pending.
HC33's immutable package still includes diagnostics, F8 fallback and20-second exit.
Cleanup did NOT silently build a different diagnostics-free artifact.

## Next action

Active request: mouse40% original1.28 (=0.512), yaw +/-80, runtime-file tuning,
camera above breech looking parallel to barrel; distance from that camera.
Descriptor sight is pitch-local(-62,0,47), source(-80,0,154)cm: 5cm above
breech panel. Descriptor SHA256
4BA7E56832299F7668AD2A8795E6011BC647FEF3DA75F670C1A796FF8404277C.
Geometry/6 separate ammo components and Q box164x164x12 at(0,0,6) unchanged.
Model AABB clearance is not near-clip/runtime visual validation.
Config keys/defaults: MousePercent=40, YawLimitDegrees=80,
MinimumPitchDegrees=-50, MaximumPitchDegrees=10. Read every entry via current
fingerprint-validated native LoadFileToArray, defaults on missing/invalid lines.
Config created without overwrite at
C:/Users/IH/AppData/Local/Voyage/Saved/Config/HarpoonCannon.ini
SHA256 A33A97115754346D574505108B16AACED016FAB1FB20CBAA505240201EB2E1EE.
Keep this preference file after test removal; it does not load any actors.

Build/generate/cook/exact five/nine inventories/semantic/container checks passed.
Independent breech-audit.ps1 verified native reader owner, defaults, two scalar
paths and absence of camera actor-relative rotation. Cooked attachment targets
HarpoonSightComponent with snap location/rotation, no weld; range subtracts
camera component location from ImpactPoint. Runtime file read and view pending.
Next user check: contextual entry, unobstructed breech view/x5, slower mouse and
expanded sector, hit name/range, native E exit. No saving. To prove file reload,
after this test edit a value while outside cannon and enter again, no rebuild.

Installed2026-09-14T01:40-01:41Z with closed-game gates and matching hashes:
- shell: artifacts/harpoon-cannon/breech-shell-01/release-manifest.json
  SHA256 868F063EC78922D103C00E0927208867F9087FADD319ACB901310F849AB8781A
- operator: artifacts/harpoon-cannon/breech-station-01/release-manifest.json
  SHA256 74ED7072992B565A6F44C49DB0DB39CAADABA6169B3C0C720065D487A302E119
After result and closed game restore OPERATOR FIRST:
artifacts/installations/HarpoonCannonLifecycleProbe/20260914-014137-breech-station-01-1db3ee79/install-manifest.json
then SHELL:
artifacts/installations/HarpoonCannonShellProbe/20260914-014056-breech-shell-01-70990ea7/install-manifest.json

Accepted recovery: railgun-shell-04 / railgun-station-03 immutable manifests
(SHA256 D149AC038410CD02F6BD90A692FCE9F63374242F563A17B769F297BDD1E1C1EF /
6B3A327341C80C23EA0F2D1B845C9347B750555AFBBBEBE6AFC3640305F837A6).
Their installation manifests at 20260914-005938 and005910 have successful
restore receipts dated011136 and011140 respectively: removed5+4 files, no
predecessors restored. Other mods untouched. No commit this turn.

Documentation boundary: candidate history is excluded from repository-level
README/tool routing. The interaction observer's HC30/HC31 details belong here:
HC30 compared stock helm acquisition with cannon acquisition in one session;
HC31 added HarpoonEntryInteractBlocks, sampled once after spawn against Block.
It is not a continuous collision-filter read. False before readiness is
inconclusive. Provider/callback markers likewise describe their own instrumented
paths, not universal detector state. Shared guidance retains only the verified
reader interface, fingerprint/serial checks and non-atomic observation limits.

Resume feature design from this accepted baseline, not old probe instructions.
Before the next runtime candidate, define one useful discriminator for the next
core function (shot / stock mooring cable integration) or explicit productionization
of the operator lifecycle. Save/persistence, death/travel, multiplayer and long
sessions remain open. Do not install another experiment merely to test cleanup.
If a new test yields no useful information, return to producer/consumer research
after one or two no-ops. The user's manual-test budget is limited.

## Read only what is needed

- [Architecture and consolidated failed approaches](../mods/HarpoonCannon/RESEARCH.md)
- [Local rules](../mods/HarpoonCannon/AGENTS.md)
- [Native entry and interaction evidence](../mods/HarpoonCannon/VEHICLE_ENTRY_RESEARCH.md)
- [Provenance](../mods/HarpoonCannon/GAME_DERIVED_SOURCES.md)
- [Pipeline observations and recovery](../mods/HarpoonCannon/PIPELINE_OBSERVATIONS.md)

Model backlog is separately owned. Do not contact its agent without model work.
Historical reports and ZIP evidence are not active instructions.

## Current fingerprint

Rechecked 2026-09-13T23:05:01Z, processCount0:
Steam25191271; executable SHA256
747DC2553F7E68D8EA7ED0B2E0CAC6D08943EA3F50DD6ED822E9293E0B45F58B.
Game UE5.8 / editor5.8.2.
Reviewed mapping mappings/Voyage/steam-25191271-ue5.8/Voyage-25191271.usmap:
E25620FB2D0674B26A3E4EF059AC2C5AF58E28CE1B4F07307860F7FADB9A0F68.
Changed fingerprint invalidates all consumed reconstructed contracts.

## Retained exact recovery artifacts

Paths below are relative to artifacts/harpoon-cannon; SHA256 identifies the manifest.

| Identity | release-manifest.json SHA256 |
| --- | --- |
| hc33-candidate-01 | 265B8C4787E0186A257ECF6ABEBE4C05503F5E2179EFAC2B6A9F146EC36053E6 |
| hc32-candidate-03 | 126C14D735977617C4C6B72CCB28AD1FAB343D8AE815B7F2B4DB752340228A07 |
| shell-25191271-candidate-01 | 0993AB5F4EA5317856D83E24638654BB71392963DE45C05998CA106F3BF9CD26 |

HC33 payload:
- ucas13CAB7A3FC2AF61D860DE06B7EF1041612D1E6CEEB6DE3823F01338628C884BD
- utoc37CD23FA33B46254A5638D3CAE14866F6DCE62B327E40A302D747FA0E24EA4E9
- pak75E7144577253917F6DA7312EF5E585B12FB728226A22B0938323751A6B555CD
- autoload61CB50478478F07439F3FAB6DC973E3E0CBEF64B11DE8D27B66B5222A9094A8A

HC33 baseline audit66 and HC32/33 comparison22 passed before runtime acceptance.
Retained hc32-audit.ps1 and hc33-compare.ps1 are checkpoint-bound evidence helpers.
Build/inspect/cook/verify alone never promoted this to game-validated.

## Completed removal

Probe first:
artifacts/installations/HarpoonCannonLifecycleProbe/20260913-223612-hc33-candidate-01-04e80012/restore-manifest-20260913-230627-dd00128b.json

Shell second:
artifacts/installations/HarpoonCannonShellProbe/20260913-223547-shell-25191271-candidate-01-9d19de77/restore-manifest-20260913-230716-56a6bf8c.json

Five probe files and four shell files removed, no predecessors, other mods untouched.
Use public installer/restorer with exact manifests; check closed game immediately
before mutation. Install shell first, probe second; restore in reverse order.

## History location

The pre-cleanup chronology, logs and old candidates have verified archives at
artifacts/archive/harpoon-hc33-20260913. See cleanup receipt for hashes and recovery.
Their old repository-relative artifact paths are ZIP entry names, not live paths.
The accepted sources and payloads remain active. No gameplay source refactor was
bundled with cleanup; historical Probe identifiers remain where used by HC33.
