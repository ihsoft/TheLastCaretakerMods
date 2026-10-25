# Railgun rules

Root `../../AGENTS.md` applies. This file owns only Railgun-specific contracts.

- The supported product root is `mods/Railgun`; do not recreate parallel probe,
  shell, station or model projects.
- `Build-Railgun.ps1` is the only public producer. Requested builds use
  `-Install` by default unless the user explicitly asks for preparation only.
- `-OutputRoot` selects a fresh directory below `Tmp/Railgun/`;
  `-ArtifactVersion` gives the temporary run directory a distinct safe identity;
  it never changes the canonical `Railgun-V<N>.zip` name. On a changed
  Steam build, retry with `-IgnorePinnedGameVersion` and test the resulting
  candidate in game before updating `game-build.json`.
- Never stop the game. Installation must retain the common closed-game,
  manifest, backup, readback, and rollback gates.
- All mod-authored build inputs, including the GLB, sound and settings schema,
  live below this directory. A Railgun build must not depend on preserved
  scratch output from an earlier Railgun run. Published toolchain binaries and
  reviewed reusable caches selected through their documented interfaces are
  separate dependencies. Apply the repository lifecycle policy: the public
  producer writes its candidate, loose payload, reports and logs below
  repository `Tmp/Railgun/`. Keep the exact candidate there through its game
  test and Nexus publication. Only after publication is confirmed and
  `version.txt` advances, use the shared installer's
  `-RetainPublishedRelease` mode to preserve the exact ZIP and original release
  manifest under `artifacts/installations/Railgun-V<N>` without rebuilding or
  reinstalling.
  A temporary installation keeps its recovery data in
  `Tmp/Railgun/installations/`. No ZIP goes into Voyage's Paks directory.
- `Assets/Railgun.ini` owns distributed values, comments, order and formatting.
  `Settings/Railgun.settings.json` owns runtime bindings, types, required schema
  fallbacks, numeric storage, and optional inclusive ranges. Add each option to both,
  then wire only its consuming behavior by hand; never edit generated output.
  Fixed presentation values that are intentionally not user options belong as
  named constants with their audio or HUD owner and in neither settings file.
  `Build-Railgun.ps1` owns the internal validation and generation step. The
  recoil schema fallbacks are 0.5 for camera recoil and 2 for ship recoil,
  matching the current distributed values. Missing or invalid values use these
  fallbacks; an explicit zero disables the corresponding effect.
- The user owns model geometry. The agent may move or wire model files and
  update the role manifest, but must not alter GLB geometry unless explicitly
  requested.
- Replacement gun geometry and the model-selected composite cassette are gated
  by behavior-critical roles, stable owned identities, usable render geometry
  and collision, and the dependencies actually produced from the main GLB. Do
  not freeze authored models by revision hash, topology, decorative-node names,
  previous bounds, or material and texture counts. The producer validates the
  actual inputs and retains only the bounded Core cook-list digest required by
  the commandlet's time-of-check/time-of-use contract.
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
  `/Game/Data/Assets/Skill`. Keep their names mod-unique and do not override
  stock primary-asset packages.
- The released container is exactly the `Railgun` triplet plus optional user
  settings. The gun shell owns event-driven, idempotent station initialization.
  Its PAK contains only the three-record primary-asset
  registry at `Voyage/Mods/RailgunCatalogue/AssetRegistry.bin`; the release ZIP
  also owns the matching loose content-only descriptor. Never ship or replace
  `Voyage/AssetRegistry.bin`. Build registry records from the owned data-asset
  JSON through the shared Voyage registry profile/tool; no stock registry,
  stock primary record, or Railgun-owned registry writer/policy is a build
  input. Editor mirrors and generator binaries are never shipped.
- Keep documentation factual and compact. Do not maintain an experiment
  chronology, candidate backlog, or records that the user tested a build.
  Describe current behavior, durable findings and compatibility limits;
  selected release package identities belong to release artifacts.
  Keep transient logs and discarded
  experiment output in repository `Tmp/`; promote durable conclusions to
  documentation and retain research artifacts only when they are needed for
  continuing work.
- Runtime/UI/gameplay changes are complete only after a real-game test. Build,
  cook and static verification remain lower gates.
