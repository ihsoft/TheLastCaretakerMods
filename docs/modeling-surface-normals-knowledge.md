# Surface normals (geometric model; rendering exceptions exist)

For a conventional solid mesh, a surface separates material from visible free
space. The outward orientation points from the material into that free space.
This is a useful geometric model, not a repository requirement for every mesh:
thin surfaces, interior views, two-sided rendering, and authored shading can
have different contracts.

## Geometric orientation

- An exterior solid surface normally faces away from the solid.
- A tube's exterior faces away from its centerline; the wall of an open hole
  faces into the hole's empty space.
- End caps face out of the solid volume.
- A physically thick sheet has two distinct sides with opposite outward
  orientations. A zero-thickness two-sided mesh can be a deliberate rendering
  representation of that sheet.

These statements describe the intended physical surface. The actual renderer
also depends on face winding, stored vertex or custom normals, material sidedness,
and transforms.

## Face winding and stored normals

Face winding determines a geometric face normal. Stored vertex normals may be
interpolated or authored separately for shading. Reversing triangle winding
does not necessarily update the stored `NORMAL` values. Conversely, a dark face
does not by itself prove that its winding is reversed: material, lighting,
normal maps, and custom normals can contribute.

For ordinary outward-facing shading, face and stored normals usually point into
the same broad hemisphere. Smooth shading may tilt vertex normals away from
the geometric face normal. Compare winding and stored normals independently
before deciding which data needs repair.

## Useful checks

For the exterior of a cylindrical section, an outward radial orientation can
be checked with:

```text
dot(vertex - centerline, vertexNormal) > 0
dot(faceCenter - centerline, faceNormal) > 0
```

These checks apply to the cylindrical exterior, not its caps, cavities,
deliberately inward-facing faces, or arbitrary custom shading. A known outward
direction for a planar face supplies an analogous dot-product check.

When a surface looks unexpectedly dark, comparing it with a correctly rendered
surface using the same material can help distinguish geometry and normal data
from material settings. Inspect winding, vertex normals, and relevant transforms
separately. For a GLB, a local primitive/accessor repair may preserve unrelated
authored normals better than a full re-export; choose the repair after inspecting
the affected asset.
