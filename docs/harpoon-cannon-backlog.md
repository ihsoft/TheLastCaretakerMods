# HarpoonCannon: current state and next work

## Accepted baseline

- Logic/integration commit: `089d1e42397c85f89be1306985cb0b9bdca449d8`.
- Model/palette commit: `0dd54ca113e5ec319938dbd250ba1d64ee55f51b`.
- User accepted clean HUD, center circle, Toggle scope, first-person camera and
  v5 model/materials. Single-player operator/optics are game-validated.
- Not implemented/validated: firing, cable attachment/tension, ammunition state,
  save-safe station persistence, multiplayer and long-session lifecycle.
- Native E exit, F8 fallback and 20-second safety exit remain.
- GLB V1 integration `glb-v1-rt-04` game-validated 2026-09-16: placement,
  construction, entry, yaw/elevation, RMB 1x/5x and E exit.

## Installed pair and recovery

All paths below are repository-relative. Retain these immutable artifacts.
Older manifest sourceCommit fields remain original build provenance.

Operator:
`artifacts/harpoon-cannon/clean-hud-02/release-manifest.json`

Current shell (construction and operator/optics game-validated 2026-09-16):
`artifacts/harpoon-cannon/glb-v1-rt-04/release-manifest.json`

Previous accepted shell, retained for recovery:
`artifacts/harpoon-cannon/visual-v5-palette-02/release-manifest.json`

Recovery only when requested, after checking the game is closed. Undo the current
GLB layer through its receipt before using older baseline teardown instructions:
`artifacts/installations/HarpoonCannonShellProbe/20260915-083631-glb-v1-rt-04-f532945e/install-manifest.json`.
For older baseline teardown, restore OPERATOR FIRST through:
`artifacts/installations/HarpoonCannonLifecycleProbe/20260914-051228-clean-hud-02-ac501b80/install-manifest.json`

Then SHELL:
`artifacts/installations/HarpoonCannonShellProbe/20260914-050913-visual-v5-palette-02-1f91521e/install-manifest.json`

Use public installer/restorer, exact manifests and installed hash checks.
Do not replace packages while playing. Repository-only research may continue.
Retain local `%LOCALAPPDATA%/Voyage/Saved/Config/HarpoonCannon.ini` preferences.

## Current checkpoint: validated GLB integration

User confirmed on 2026-09-16 that `glb-v1-rt-04` supports placement,
construction, entry, yaw/elevation, RMB 1x/5x and E exit. Do not repeat these tests.
Unfinished-construction Q cancellation was not separately reported for V1.
Save persistence, multiplayer, ammunition and firing remain outside validation.

Current source: `models/HarpoonCannon/model-source.json`, GLB SHA256
`9FD66B3350239F4CE0E37E0B8BF4F8E9F033A3E22E8EE88CD2DBF9158578A5BD`.
Native Interchange imports 72 meshes and13 material instances. The adapter keeps
93 component nodes, actual hierarchy and registry-bound yaw/pitch/sight roles;
ammo stays separately addressable under its actual pitch ancestor. The86-package
inventory drives cook and validation. No manual OBJ/palette/offset bridge.

Independent verification matched rigid local transforms (max error1.42e-14) and
all13 cooked material factors against
`artifacts/harpoon-cannon/glb-v1-02/source-audit.json`.
Reviewed stock material parent:
`/InterchangeAssets/gltf/MaterialInstances/MI_Default_Opaque`.
Source-only model revisions should reuse this pipeline. Stop and discuss if
conversion requires recurring per-revision code changes.

Recovery: no automatic rollback requested by user. Retain the exact manifests
above; do not delete accepted payloads. A prior crash at executable RVA0x2f3851b
was resolved by ray-tracing resource cooking, without geometry/control changes.
Bounded evidence: `artifacts/harpoon-cannon/glb-crash-top.json`,
`glb-crash-caller.json`, and the glb-v1-03/glb-v1-rt-04 cook logs.
The small dump lacks some heap pages; JSON inspector omits ray-tracing proxy
data. Preserve this validation limitation, not a false static-resource assertion.
Durable import/client-mode and ray-tracing lessons are in PIPELINE_OBSERVATIONS.

Cleanup authorized: removed unused OBJ/parser/legacy operator generation and
ModelRecipe.h; source-model files owned by the model task remain untouched.
Candidate `glb-cleanup-06` passed build/generate/cook/container/semantic checks
without installation. Its model inventory exactly matches the game-validated
payload; independent93-component/13-material verification passed again.
The first cleanup attempt stopped before Unreal because hashing included a
Git-tracked deleted file. Source hashes now cover existing files; Git source
status retains deletions. This was a blocking correctness fix, not a new tool.
Next: resume firing research below. Installed user-validated shell stays unchanged.
Shared tools/README.md and tools/glb/README.md contain mixed concurrent changes
and are intentionally excluded from the scoped feature commit; their Harpoon
documentation additions remain unstaged alongside the other owner's additions.
Tool report: public Build-Shell initially rejected a deleted source at hashing;
after the scoped fix its full pipeline passed. verify_harpoon_import and exact
inventory comparison passed. No runtime test repeated and no install performed.

## Next work: firing

User authorized transition to shot logic. No firing implementation or new test
package exists yet. Keep the accepted installed pair unchanged during research.

Current-build stock `BP_Module_Turret` has `OutputLocation`,
`CreateAbilityComponent`, `StartFiring` and `ActivateAbility`.
Its shared ubergraph references:
- `VoyageCombatBlueprintFunctionLibrary.ActivateAbility`;
- `VoyageCombatBlueprintFunctionLibrary.ActivateBallisticAbility`;
- `WeaponAbilityComponent.GetWeaponData`.

Next inspect the native signatures, ability setup and hit consumer. Event
wrappers jump into a shared graph; a call summary is not the full firing contract.
Evidence: `artifacts/asset-summaries/game/05E7DB681D3AA8B6ADF4A73FF8F3598B19E1BAE59490851B2293271084A5A8A9/summary.json`.

Proposed first discriminator: one visible shot from a model-defined muzzle,
trajectory and hit detection, with explicit own-actor collision policy.
Separate cable/tension, damage balance and ammunition persistence from that test.
Do not invent those contracts from the successful operator prototype.

## Version gate and owning references

Last fingerprint check: Steam25191271, executable SHA256
`747DC2553F7E68D8EA7ED0B2E0CAC6D08943EA3F50DD6ED822E9293E0B45F58B`.
Game UE5.8 / editor5.8.2. Recheck before reusing game-derived inputs.
Reviewed mapping: `mappings/Voyage/steam-25191271-ue5.8/Voyage-25191271.usmap`.

- [Architecture and findings](../mods/HarpoonCannon/RESEARCH.md)
- [Native entry contracts](../mods/HarpoonCannon/VEHICLE_ENTRY_RESEARCH.md)
- [Provenance](../mods/HarpoonCannon/GAME_DERIVED_SOURCES.md)
- [Tool pitfalls and recovery](../mods/HarpoonCannon/PIPELINE_OBSERVATIONS.md)
- [Rules](../mods/HarpoonCannon/AGENTS.md)

Model work is separately owned. Historical chronology is recoverable from Git;
old logs/candidates remain ignored evidence, not routine reading instructions.
