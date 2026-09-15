# HarpoonCannon — GLB-first model

**Authoritative model: HarpoonCannon_V1.glb.** The user selected this uploaded
reference as the first revision of the new GLB workflow. It is not the previous
procedurally generated v5, and it has not yet been game-validated.

[model-source.json](model-source.json) is the version/hash and named-role registry.
Versioned source: [HarpoonCannon_V1.glb](HarpoonCannon_V1.glb).
SHA256: `9FD66B3350239F4CE0E37E0B8BF4F8E9F033A3E22E8EE88CD2DBF9158578A5BD`.
The supplied bytes are unchanged; the original upload filename is not our version.

## Workflow and ownership

1. Edit GLB as the source of truth. Blender or another editor is allowed, but a
   .blend, OBJ, FBX or cooked mesh is not a parallel editable master.
2. Name accepted revisions HarpoonCannon_V1.glb, V2, etc. Use temporary candidate
   names while editing; do not silently overwrite accepted bytes. Model version
   is independent of mod/release/game versions.
3. Update the registry path/hash/roles together with an accepted new GLB. Verify
   the actual scene graph, geometry and materials; show direct model renders.
4. Hand the GLB and registry to final assembly. Convert only as a derived build
   step to whatever the engine importer needs, pinned to that exact GLB hash.
   Do not edit generated OBJ/MTL/palette files or regenerate GLB from old primitives.
5. Preserve logic-facing names and consult the gameplay owner about changed
   hierarchy, pivots, sight, ammo parenting or socket integration. Collision,
   input, camera behavior, resource logic and installed packages remain theirs.

## V1 facts and bindings

GLB 2.0, meters, Y-up, cannon forward +X. All 94 nodes are active; 77 mesh nodes
reference 72 mesh definitions, 11682 instantiated triangles and 13 materials.
No textures/images, skins or animations. Bounds about 2.993 x 1.718 x 1.715 m in
glTF X/Y/Z. File has been structurally audited, imported and directly rendered
in Blender; these checks are not game runtime acceptance.

| Role | Node / actual hierarchy |
| --- | --- |
| Model root | world -> HarpoonCannon |
| Stationary base | HarpoonCannon -> HC_Base |
| Traverse | HC_Base -> HC_Yaw |
| Elevation | HC_Yaw -> HC_Pitch |
| Sight | HC_Pitch -> HC_RG_PITCH_OpticHousing -> HC_Sight |
| Ammo basket | HC_Pitch -> HC_RG_YAW_AmmoBasket |
| Six rods | basket -> HC_RG_YAW_AmmoBasket_Ammo -> HC_RG_AMMO_Rod_01..06 |
| Power anchor | HC_Base -> HC_PowerSocket |
| Temporary game reference | anchor -> ElectricalSocket_Reference -> SM_Socket_Electrical_Output |

Names identify nodes; their prefixes do NOT determine motion ownership. In V1
the basket/ammo follows pitch, despite YAW in the basket name. Preserve nested
transforms; a parent may itself have mesh geometry (optic housing, rear latch).
Some existing names now mark empty nodes, such as CoilStatus and AlignmentActuator.
Do not interpret them as missing meshes and regenerate old geometry.

Read complete local matrices from GLB, multiply ancestors once, and convert axes
and units once at the engine boundary. Useful checks, glTF meters:

- HC_Pitch relative to yaw is approximately (0,1.07,0), not old (-.18,1.07,0).
- HC_Sight world position is approximately (-.70,1.63,0), under the optic housing.
- HC_PowerSocket relative to base is (0,.25,.77).
- Embedded PBR factors are already glTF material values; do NOT apply the retired
  MTL-to-linear conversion again. V1 has 13 materials, not the legacy nine.
- The source contains no animation; runtime articulation/independent ammo control
  must be supplied by the gameplay adapter and validated in game.

## Socket reference and explicit V1 Git exception

The user explicitly authorized committing this first GLB with its embedded socket
as a material baseline. Root rules now allow this exact V1 path/hash, with provenance
in model-source.json. The binary belongs in Git and is available from a clean clone;
it is not regenerated from the retired scripts. All other extracted outputs remain
ignored. This narrow exception is not blanket permission for later mixed revisions.

The intended future implementation replaces the reference subtree with the original
game socket at HC_PowerSocket. Keep the anchor; never silently delete the reference
or describe it as newly authored geometry. When ready for native socket integration,
make an explicitly accepted new model revision with the anchor retained. Keep V1
as its historical baseline; do not strip the socket from it retrospectively.

## Tools

[GLB toolkit](../../tools/glb/README.md) owns inspection, direct previews and standard
Blender-to-GLB conversion. Original game mesh extraction is kept in
[VoyageMeshReference](../../tools/VoyageMeshReference/README.md). Preserve these tools
when retiring construction scripts. Texture/material extraction remains a desired
reusable capability, but is not implemented by the geometry-only reader.

```powershell
python tools/glb/inspect_glb.py --registry models/HarpoonCannon/model-source.json
```

Current local evidence: artifacts/modeling/HarpoonCannon/versions/V1/audit.json
and preview.png. No game assets or installed containers are changed by these tools.

## Frozen legacy build bridge — NOT the current model

Current gameplay assembly still consumes runtime-model.json (schema 1), the v5
OBJ/MTL, material-palette.json and Unreal palette headers at their old paths.
They are temporarily retained unchanged until the gameplay adapter is migrated.
They do NOT describe V1, are NOT derived from it, and cannot build this revision.

The old construction chain is retired for authoring: geometry.py, build_model.py,
refine_geometry.py, render_model.py, prepare_handoff.py, prepare_materials.py and
export_blender.py. Their v4/v5 OBJ/MTL, baseline.json and palette are frozen legacy
inputs/methods, not editable baselines. Some are transitive dependencies of the
old bridge, so removal/moving must wait for consumer migration; do not break an
active producer merely to tidy the directory.

The reusable former export_glb.py has moved to tools/glb/export_blender_glb.py.
Do not implement GLB migration by overwriting the old descriptor while old readers
still assume yaw-parented ammo, fixed origins and the nine-slot MTL palette.

Current work and pending integration: [model backlog](../../docs/harpoon-cannon-model-backlog.md).
