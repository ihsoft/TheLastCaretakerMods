# HarpoonCannon rules

Root ../../AGENTS.md applies. This file contains only mod-owned constraints.

## Scope and restart

- Ada owns logic and this mod's sources/docs. Model geometry and its backlog have
  another owner; do not contact that agent without actual model work.
- Start with ../../docs/harpoon-cannon-backlog.md, then RESEARCH.md and the narrow
  relevant contract in VEHICLE_ENTRY_RESEARCH.md / GAME_DERIVED_SOURCES.md.
  Historical archives are evidence, never current build/install instructions.
- Do not modify DonkLift, other mods or their installed files. Coordinate shared
  architecture edits if another owner is actively changing that file.
- User permits scoped Harpoon installs without asking again when game is closed.
  Never stop the game. Keep all manifest/fingerprint/backup/readback gates.
- Test/play cycle: after a result, restore probe FIRST and shell SECOND through
  exact installation manifests once game is closed. Prepare the next change
  repository-only; announce readiness and wait for the agreed test window.
  State what was established and the next action, not just permission to play.

## Authoring and preserved behavior

- Current supported paths are Build-Shell.ps1 and AutoloadProbe/Build-Probe.ps1.
  AutoloadProbe contains the active HC33 generator despite its historical name.
  Root generator supports only ShellOnly; legacy operator emission stays blocked.
- Native Voyage mirrors and generator DLLs are editor-only; never ship them.
  Renew all consumed identities on fingerprint change using public tools.
- Station packages must be TAGGED. Reject PKG_UnversionedProperties and unintended
  native CDO/subobject/property deltas through independent cooked inspection.
  Partial action structs are named bytecode fields, not serialized native defaults.
  Never invent ABI padding or infer native interface inheritance from a donor.
- Preserve exact declaring function owners AND required implemented interfaces.
  Explicit Blueprint interface implementation uses the exact interface graph;
  it does not require an inherited SuperStruct. Gate the signature and membership.
- Construction remains VoyageModuleActor at the Cyclone LEAF with exact item/DSO
  identities. Do not override small-turbine/Whisper parent. Preserve base-mesh simple
  collision for unfinished Q cancellation; do not alter it to fix preview rendering.
- Use the common VoyageVehiclePawn native entry/exit, stationary nonphysical root
  and independent camera/HUD/inputs. Do not import Drone/Forklift movement/physics
  or mask ownership defects through per-Tick transforms/global HUD suppression.
- Keep explicit cooked Interact=Block and separate interaction/query ownership.
  Editor in-memory collision masks do not prove serialized runtime masks.
- Keep own native HUD selection, matching context/action hints and useful character
  stats. First-person angular x5. Aim limits and sensitivity are logic-owned;
  requested tuning and pending camera/range changes live in the active backlog.
- Authoritative model authoring/handoff is GLB, versioned by
  models/HarpoonCannon/model-source.json. Read actual node hierarchy/matrices;
  prefixes do not determine parents (V1 ammo follows pitch). Preserve role/sight/
  ammo/anchor names. Current runtime-model.json/OBJ/palette is a frozen pre-GLB
  build bridge, NOT a V1 conversion; retain until a coordinated consumer migration.
  Do not feed new GLB assumptions into old readers or infer collision/input from it.
- Any-hit display name is not target eligibility or persistent identity. Prefer
  nonempty game item Name; otherwise technical actor name. Clear on miss and
  write fallback/range before optional game-data gates.

## Validation and cleanup

- Every test must justify its information value and separate observable outcomes.
  One/two no-ops or a weak discriminator means research, not blind field changes.
  Place markers before guards and audit reachability, not just bytecode presence.
- Preserve last accepted exact payload and source before risky changes. Compile,
  cook/retoc and clean load do not replace gameplay validation.
- HC33 is a tested operator/optics checkpoint, not a firing/persistence release.
  Safety timeout/diagnostics remain in its immutable artifact until a separately
  verified change. Do not claim full lifecycle or multiplayer support.
- Housekeeping preserves active source, accepted payloads, provenance/recovery
  manifests and methods. Archive/hash-check old evidence before deleting it;
  generated intermediate files may be rebuilt. Never clean the shared game store.
- Report tool results/friction at the natural checkpoint; reusable observations
  belong in PIPELINE_OBSERVATIONS.md and the shared toolchain backlog as a batch,
  not a new wrapper/rule for each failed command.
- Do not create one tracked report per experiment. Give the compact tool-use
  report in the handoff, promote durable findings into existing owned documents,
  and put transient logs/receipts in ignored artifacts. Keep only current state
  and pending gates in the backlog; discard report prose after consolidation.
