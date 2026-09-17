# HarpoonCannon: current state and next work

## Restart

User accepted native firing and requested removal of test-only code and a scoped
commit. The shark died after approximately four shots at200 configured base
damage with no inter-click cooldown. This validates damage and repeated firing,
not exact effective damage, initial shark health or final weapon balance.
Do not repeat the high-speed collision, shark-kill or dismantle discriminators
without new evidence. Durable contracts are in RESEARCH.md and
VEHICLE_ENTRY_RESEARCH.md.

Installed game-validated station:
`artifacts/harpoon-cannon/direct-damage-200-01/release-manifest.json`.
Receipt and exact predecessor backup:
`artifacts/installations/HarpoonCannonLifecycleProbe/20260917-073610-direct-damage-200-01-33cbecc8/install-manifest.json`.
Installed shell remains:
`artifacts/harpoon-cannon/glb-v1-rt-04/release-manifest.json`.
Its receipt:
`artifacts/installations/HarpoonCannonShellProbe/20260915-083631-glb-v1-rt-04-f532945e/install-manifest.json`.

Cleanup removes the automatic speed-test harness, diagnostic trajectory/contact
meshes, shot-status text and20s forced exit. E exit, emergency F8, normal target
name/range, scope, character stats and native attack safeguards remain.
Stable runtime package names (including BP_HarpoonTestShot) are intentionally
unchanged. No accepted payload, receipt or shared game-store entry was deleted.
Clean package: `artifacts/harpoon-cannon/firing-clean-02/release-manifest.json`.
Prepared, not installed or independently game-validated after cleanup.

## Next feature scope

Preserve the accepted firing core: LMB Started -> one projectile per click ->
native swept movement2000m/s -> original ReceiveHit -> one Directional attack.
Range1000m; ignore cannon/operator/station, not the ship. No inherited ship or
character velocity, gravity, penetration or ricochet. Four-second shot lifetime
keeps the native attack causer alive and is not a cooldown.
Next additions are ammunition and energy eligibility/consumption, then physical
and audio effects. Do not implement them as part of cleanup. Exact resource
costs and balance require a separate decision.
Save persistence, multiplayer, death/travel and long-session lifecycle remain
unvalidated. Do not claim a save-safe release.

## Established model/operator baseline

GLB integration source checkpoint: `402f627b685686df502bef4455d02e4067dcf8ee`.
User validated placement, construction, entry, yaw/elevation, RMB1x/5x and E exit.
Current model: `models/HarpoonCannon/model-source.json`, GLB SHA256
`9FD66B3350239F4CE0E37E0B8BF4F8E9F033A3E22E8EE88CD2DBF9158578A5BD`.
Native Interchange imports72 meshes and13 material instances; adapter retains
93 component nodes and actual hierarchy. Ammo objects stay independently
addressable under pitch; no ammo consumption is implemented yet.
Model ownership remains separate. Changes should reuse the GLB pipeline; stop
and discuss recurring per-revision conversion work instead of normalizing it.
V1 lacks an authored muzzle: hash-gated pitch-local fallback(208.4,0,4)cm, +X,
from `artifacts/modeling/HarpoonCannon/muzzle-v1-audit.json`. A future authored
muzzle replaces this fallback. Sight and projectile origin are distinct.

Dismantle-after-exit was validated with `dismantle-safe-01` and retained in the
current firing source. All seven unavailable-entry branches explicitly return
handled/empty actions, preventing native legacy fallback. Invalid-anchor cleanup
disables new acquisition before station destruction. See VEHICLE_ENTRY_RESEARCH.
The earlier GLB construction crash was resolved by ray-tracing resource cooking;
see PIPELINE_OBSERVATIONS, not old trial instructions.

## Retained evidence and recovery

All paths are repository-relative; generated evidence remains ignored.
- Native speed collision: `artifacts/harpoon-cannon/speed-collision-04/release-manifest.json`.
  Both200/2000m/s hit a1cm blocker123.45m away. This does not prove moving-target,
  water or full weapon-ability behavior; timings were event age, not precise flight time.
- Native attack: `artifacts/harpoon-cannon/attack-signature-audit.json`,
  `attack-struct-audit.json`, `attack-constructor.json`,
  `register-attack-thunk.json`, `register-attack-body.json` and
  `register-attack-directional.json` in the same directory.
- Accepted firing cooked JSON: asset-inspections/20260917T073452782Z-d8ffa0e9
  (shot) and20260917T073455225Z-bafa3057(station), beneath artifacts.
- Dismantle repair: `artifacts/harpoon-cannon/dismantle-modern-fallback.json`
  and `dismantle-safe-01/release-manifest.json`.
- Scoped audits: `artifacts/harpoon-cannon/verify-direct-damage.py` and
  `verify-dismantle-provider.py`. These are checkpoint assertions, not general tools.

No automatic rollback requested. Restore only through exact receipts if needed,
after checking the game is closed. Preserve local HarpoonCannon.ini preferences.

## Version gate and owning references

Steam25191271; executable SHA256
`747DC2553F7E68D8EA7ED0B2E0CAC6D08943EA3F50DD6ED822E9293E0B45F58B`.
Game UE5.8 / editor5.8.2. Recheck before reusing game-derived inputs.
Reviewed mapping: `mappings/Voyage/steam-25191271-ue5.8/Voyage-25191271.usmap`.

- [Architecture and findings](../mods/HarpoonCannon/RESEARCH.md)
- [Native entry contracts](../mods/HarpoonCannon/VEHICLE_ENTRY_RESEARCH.md)
- [Provenance](../mods/HarpoonCannon/GAME_DERIVED_SOURCES.md)
- [Tool pitfalls and recovery](../mods/HarpoonCannon/PIPELINE_OBSERVATIONS.md)
- [Rules](../mods/HarpoonCannon/AGENTS.md)

## Tool-use checkpoint

Accepted damage build passed producer compilation/generation/cook,13 tagged
headers, container verification, public Mod JSON audit and installation readback.
User supplied the runtime damage/kill result. Cleanup build/generate/cook,
13 tagged packages and container verification passed. Public Mod JSON plus
verify-direct-damage.py --clean confirm200 damage, one attack/single-hit guard,
per-click spawning and removal of shot visuals/status/timed exit.
verify-dismantle-provider.py passes all7 handled/empty paths.
Evidence: asset-inspections/20260917T082831498Z-56d2fe7e(shot) and
20260917T082834402Z-a5030772(station), beneath artifacts.
First cleanup compile found one stale renamed constant reference; fixed and
rebuilt with fresh output identity. Python was absent from PATH; audits used the
bundled runtime executable. No shared-tool implementation inspection, installation
or new reusable workflow was needed. All recurring asset/release operations used
documented entry points; no uncovered recurring operation identified.
