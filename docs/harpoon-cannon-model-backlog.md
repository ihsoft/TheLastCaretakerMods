# HarpoonCannon model backlog

## Restart / current state

- User changed the authoring/handoff contract: GLB is authoritative; V1 starts a
  NEW model version line unrelated to procedural v4/v5 or upload filename v20.
- Registry: models/HarpoonCannon/model-source.json. File:
  models/HarpoonCannon/HarpoonCannon_V1.glb.
  SHA256 9FD66B3350239F4CE0E37E0B8BF4F8E9F033A3E22E8EE88CD2DBF9158578A5BD,
  463824 bytes, unchanged copy of the supplied source. Never regenerate it from
  old OBJ/geometry scripts. Source is user-selected, NOT game-validated.
- User explicitly authorizes V1 INCLUDING the socket reference in Git as the
  material baseline. Root rule has a narrow exact-file exception; registry pins
  the hash and original socket provenance. GLB lives alongside registry, unchanged.
  Do not strip the reference now. A future requested revision replaces it with
  native game socket integration at HC_PowerSocket; V1 remains historical source.
- GLB audit/direct Blender import: 94 nodes (all active), 77 mesh nodes, 72 mesh
  definitions, 11682 triangles, 13 materials, no textures/images/skins/animations.
  Dimensions in glTF axes ~2.993 x1.718 x1.715 m. Direct preview inspected.
- Critical changes: HC_Pitch local ~(0,1.07,0)m; HC_Sight under optic housing,
  world ~(-.70,1.63,0)m. Basket/rods now under PITCH despite YAW name prefix.
  Power anchor under base at (0,.25,.77)m. Preserve full nested matrices.
  Some old mesh names now empty nodes; don't recreate their former geometry.
- Frozen legacy schema1 descriptor/OBJ/palette/header bridge remains unchanged
  while logic owner coordinates migration. It is not V1-compatible. No build,
  cook, install, gameplay source or collision changes made here.
- Old procedural construction scripts retired for new authoring, retained only
  for legacy consumer dependency safety. Reusable exporter relocated to
  tools/glb/export_blender_glb.py; socket extraction method retained.
- Documentation: models/AGENTS.md, owning model README/registry, tools/glb/README.md.
  Older findings/checkpoints archived in harpoon-cannon-model-pre-glb-history.md;
  read historical sections only when needed, not as current workflow.
- After compaction read this section, then the relevant source/contract. User
  authorized placing V1 in Git; scoped source/docs/tools commit is now requested.
  Preserve other agents' dirty work and installed state.

## Next actions / gates

1. Gameplay owner received registry/audit/hash and breaking hierarchy details.
   Mod README/AGENTS now identify the frozen old bridge. Await a coordinated
   GLB-derived adapter; don't disguise old OBJ as V1 or assume fixed yaw ammo.
   Owner explicitly acknowledged: no running producer, no consumer migration/
   build/install now. Preserve Build-Shell, ModelRecipe.h, generator, Validate-Shell,
   runtime-model, OBJ/MTL, palette, prepare_materials and both palette headers.
   AutoloadProbe depends on HarpoonModelContract/runtime tags/sight, not GLB directly.
2. A derived conversion must handle full hierarchy/materials, preserve logic-facing
   node identities, and fail if a shader/resource feature cannot be represented.
   GLB and registry are the handoff, derived engine data is not a second master.
3. Before V1 in-game use: adapter geometry/role/sight/ammo/material verification,
   then real-game validation. No release compatibility inferred from file import.
4. When native power socket integration is requested, preserve the anchor and
   substitute original stock components/materials/logic under its owner. Reference
   geometry stays in historical V1; an explicitly accepted later revision replaces it.
5. Extend reusable texture/material extraction only against real requested assets;
   don't claim the existing geometry reader exports complete Unreal materials.

## Tool-use report for GLB-first migration

- V1 Git exception checkpoint: user-authorized source copied to models without
  byte changes; registry verification and all10 tests passed, diff-check clean.
  Exact file/provenance exception documented. Gameplay owner confirmed no active
  producer after cleanup7be5ce8b; old bridge unchanged. Commit uses git-transaction
  wrapper, excludes mixed HUD/toolchain backlogs. CIM read was denied; Get-Process
  fallback and owner's confirmation supplied the producer check. No game build/install.
- Existing tool routing reviewed, consumer search and source hash/hierarchy audit
  performed. Authored source-only GLB inspection/preview is outside game-tool
  coverage. No extraction/cook/install operation in this migration.
- Added bounded read-only GLB reader/registry verifier (Python+NumPy), 10 synthetic
  regression tests pass. V1 buffer ranges/indices/attributes/scene graph/matrices/
  bindings validated, standard Blender import and direct preview passed.
- Preserved supplied bytes exactly. New GLB utilities are methods, not a procedural
  replacement for the user's model. Relocated standard exporter; retained game
  mesh reader. Full glTF conformance and general shaders/skin/Nanite support are
  explicitly not claimed.
- Active logic task notified of model/process and breaking parent/pivot changes;
  its build/C++/legacy descriptor/palette consumers remain unchanged. Only mod
  README/AGENTS received a model-contract clarification, communicated to the owner.
- Relocated standard Blender exporter re-verified against socket GLB: original
  source hash retained,2932 triangles/3 materials,world error2.1073424e-8m.
- No Git-state mutation. No unrelated worktree edits discarded.
- Owner's dependency audit used read-only rg/Get-Content/process checks, no failures
  or code changes. This task verified no diff on old bridge/build/C++ paths.
- Documentation command validation: initial ad-hoc nested-shell quoting failed
  before parsing; corrected literal command validated both tool README PowerShell
  blocks under Windows PowerShell 5.1. No script or source-file failure involved.
