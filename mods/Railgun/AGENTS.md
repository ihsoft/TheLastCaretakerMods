# Railgun rules

Root `../../AGENTS.md` applies. This file owns only Railgun-specific contracts.

- The supported product root is `mods/Railgun`; do not recreate parallel probe,
  shell, station or model projects.
- `Build-Railgun.ps1` is the only public producer. Requested builds use
  `-Install` by default unless the user explicitly asks for preparation only.
- Never stop the game. Installation must retain the common closed-game,
  manifest, backup and installed-hash gates.
- All mod-authored build inputs, including the GLB, sound and settings schema,
  live below this directory. A Railgun build must not depend on preserved
  scratch output from an earlier Railgun run. Published toolchain binaries and
  reviewed reusable caches selected through their documented interfaces are
  separate dependencies. Apply the repository lifecycle policy: keep retained
  releases, minimal trust evidence and rollback material in `artifacts/`; treat
  logs, loose cook output, temporary scripts and one-off readbacks as temporary.
  The public producer writes those disposable files below repository `Tmp/`,
  removes its owned successful-run scratch, and publishes only the validated
  self-contained release below `artifacts/railgun/build-*`.
- `Assets/Railgun.ini` is the canonical packaged INI and sole source of setting
  defaults, comments, order and formatting. `Settings/Railgun.settings.json`
  owns only runtime bindings, types and numeric ranges. Add each option to both,
  then wire only its consuming behavior by hand; never edit generated output.
  `Build-Railgun.ps1` owns the internal validation and generation step. The
  only runtime safety fallback outside that file is disabled recoil: when the
  INI or either recoil key is missing or invalid, camera and ship recoil use
  zero. Explicit valid INI values still override that fallback.
- The user owns model geometry. Ada may move or wire model files and update the
  role manifest, but does not alter GLB geometry unless explicitly requested.
- Replacement gun and cassette GLBs are gated by their behavior-critical roles,
  stable owned identities, usable render geometry and collision, and the
  dependencies actually produced for the current source. Do not freeze authored
  models by revision hash, topology, decorative-node names, previous bounds, or
  material and texture counts. Build provenance still hashes the actual inputs
  and rejects source edits made during one build.
- The live model has the stable path `Assets/Model/Railgun.glb`. Model logic
  binds stable roles and explicit boxes from `Assets/Model/model-source.json`;
  do not hard-code vertex topology, offsets or incidental Blender object order.
  Validate behavior-critical structure, not decorative detail names or counts.
  Inventory visuals require six distinct mapped groups with nonempty render
  subtrees, all included in hide/show control while holders stay unaffected;
  the internal mesh composition may change without changing this contract.
- The generated station uses the common Voyage vehicle entry/exit path, a
  stationary nonphysical root, owned camera/input/HUD, tagged properties and an
  explicit interaction provider. Preserve those contracts when changing logic.
- The gun actor, UI and demand-loaded graphics use product-owned
  `/Game/Mods/Railgun/...` identities. Primary assets must additionally live
  below the current game's confirmed AssetManager scan roots: the gun item
  below `/Game/Data/Assets` and the research skill below
  `/Game/Data/Assets/Skill`. Keep their names mod-unique and never ship a stock
  Cyclone package override.
- The released container is exactly `Railgun` plus its autoload sidecar and
  optional user settings. Its PAK contains only the three-record primary-asset
  registry at `Voyage/Mods/RailgunCatalogue/AssetRegistry.bin`; the release ZIP
  also owns the matching loose content-only descriptor. Never ship or replace
  `Voyage/AssetRegistry.bin`. Build registry records from the owned data-asset
  JSON through the shared Voyage registry profile/tool; no stock registry,
  stock primary record, or Railgun-owned registry writer/policy is a build
  input. Editor mirrors and generator binaries are never shipped.
- Keep documentation factual and compact. Do not maintain an experiment
  chronology or candidate backlog. Keep transient logs and discarded
  experiment output in repository `Tmp/`; promote durable conclusions to
  documentation and retain research artifacts only when they are needed for
  continuing work.
- Runtime/UI/gameplay changes are complete only after a real-game test. Build,
  cook and static verification remain lower gates.
