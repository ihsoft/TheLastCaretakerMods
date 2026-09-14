# HarpoonCannon model backlog

## Restart / current state

- **Accepted working model: railgun C v4**, approved by the user on 2026-09-13.
  Hold its geometry until the user requests the next refinement.
- User authorized committing the current model contract, including the rear-breech
  sight point. This checkpoint includes model-owned files only; gameplay changes
  and runtime camera validation remain with the logic task.
- Durable source: [models/HarpoonCannon](../models/HarpoonCannon/README.md).
  OBJ/MTL, exact baseline hashes, generator, audit and direct renderer live there.
  The authored mesh is independent of game-derived resources and Unreal.
- Runtime importer entry point: `models/HarpoonCannon/runtime-model.json`, schema
  1. Model-owned relative paths, source hashes, role selections, hierarchy and six
  ammo instances; regenerate with `prepare_handoff.py --write-runtime-descriptor`.
  Default invocation checks it for drift. Logic owns consuming this descriptor;
  no gameplay generator/source/collision changes are part of this handoff.
- Reproducible local images/audits live under ignored
  `artifacts/modeling/HarpoonCannon/`. See the model README for commands.
- Logic owner reports user confirmation of the new model/articulation and saved
  nonzero aim after exit (shell04/station03 test). This is bounded runtime evidence,
  not validation of all motion clearances, UVs or final materials/smoothing.
- Optional schema-1 `sight` is now **(-62,0,47)** cm under pitch, optical forward
  +X, no rotation override. Source **(-80,0,154)** is above the rear breech with
  5 cm top clearance. User replaced the muzzle-point request; do not reuse it.
  Logic integration/runtime sight framing and camera near-clip remain pending.
- Model work must preserve the gameplay task's existing source, collision,
  camera/HUD/input, package and installation. The user explicitly permits replies
  to direct requests from the logic agent; unsolicited pings remain out of scope.
  Gameplay decisions belong to
  [the logic backlog](harpoon-cannon-backlog.md).
- After compaction, read this restart state and relevant sections below before
  editing. Old archived task history is evidence, not current instructions.

## Accepted source and shape

Integration handoff prepared on direct request from the logic task and delivered
after the user explicitly authorized replies. Reproduce with
`models/HarpoonCannon/prepare_handoff.py`; local output is
`artifacts/modeling/HarpoonCannon/integration-v4/handoff.json`.
It covers all 83 objects exactly once: 8 base, 27 yaw, 42 pitch and 6 independent
ammunition rods. Yaw origin (0,0,0); pitch origin (-18,0,107) cm; subtract this
origin only from pitch mesh vertices and restore via component translation.
Rod base origins are (-106+15.5*i,-86,58), i=0..5, under yaw. Local rod meshes
match within 1e-6 cm, allowing one reused mesh with six independent components.
The logic owner requested yaw +/-40 degrees from installation orientation;
input and bounds belong to that task and are not encoded in the mesh.

Source hash, topology, partition coverage and neutral reconstruction checks pass.
At the initial handoff, the importer needed new path/allowlists/pivot; it accepted
v//vn but ignored normals/UVs and used default slot materials. No gameplay source
was edited here.
On the logic owner's subsequent direct request, replaced the proposed hardcoded
integration inputs with a model-owned runtime descriptor contract. Stable roles
base/yaw/pitch plus ammo prototype and ammo01..ammo06 do not depend on OBJ names.
Schema is documented in the model README; the ignored diagnostic handoff remains
available but is not the runtime input. No absolute paths or version-specific
artifact identity occur in the runtime descriptor. Source hashes/geometry unchanged.
Current descriptor SHA-256: `4BA7E56832299F7668AD2A8795E6011BC647FEF3DA75F670C1A796FF8404277C`.
The exact schema and completed worktree files were delivered to the requesting
logic task. Descriptor/handoff-script work is included in this user-approved
model-contract commit checkpoint.
Validation: default regeneration check matches byte-for-byte; 83-object coverage,
shared prototype/material/topology and neutral hierarchy reconstruction pass.
Negative checks reject wrong pitch/ammo offsets, omitted objects and absolute
source paths. `git diff --check` passed (only normal Windows EOL warnings).
Tool-use report: source-only Python audit/descriptor generation and read-only Git;
no game pipeline, fallback, runtime validation or uncovered reusable operation.
On direct logic-owner request, added optional `sight` without changing schema 1,
mesh selections, origins, hierarchy, ammo or OBJ/MTL. The user subsequently
replaced the muzzle location with a rear-breech eye point. Actual OBJ top panel
spans X=-86..-22 and tops at Z=149. Inset 6 cm from rear gives X=-80, Y=0;
nearby upper envelope within +/-10 cm XY is Z=149. Sight source Z=154 clears
the panel by 5 cm, housing by 12 cm, rear capacitor bands by 8 cm. Subtract pitch
origin (-18,0,107) once to get local (-62,0,47). Forward +/-5 cm Y strip retains
at least 5 cm clearance by conservative AABB test in neutral assembly. Optical
+X is parallel to, and 43 cm above, bore axis; it is not muzzle-convergent.
The side optic Y=-47..-33 is outside this central strip. Camera FOV/near-clip and
moving non-pitch parts remain runtime checks; source clearance is not full view
validation. Reproduction/hashes/source audits pass. Tests reject former muzzle
point and on-panel/insufficient-clearance points; absent optional sight is valid.
Geometry/generator/collision/packages untouched; source-only tools, no runtime
sight test or new reusable tooling gap. Logic owner reports the tested
shell04/station03 pair was removed; installation remains entirely logic-owned.
The decorative yaw actuators are not rigged telescoping links; full motion and
camera clearances remain unvalidated. Complete handoff includes these limits.
No game-asset/release operation or runtime test occurred; tool coverage N/A.

Files: `models/HarpoonCannon/harpoon_cannon_railgun_c_blockout_v4.obj` and `.mtl`.
Exact SHA-256 values are owned by `models/HarpoonCannon/baseline.json`:

- OBJ: `6E17C6807C0E7B3470D053A1FAEE59C5419A09DF184CADCC52FA4BC18364790F`.
- MTL: `D87428030929573105454C93DC42B5E77A22C2DDC50E697C498E2F259412D7DD`.

The user chose a grounded near-future electromagnetic deck gun firing solid
metal spikes. The older mooring-connector launcher is a different, superseded
design; its game-rendered probe remains under gameplay ownership.

- +X firing, +Y right, +Z up; centimeters. Fixed lower mount, rotating yaw
  assembly and pitching accelerator retain independent group identities.
- Six separately named `HC_RG_AMMO_Rod_01..06` objects stand in an open basket
  with thin retainers and individual guides/cups. Rods have 9 cm diameter,
  68 cm shaft and 18 cm tip. Empty positions remain recognizable. The chambered
  rod has the same 86 cm total length. Inventory/state/visibility logic is pending.
- Five top covers align with five orange panels on each side at
  X=`20, 54.5, 89, 123.5, 158 cm`. First station clears the elevation trunnion.
- Accelerator retains the accepted 10% length increase. Rail clearance is 10 cm.
  The muzzle is a closed hollow chamfered housing with thin orange side guards:
  36 x 36 cm rear, 30 x 30 cm nose, 10 x 10 cm passage; terminal X=190.4 cm.
- Electricity inlet is centered at X=0, Z=22 on the fixed mount. Its plate
  penetrates the lower housing and the socket body overlaps the plate. It is
  still an authored placeholder for the game's standard electricity assembly.
- Authored solids retain intentional assembly intersections. They have not been
  boolean-unioned or validated for all pitch/yaw/camera/collision clearances.

## Validated source behavior and pitfalls

- 83 solids pass closed-edge incidence, winding, positive volume, triangle area,
  face planarity, rectangular-box coordinates and magnet alignment checks.
  Audit repeats after exported OBJ readback; explicit face normals are exported.
- Direct renderer uses perspective-correct per-pixel depth and backface culling.
  Hero/left/right/rear/front order-invariance checks each change zero pixels
  above one color level at 480 x 320. This is bounded source-render evidence.
- The old painter renderer sorted triangles by average depth without a depth
  buffer. It produced false holes, smeared rails and warped-looking flat boxes.
  Removing wireframe lines did not fix it. Do not reuse that rendering approach.
- Real coincident surfaces also existed at capacitor front-band/body end caps
  and basket bottom/guards. V3 recessed band ends and fitted the basket bottom
  between guards. Guard/handle boxes became single solids. The initial reversed
  draw-order test found 478 changed pixels; repaired geometry reduced this to zero.
- Image-generated presentation art inherited defects, changed geometry and
  repeatedly failed to show six rods. The user rejected it as model evidence.
  Use concept art for shape references only; all model reviews use direct renders.
- Do not equate topology/OBJ checks, compile, cook or package verification with
  validated in-game rendering. No v4 Unreal import/runtime claim has been made.

## Stock asset reuse and historical construction preview

Last model-owned fingerprint observation (2026-09-10): Steam build `25191271`,
EXE SHA-256 `747DC2553F7E68D8EA7ED0B2E0CAC6D08943EA3F50DD6ED822E9293E0B45F58B`.
This is a historical observation, not a current compatibility claim.

Stock catalog candidates are `/Game/AssetSets/Sockets/Electric_Socket_In`,
`Electric_Socket_Base`, `Electric_Socket_Indicator_In`,
`Electric_Socket_Indicator_NoConnection`, `Electric_Socket_Latched`, and
`Electric_Socket_BlackMetal`, plus
`/Game/Blueprints/Modules/BP_AttachmentVirtualSocketActor_Electricity` and
`/Game/Data/Assets/ModuleSockets/DA_Socket_ElectricData`.
Exact mesh/material/state ownership was not confirmed: the summary tool reported
no reviewed mapping for that fingerprint at the time. Resolve the current game
fingerprint/mappings through the documented tools before resuming stock research.

The old gameplay proxy's construction preview showed luminous points/edges with
little filled silhouette. Adding base simple collision did not fix it; preserve
that collision for Q cancellation. Historical inspection found
`BP_Location_Fabricator.UpdateVisualizationItem` replacing copied-mesh materials
with a dynamic `/Game/Materials/Crafting/FX_DissolveOverlay_Mat` instance.
The remaining hypothesis concerns UVs/normals/tangents and forced hard edges;
it is not proven. Revalidate the contract before a coordinated test. Do not
repeat collision-only probes or claim the source renderer fixes this game issue.

The historical Whisper parent-path replacement failed before visual validation;
extra mesh packages and stock-head reference were exonerated only for that crash.
Use the gameplay owner's current carrier/installation contract. Do not revive
old lock, package, installation or test instructions from archived model history.

## Cleanup checkpoint: 2026-09-13

Consolidated nine experimental scripts into three self-contained source files
under `models/HarpoonCannon`. Exact accepted OBJ/MTL and README/hash registry are
prepared for a scoped commit. Old source imports are no longer required.
Generated images and reports stay ignored; old gameplay handoff files and the
chosen concept C reference remain locally preserved. No gameplay files changed.

Removed 84 obsolete model/inspection files (22,265,316 bytes), including failed
renders, superseded v1-v3 exports, old scripts/caches and 16 model-owned inspection
directories. Before deletion, all archived entries were SHA-256 checked. Recovery:
`C:/Users/IH/AppData/Local/Temp/HarpoonModelCleanup-20260913-164720/model-work-before-cleanup.zip`;
the adjacent `receipt.json` records exact original paths/hashes. The full previous
backlog is in that archive; temporary storage is not a permanent history store.

Tool report: source consolidation, baseline copy/hash check, regeneration/readback,
direct render/order checks, exact-path archive/cleanup. Baseline bytes match and
all five renderer checks pass. The cleanup helper initially needed an explicit
System.IO.Compression assembly load under PowerShell 5.1; corrected before archive
or deletion. Temporary consolidation/cleanup helpers and verification outputs are
removed after use. Game-asset/release coverage is N/A; no new general toolchain gap.

Commit preparation: the wrapper initially rejected OBJ/MTL CRLF as trailing
whitespace with `-text`. Scoped `cr-at-eol` attributes now recognize their line
endings while retaining normal whitespace checks and exact baseline hashes.
The failed attempt restored the index; no partial commit was created.
