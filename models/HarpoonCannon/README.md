# HarpoonCannon model source

The user accepted **railgun C v4** on 2026-09-13. OBJ/MTL are the exact accepted
authored mesh and placeholder palette; `baseline.json` pins their SHA-256.
They contain no extracted game mesh, texture, or reconstructed Blueprint.
This directory is independent of the gameplay mod's older `SourceAssets`.

## Contents

- OBJ/MTL and `baseline.json`: accepted source checkpoint, centimeters, +X firing,
  +Y right, +Z up. Never silently overwrite this checkpoint for a new revision.
- `geometry.py`: mesh primitives and placeholder palette.
- `build_model.py`: complete generator, geometric audit, triangle/normal export,
  OBJ readback, and exact baseline hash verification.
- `render_model.py`: direct OBJ inspection with perspective-correct per-pixel
  depth, backface culling, and five reversed-draw-order checks.

The old experimental generator chain was mechanically consolidated here.
Reproduction is verified against the original accepted OBJ/MTL bytes.
No script imports or needs the deleted scripts from `artifacts/`.

## Run from the repository root

Requires Python 3.10+, NumPy and Pillow. Rendering uses Windows Arial.
Use an existing Python environment containing those packages; no game, Unreal,
mapping, network access, or package installation is part of these commands.

```powershell
python models/HarpoonCannon/build_model.py
python models/HarpoonCannon/render_model.py
```

Both accept `--output <directory>`; pass the same directory to both. The default
is ignored `artifacts/modeling/HarpoonCannon`. A clean checkout can recreate it.
Output may not target this accepted source directory. Build checks mesh closure,
consistent winding, face planarity, nondegenerate triangles, magnet alignment,
socket attachment, export readback and exact accepted hashes. It fails on drift.
Do not run Python with `-O`, which disables geometry assertions.

Rendering produces `_hero.png`, `_inspection.png`, `_details.png` and a compact
`_render_audit.json`; building produces OBJ/MTL and `_audit.json`. Renderer failures
or changed source hashes must be investigated before changing the baseline.
Do not replace direct mesh renders with AI-generated images for geometry review.

## Model contract and pending work

- Fixed centered electricity inlet, yaw base, pitch assembly; standard game
  electricity mesh is still a placeholder and requires current-build validation.
- Five aligned top/left/right magnet stations, six independently named pointed
  rods; 9 cm diameter, 86 cm length. Visibility behavior remains logic-owned.
- 10 cm rail clearance; chamfered muzzle with 10 x 10 cm passage.
- Final UVs, materials, smoothing, pivot-motion clearances, Unreal import,
  construction preview and real-game appearance are not yet validated.
- Preserve gameplay source, collisions, camera/HUD/input, package and installation.

Current decisions and restart state: [model backlog](../../docs/harpoon-cannon-model-backlog.md).
