# Four-Plane Cut Nose Modeling Rule

## Purpose

Use this rule when creating a pointed projectile nose for the railgun model.

The required shape is **not** a pyramid and **not** a smooth-shaded mathematical cone.

The approved construction method is called:

**Four-Plane Cut Nose**

Russian shorthand:

**носик четырьмя секущими плоскостями**

---

## Core Construction Rule

Start from a **cylinder**.

Create one angled cutting plane such that:

- the plane intersects the top face exactly along a diameter;
- the plane enters the cylinder wall at approximately **2/3 of the cylinder height**;
- all material on the outside of the cutting plane is removed.

Repeat the **same cut four times**, rotating the cutting plane by:

- 0°
- 90°
- 180°
- 270°

The result is a cylindrical body with a four-sided sharpened nose that is derived from the cylinder itself.

Do **not** construct the nose by attaching a pyramid to a cylinder.

---

## Geometric Interpretation

For a cylinder aligned to the local Y axis:

- cylinder radius: `R`
- cylinder top: `Y = H`
- cut depth at the cylinder wall: approximately `Y = 2H/3`

A single cut can be understood as a plane that:

1. passes through the cylinder axis at the top surface;
2. passes through one side of the cylinder at approximately `2H/3`.

The other three cuts are exact 90° rotations of this first cut around the cylinder axis.

The final top surface is the intersection of the four retained half-spaces.

---

## Elongated Version

If a longer projectile nose is required:

**Do not change the construction principle.**

First create a correct Four-Plane Cut Nose, then extend the nose along the cylinder axis.

Use the name:

**Elongated Four-Plane Cut Nose**

The silhouette may become much longer and narrower, but it must still be based on the same four planar cuts.

---

## Why This Method Is Preferred

This shape has several advantages for the game asset:

- preserves a clearly cylindrical projectile body;
- produces an intentional engineered appearance;
- avoids problematic smooth shading around a singular cone apex;
- works well with modest topology;
- remains visually readable at gameplay distance;
- is easy to reproduce procedurally;
- does not require excessive radial subdivision to appear correct.

This is a **game-readable engineering shape**, not an attempt to reproduce a real railgun projectile.

---

## Shading

The four cut faces are planar surfaces.

Therefore:

- each cut face should shade as a plane;
- edges between neighboring cut faces are real geometric edges;
- do not try to smooth the four cut faces into a mathematical cone;
- the untouched cylindrical surface may use normal smooth shading around its circumference.

Where the cut surfaces meet the cylindrical body, use the actual geometry to determine the shading transition.

Do not increase topology merely to hide the four-plane construction.

---

## Surface Normals

All normal rules for the project still apply.

> Normals point toward where an observer could physically exist.

For this geometry:

- cylinder exterior normals point away from the cylinder axis;
- cut-face normals point outward from the retained solid;
- bottom/end-cap normals point outward from the solid volume.

Face winding and vertex/custom normals must be checked independently.

Do not assume that flipping face winding automatically repairs stored vertex normals.

---

## Procedural Validation

Before delivery, verify:

1. The source shape began as a cylinder.
2. There are exactly four equivalent angled cuts.
3. The cuts are rotated by 90° increments.
4. Each cut removes material rather than adding a separate wedge or pyramid.
5. The cylindrical body remains geometrically continuous with the nose.
6. Cut faces have outward face normals.
7. Cylinder exterior normals point away from the axis.
8. No unrelated meshes were modified.
9. Topology has not been increased merely to improve apparent smoothness.

For a cylindrical section, an automated sanity check may use:

`dot(vertex - centerline, normal) > 0`

and for face normals:

`dot(faceCenter - centerline, faceNormal) > 0`

For planar cut faces, test against the known outward normal of the corresponding cutting plane.

---

## Anti-Patterns

Do **not** use any of the following as substitutes:

### Cylinder + Pyramid

Attaching a four-sided pyramid to a cylindrical body is not the same shape.

The four faces must be produced by **cutting the cylinder**.

### Smooth Mathematical Cone

Do not replace the Four-Plane Cut Nose with a rotational cone.

A smooth cone caused repeated shading and apex-normal problems and is not the approved projectile-base shape.

### Excessive Segmentation

Do not keep increasing radial segments to disguise shading artifacts.

Fix geometry or normals instead.

### Custom-Normal Guessing

Do not invent custom apex-normal schemes unless explicitly required.

If the geometry itself can produce the intended visual result, prefer the geometric solution.

---

## Naming Convention

Use the following terminology consistently:

- `Four-Plane Cut Nose` — standard version
- `Elongated Four-Plane Cut Nose` — axially stretched version

Avoid ambiguous names such as:

- cone
- pyramid nose
- four-sided cone
- smooth nose

unless describing a different shape intentionally.

---

## Reference Summary

**Recipe:**

1. Start with cylinder.
2. Make one angled plane.
3. Plane crosses the top face along a diameter.
4. Plane reaches the cylinder wall at about 2/3 height.
5. Remove material beyond the plane.
6. Repeat at 90°, 180°, and 270°.
7. Optionally elongate along the cylinder axis.
8. Validate normals and winding.
9. Do not smooth the four planar cut faces into a cone.

This construction is the canonical projectile-nose method for the railgun model unless explicitly superseded.
