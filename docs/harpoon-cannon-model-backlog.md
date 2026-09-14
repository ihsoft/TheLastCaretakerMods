# HarpoonCannon model backlog

## Restart / current state

- **Accepted working baseline: railgun C v5 + palette**, visually game-validated.
  Logic owner reports user tested `live-eye-01 + visual-v5-palette-02`, said it
  works well, supplied a screenshot with colored materials and explicitly
  authorized fixing the new model as the working variant. This accepts visuals,
  not firing, save persistence, multiplayer or every camera/motion clearance.
  Original accepted v4 source remains intact with `baseline.json` hashes.
- New geometry-only **v5 candidate** implements the user's screenshot feedback:
  seated side/top magnets and muzzle wall 2.5x thinner. Reproduce with
  `models/HarpoonCannon/refine_geometry.py --render`; output is isolated under
  `artifacts/modeling/HarpoonCannon/geometry-v5/`. `--adopt` added sibling v5 sources
  and changed ONLY source identities/hashes in runtime-model.json. Roles/origins/
  sight/ammo/geometry hierarchy preserved. Visual runtime acceptance is recorded
  above; do not start further model work without a new request.
- User authorized color implementation after confirming the grey-default-material
  diagnosis. Nine-slot palette and editor material factory are prepared under
  model-owned paths and now integrated through the explicitly delegated shell
  material hook/provenance/shader-cook/material-validator scope. Shared build/cook
  has been released to the logic owner. No station/config/installation changes.
- **Visually game-validated shell:** `artifacts/harpoon-cannon/visual-v5-palette-02/`
  `release-manifest.json`, SHA256
  `4C4BE69B99C99EA159F445B7CCA36FC1FBE94AA6ECB23A2DB288D383B4153BF8`.
  Exact five-package inventory, material parameters/refs and prior shell/collision/
  role/sight gates pass. SM5+SM6 cooked, zero errors/warnings; colored appearance
  subsequently confirmed by the user's runtime test via the logic owner.
  Logic owner received manifest/hashes and owns install/test/rollback.
- User explicitly requested a commit of the working v5 + palette model. This
  checkpoint includes model-owned source/docs only. Mixed dirty mod generator,
  Build-Shell and Validate-Shell files remain under logic-owner integration;
  their material hooks are not included in the model-only commit. Prior contract
  commit: `af5602a`. No new build/install requested; preserve the active logic test.
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

### Geometry v5 (accepted working visual baseline)

- User screenshot evidence showed 4.5 cm side-magnet gaps and 5 cm top-magnet
  gaps, confirmed in actual v4 OBJ bounds. All five stations on both sides move
  inward 4.6 cm, retaining 0.1 cm overlap with unchanged rails. Top covers move
  down 5.1 cm, width 32 -> 21.8 cm; status plaques follow their supporting covers.
- Muzzle inner chamfered 10 x 10 cm contour and every X coordinate remain exact.
  Each outer outline corner moves toward its corresponding inner corner by 1/2.5:
  rear outside 36 -> 20.4 cm, nose 30 -> 18 cm, straight nose rim 10 -> 4 cm.
  Side guards are reduced/refitted to the same envelope; no floating remnants.
- 23 objects changed (20 magnet/status + 3 muzzle); 60 unchanged byte-equivalent
  vertex arrays, including the barrel rails/floor, base, ammo and breech. All
  faces/material identities and roles stay unchanged. Pivot/sight transforms
  pass the existing descriptor validator against the candidate.
- Source OBJ SHA-256: `9D23449F47795B9442D7B2C3FDA84125CD0C718009A5AD4431A6603ADC96F4AE`.
- Initial top width 22 cm created coincident rail-side faces in the 0.1 cm
  contact overlap. Hero reversed-draw-order check rejected it. Width 21.8 cm
  removed that coincidence; all five render order checks now change zero pixels.
- Tool report: standalone source-only refinement, topology/contact/thickness/
  preservation audits, exported readback and direct mesh renders passed. No
  game extraction/build/cook/package/install, AI-image edits, shared generator
  changes or new reusable game-tool gap during source preparation. The later
  `visual-v5-palette-02` runtime test confirms visual acceptance.

### Palette integration (runtime visual acceptance confirmed)

- Former C++ importer kept `usemtl` but assigned default grey surface. The only
  generator edit is include + material assignment; nine used MTL entries now have
  owned linear color data and masters/MICs in existing mesh packages. Geometry v5
  was adopted separately; descriptor values apart from source files/hashes unchanged.
- `prepare_materials.py --write` generates `material-palette.json` and
  `Unreal/HarpoonPaletteData.h`; default verifies outputs. Uses preview Kd^2.2,
  matching renderer, and PowerBlue emissive x0.4. Roughness 0.4 for shiny slots,
  0.65 otherwise; metallic 0 retains preview-like diffuse palette. No UVs needed.
- `Unreal/HarpoonPaletteMaterials.h` compiled/cooked successfully: 4 masters,
  20 MICs total across 4 mesh packages, no new package paths. Cook-only overrides
  in Build-Shell disable shared shader code (Game packaging config class) and
  enable D3D11 SM5 + D3D12 SM6. Shared config and game-global shader archives untouched.
- Final container hashes: pak `75E7144577253917F6DA7312EF5E585B12FB728226A22B0938323751A6B555CD`,
  ucas `071C8E15ADDBDD182F460BB37E132A82846B8893FF793819D0BCBED977477CD5`,
  utoc `4E9D9B6B822526FCF1A18BA81DB99D38659FE5EEA3F4DE704F37B647E8FAFAF1`.
  ZIP `HarpoonCannonShellProbe_visual-v5-palette-02.zip` SHA256
  `BC1B2BEEDC903B1255414B1E1D3C410352EE1262E5C86804C99B86002E99898C`.
- Build-Shell outside sandbox: compile/generate/cook/package/exact inventory/
  release manifest succeeded. Validate-Shell retains prior logic/collision gates
  and checks slot -> MIC -> owned master, all linear base/emissive/scalar values.
  First candidate stopped because validator expected package-qualified ObjectName;
  public Mod JSON showed `Material'M_HarpoonPaletteSurface'`. Fixed validator,
  rechecked, then made fresh candidate02 with both SM5/SM6. Candidate01 is not final.
- Unexpected `LoadedMaterialResources=[]` in inspector led to bounded CUE4Parse
  source inspection: provider.ReadShaderMaps defaults to skipping them. Do not
  claim absent shaders from that output. Cook log confirms both formats; actual
  rendering was subsequently accepted in the reported user test. No parser/tool
  implementation changed; this does not validate all RHIs/hardware combinations.
- Tool report: palette generation/drift/hash checks passed; existing source
  importer and local UE headers inspected for material-hook implementation.
  First guessed engine header path used legacy Classes/Materials (absent); actual
  UE5.8 headers resolved under Public/Materials. No game-derived input consumed,
  eligible operations went through documented Build-Shell, Validate-Shell and
  Get-VoyageAssetJson entrypoints (100% recurring operation coverage; two build
  attempts, one semantic recheck, one explicit JSON diagnostic). Model scripts
  excluded from metric. Model task did no install/restore; logic owner handled
  the runtime test and reported visual acceptance. Shader-map inspection
  not exposed by normal JSON is a limitation for handoff, not new tool work.
- Acceptance recording only: edited model README/backlog and checked their diff;
  no new geometry/material changes, build/cook, installation or commit. Functional
  firing/save/multiplayer remain outside this visual approval.

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
Current v5 descriptor SHA-256: `E7A68E5099EA422E02584191CC8B85620DBC5AE6C8AD51AF3A18977DF8A49337`.
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

Preserved original files: `models/HarpoonCannon/harpoon_cannon_railgun_c_blockout_v4.obj` and `.mtl`.
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
