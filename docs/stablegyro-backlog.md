# StableGyro active backlog

## Restart

- Task: `SG-20261012-C`.
- Source checkpoint before this candidate: `97b5fdf`.
- Current target fingerprint: Steam build `25880952`, UE `5.8.2` editor /
  `5.8` parser, executable SHA-256
  `16D0726AF6545CDFCCC2722444DB4DAD7236B93514368E8FE4758171E758EF44`.
- The user game-validated settings reload on Gyro entry and the
  `EnablePitchCorrection` enabled/disabled A/B switch on Steam build
  `25880952`. This result does not validate every altitude, horizontal-damping,
  or pitch-physics case.
- User result for installed `v1-test`: the prior changes generally work, but
  forward-pitch integration feels approximately half as fast. Treat this as a
  bounded observation, not proof that removing the old pure reevaluation halved
  sustained integration and not blanket validation of every detailed case.
- Current work: commit the game-tested `v1-test2` settings lifecycle and toggle
  together with their previously uncommitted StableGyro dependencies. Keep
  `PitchRampSeconds=3.0` unchanged.
- Pending gate: the reported forward-pitch speed remains unresolved. The next
  stage is read-only investigation of why it feels approximately half as fast;
  do not change tuning or behavior before that evidence exists.
- Current candidate: installed `v1-test2`, built and container-verified under
  `Tmp/StableGyro/build-20261012-sg12b-v1-test2-r7`; the validated common
  release manifest is
  `Tmp/StableGyro/release-20261012-sg12b-v1-test2-r7/release-manifest.json`
  (SHA-256 `DC1F7ECD66AF6D1C2EA644A3603B45EA5982033D66D50D00D51E557D56E7D308`).
- Installed evidence:
  `artifacts/installations/StableGyro/20261012-231553-v1-test2-736debce/install-manifest.json`,
  with sibling `settings-installation.json`. The triplet and
  `StableGyro_v1-test2.zip` read back with their manifest hashes. The existing
  571-byte INI remains an exact prefix and the new comment/key append produces
  SHA-256 `B4BD775636E5E237EA91EF41D0C5ADCC92B4F9BC8FE4D78F3648852E2256B15C`.
- Next action: after the scoped commit, investigate the forward-pitch speed
  read-only while retaining the current `PitchRampSeconds=3.0` control.
- Owning references: `mods/StableGyro/AGENTS.md`,
  `mods/StableGyro/GAME_DERIVED_SOURCES.md`, and
  `docs/vehicle-and-hud-modding-patterns.md`.

## Current evidence and decisions

- Reviewed current mappings resolved successfully for the installed
  fingerprint. Native reflection comparison retained the previously consumed
  owners, property names/types/flags, function signatures, and action enum
  values. The bounded evidence record is
  `Tmp/StableGyroResearch/revalidation-evidence.json`.
- Current stock assets retain the native `VoyageVehicleGyroCopter` parent, no
  serialized Blueprint functions, stock tilt overrides `15/-15/5`, the
  `VehicleMesh` runtime component identity, the exact stock item `DroppedActor`,
  all 14 keyboard mappings, and no X mapping.
- The native Gyro primary tick is `TG_PostPhysics`. The candidate therefore
  uses the explicit acyclic order player controller -> PrePhysics pitch helper
  -> Gyro -> PostPhysics stabilization helper. UE 5.8 tick prerequisites use
  unique edges, and child-actor attachment does not add an actor-tick edge.
- A one-argument `SetChildActorClass` retained the blank actor template's
  `TG_PrePhysics` override even though the stabilization class CDO cooked as
  `TG_PostPhysics`. The generator now supplies the generated class CDO and sets
  each created child template's tick group and enable flags explicitly. Final
  cooked readback has no child-template tick override, so the pitch and
  stabilization templates inherit their respective PrePhysics and PostPhysics
  class CDO settings.
- The pitch helper writes `TiltForwardInput` from a stored `IntegratedPitch`
  read after the executing setter. This prevents pure-node reevaluation from
  integrating a second time and restores the stored command even when the
  current native marker is neutral.
- `Inspect-VoyageAsset` pseudocode renders the terminal of a standard
  `ForEachLoop` body as `return`, but the cooked expression is
  `EX_PopExecutionFlow`; the following increment and `EX_Jump` return to the
  loop condition. Only the macro's Completed path emits `EX_Return`. Use the
  raw JSON bytecode when distinguishing parser-loop continuation from a real
  function return.
- The current native pitch properties remain protected and Blueprint-readable.
  The editor-only mirror intentionally exposes the existing pitch inputs as
  writable solely so the generator can emit the established Blueprint write;
  this is not a claim about changed native flags.
- Ctrl is the explicit exception to always-on vertical stabilization: it clears
  vertical lock/braking samples without touching Z velocity or location. The
  rotor-gated XY correction remains independent. Releasing Ctrl reacquires via
  the same positive-Vz braking path used after Space release.
- Parent review accepted the final cooked graphs. The common installer then
  installed the explicitly authorized `v1-test` artifact at
  `2026-10-12T22:25:46.7612400Z`, preserving recoverable predecessors. The
  installed INI remains 571 bytes with SHA-256
  `9A4CC8448F715DE28C41F3915D7A394C43320CEB0C20C19D4F8F749C511632EF`.

## Runtime validation plan

After guarded installation, verify W/S integration and neutral hold, both
limits, X reset, exit/re-entry, partial- and full-throttle stabilization,
positive climb braking without an instantaneous Vz loss, exact hold after
braking, Space climb floor, Ctrl descent/landing, Ctrl+Space precedence, Ctrl
release reacquisition, and horizontal damping in neutral/directional/reversing
input. Record the actual game result before changing another variable.
