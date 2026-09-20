## Surface Normals

> **Rule of thumb: normals point toward where an observer could physically exist.**

- Mechanical geometry represents solid physical matter unless explicitly stated otherwise.
- Every surface is a boundary between material and observable free space.
- Normals MUST point from the material into that free space.

### Consequences
- Solid exterior: normals point outward.
- Tube/cylinder exterior: normals point away from the centerline/axis.
- Hole/cavity wall: normals point into the cavity, because that cavity is free space.
- Closed end caps: normals point outward from the solid volume.
- If a surface can be observed from both sides, model physical thickness and two separate surfaces with opposite outward normals.
- Zero-thickness two-sided surfaces are allowed only as an explicit game/rendering optimization.

### Face Winding and Vertex Normals
- Correct face orientation alone is NOT sufficient.
- Face winding and vertex/custom normals MUST agree and point to the same physical side.
- After flipping face winding, never assume stored `NORMAL` data became correct automatically.
- Do not fix shading by changing the material when the real problem is orientation or normals.
- Procedural generators must construct correct winding and normals from the physical model, not generate first and guess whether to flip afterward.

### Shading Consistency
If geometry has correct outward face orientation but still renders unusually dark:

1. Compare its material with a known-good object using the same material.
2. Verify face winding independently.
3. Verify vertex/custom normals independently.
4. Rebuild or flip the `NORMAL` data if normals still point into the material.
5. Do NOT modify material properties to compensate for broken normals.

Unexpectedly dark shading on an object that shares a material with correctly rendered objects should be treated as a normals/shading problem first.

### Procedural Validation
Where possible, mathematically verify orientation before delivery.

For a tube:

- vertex/custom normal:
  `dot(vertex - centerline, normal) > 0`

- face normal:
  `dot(faceCenter - centerline, faceNormal) > 0`

Both checks must pass.

### Physical Thickness
A surface visible from both sides represents two physical boundaries, not one zero-thickness wall.

In a real object there is material between those sides and therefore some wall thickness. Treat a zero-thickness surface as an optimization of the game representation, not as the default physical model.

Use zero-thickness geometry only when:
- the opposite side is intentionally unobservable, or
- it is explicitly requested as a rendering/game optimization.

### Delivery Validation
Before delivering any new, rebuilt, or procedurally generated mesh:

1. Verify face orientation.
2. Verify vertex/custom normals agree with face orientation.
3. Verify normals point into observable free space.
4. Verify smooth/hard shading behavior is preserved.
5. Compare suspicious shading against a known-good object using the same material.
6. Run an automatic outward-normal sanity check whenever one can be defined.
7. Do not full re-export a GLB merely to fix one mesh if that risks destroying unrelated custom normals.
8. Prefer patching only the affected primitive/accessors when possible.