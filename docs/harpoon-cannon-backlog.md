# HarpoonCannon: current state and next work

## Accepted baseline

- Logic/integration commit: `089d1e42397c85f89be1306985cb0b9bdca449d8`.
- Model/palette commit: `0dd54ca113e5ec319938dbd250ba1d64ee55f51b`.
- User accepted clean HUD, center circle, Toggle scope, first-person camera and
  v5 model/materials. Single-player operator/optics are game-validated.
- Not implemented/validated: firing, cable attachment/tension, ammunition state,
  save-safe station persistence, multiplayer and long-session lifecycle.
- Native E exit, F8 fallback and 20-second safety exit remain.

## Installed pair and recovery

All paths below are repository-relative. Retain these immutable artifacts.
Older manifest sourceCommit fields remain original build provenance.

Operator:
`artifacts/harpoon-cannon/clean-hud-02/release-manifest.json`

Accepted shell:
`artifacts/harpoon-cannon/visual-v5-palette-02/release-manifest.json`

After checking the game is closed, restore OPERATOR FIRST through:
`artifacts/installations/HarpoonCannonLifecycleProbe/20260914-051228-clean-hud-02-ac501b80/install-manifest.json`

Then SHELL:
`artifacts/installations/HarpoonCannonShellProbe/20260914-050913-visual-v5-palette-02-1f91521e/install-manifest.json`

Use public installer/restorer, exact manifests and installed hash checks.
Do not replace packages while playing. Repository-only research may continue.
Retain local `%LOCALAPPDATA%/Voyage/Saved/Config/HarpoonCannon.ini` preferences.

## Next: firing

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
