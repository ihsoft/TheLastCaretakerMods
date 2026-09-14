# HarpoonCannon model backlog

## Restart / current state

- **Accepted working model: railgun C v4**, approved by the user on 2026-09-13.
  Hold its geometry until the user requests the next refinement.
- Durable source: [models/HarpoonCannon](../models/HarpoonCannon/README.md).
  OBJ/MTL, exact baseline hashes, generator, audit and direct renderer live there.
  The authored mesh is independent of game-derived resources and Unreal.
- Reproducible local images/audits live under ignored
  `artifacts/modeling/HarpoonCannon/`. See the model README for commands.
- This is source-model acceptance, not Unreal or real-game validation. UVs,
  final materials/smoothing, motion clearances and in-game appearance remain open.
- Model work must preserve the gameplay task's existing source, collision,
  camera/HUD/input, package and installation. Do not ping its agent during this
  independent phase. Gameplay decisions belong to
  [the logic backlog](harpoon-cannon-backlog.md).
- After compaction, read this restart state and relevant sections below before
  editing. Old archived task history is evidence, not current instructions.

## Accepted source and shape

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
