# Model authoring contracts

- GLB is the authoritative editable source and final model handoff. Keep a named
  version and SHA-256 in the owning model registry; never infer model version from
  an upload's filename or the gameplay mod version. A changed accepted file gets
  a new V-number; conversions and renders are derived outputs, not alternate masters.
- Use actual GLB node hierarchy, transforms, units and embedded PBR materials.
  Preserve logic-facing node names and coordinate contracts. A name prefix does
  not override parenting; changes affecting gameplay consumers require coordination.
- Do not regenerate current models with retired procedural/OBJ generators. Frozen
  old build bridges may remain temporarily for existing consumers, explicitly
  marked legacy; they must never masquerade as a conversion of the current GLB.
- Reusable GLB inspection/conversion and game asset extraction methods belong in
  tools/. Keep them when retiring model-construction scripts. Extraction of meshes,
  textures and materials is useful future work, not permission to invent untested
  capabilities. Root provenance, ignored game-derived data and validation rules apply.
