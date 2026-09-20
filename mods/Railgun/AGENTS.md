# Railgun rules

Root `../../AGENTS.md` applies. This file owns only Railgun-specific contracts.

- The supported product root is `mods/Railgun`; do not recreate parallel probe,
  shell, station or model projects.
- `Build-Railgun.ps1` is the only public producer. Requested builds use
  `-Install` by default unless the user explicitly asks for preparation only.
- Never stop the game. Installation must retain the common closed-game,
  manifest, backup and installed-hash gates.
- All mod-authored build inputs, including the GLB, sound and settings template,
  live below this directory. `artifacts` contains outputs and evidence only and
  must never be required to build.
- The user owns model geometry. Ada may move or wire model files and update the
  role manifest, but does not alter GLB geometry unless explicitly requested.
- The live model has the stable path `Assets/Model/Railgun.glb`. Model logic
  binds stable roles and explicit boxes from `Assets/Model/model-source.json`;
  do not hard-code vertex topology, offsets or incidental Blender object order.
- The generated station uses the common Voyage vehicle entry/exit path, a
  stationary nonphysical root, owned camera/input/HUD, tagged properties and an
  explicit interaction provider. Preserve those contracts when changing logic.
- Construction intentionally overrides the stock Cyclone leaf package required
  by the game's fabricator. Product-owned assets use `/Game/Mods/Railgun/...`.
- The released container is exactly `Railgun_P` plus its autoload sidecar and
  optional user settings. Editor mirrors and generator binaries are never
  shipped.
- Keep documentation factual and compact. Do not maintain an experiment
  chronology or candidate backlog. Put transient logs under ignored artifacts;
  promote only game-validated facts or clearly labelled design hypotheses.
- Runtime/UI/gameplay changes are complete only after a real-game test. Build,
  cook and static verification remain lower gates.
