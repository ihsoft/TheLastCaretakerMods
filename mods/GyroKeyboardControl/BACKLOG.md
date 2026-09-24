# GyroKeyboardControl active backlog

## Restart

- Current game fingerprint: Steam build `25191271`, executable SHA-256
  `747DC2553F7E68D8EA7ED0B2E0CAC6D08943EA3F50DD6ED822E9293E0B45F58B`.
- The first runtime candidate is rejected: relocating the stock Gyro Blueprint
  and replacing its original package with a child produced a world-bound Gyro
  whose driver could not exit.
- Root-node experiments were rejected as the explanation: the validated
  DonkLift child contains the same generated default-root pattern.
- Current source leaves the stock Gyro Blueprint in place, generates
  `/Game/Mods/GyroKeyboardBP/BP_GyroCopter_KeyboardCtl`, and patches
  `DA_Item_Module_GyroCopter.DroppedActor` to select that subclass.
- UBT, both generators, narrow cook, equal-length DroppedActor patch, package
  inventory, `retoc verify`, installation, and installed hash readback pass.
- First validated subclass candidate: `artifacts/gyro-keyboard/build-20260922-023927`, archive
  SHA-256 `CD003C4D86F508BA931760F9A8282B5C3131796C52C00C2BBB71EAA3BBC169AD`.
- Installed semantic readback proves the container has exactly the helper,
  subclass, and patched item data asset; it does not override the stock Gyro
  Blueprint. The subclass directly inherits the stock Blueprint and the decoded
  DroppedActor path selects the subclass.
- The user confirmed the current subclass architecture, pitch controls, and
  standard X HUD hint work in the real game on 2026-09-22.
- The X hint is supplied through `IAV_GyroPitchReset`, an
  exact stock-mapping-preserving replacement of `IMC_GyroCopter_Keyboard`, and
  a child `GetProvidedActionsBP` override that calls the stock Blueprint parent
  implementation before appending reset.
- Editor module build, Blueprint generation, and four-package cook passed in
  `artifacts/gyro-keyboard/build-x-hint-20260922-03`; that agent run stopped
  safely before stock extraction because Voyage was running. The user then ran
  the public producer and validated the resulting package in Voyage.
- The user game-validated the current two-setting build on 2026-09-24:
  `PitchRampSeconds=3.0` and
  `AltitudeStabilizationVerticalDeceleration=100.0` both work. This supersedes
  the earlier one-sided Space-floor checkpoint as the current validation gate.
- Current installed candidate:
  `artifacts/gyro-keyboard/build-remove-lift-compensation-install-20260924-03`,
  archive SHA-256
  `2DBE231F13536A34760E1D6900178F5EEA68855C6515A1D7999776F9CD2E6581`,
  UTOC SHA-256
  `6699C19248414448EE2605EAF47F89FA2892A92B1706431AE8BA347F0E8783CF`.
  Its cooked structure and installed hashes pass, and its two active settings
  are game-validated.
- The user tested the first v2 experiment in game. Removing the raw
  ThrottleInput contribution prevented useful climb: at 100% lever the Gyro
  only hovered, and below 100% it descended. The complete GetTotalThrottle
  override and its INI flag are therefore rejected and removed; stock
  Space/Ctrl behavior is authoritative.
- The first lift correction was absent or visibly insufficient. Its property
  target and type are correct (`PropellerLift`, `DoubleProperty`), but
  `75/cos(10 degrees * integrated pitch)` adds only about 1.54% at maximum
  pitch and almost nothing near neutral.
- The next v2 candidate keeps `CompensateTiltLift=true` and adds
  `TiltLiftCompensationMultiplier=1.0`, clamped to 0..100. It sets lift to
  `75 + multiplier * (75/cos(10 degrees * integrated pitch) - 75)`, so the
  coefficient scales only added compensation and does not change neutral lift.
- Revised candidate built and installed from
  `artifacts/gyro-keyboard/build-lift-multiplier-20260922-02`; archive SHA-256
  `9B310665C766AC17502290853B0BD72CB6CFD5C0F41DB52D8062438E0557ECE0`,
  installed UTOC SHA-256
  `620217C044DD9EB79E52AFF497DEC517D4A005FCF4AA1EAE82A7A86AE37039A9`.
  The installer removed the retired INI key and added the multiplier while
  preserving existing pitch-ramp and compensation values.
- Exact installed-container pseudocode confirms the 0..100 INI clamp, the
  intended scaled-extra-lift formula, and the DoubleProperty write to
  `PropellerLift`. The cooked child has no GetTotalThrottle override or
  throttle-related state.
- The user tested `TiltLiftCompensationMultiplier=10.0` and observed no
  noticeable effect. The installed INI contains that exact value. Raw helper
  bytecode confirms its ForEachLoop uses execution-flow push/pop and therefore
  continues past PitchRampSeconds to the multiplier; the pseudocode renderer's
  apparent `return` is not an early exit. UE 5.8 engine source confirms
  SetDoublePropertyByName finds an inherited FDoubleProperty on the runtime
  class and writes it in the object container, while stock Gyro bytecode never
  assigns PropellerLift after its default.
- The user then tested `TiltLiftCompensationMultiplier=100.0`; the installed
  INI contains that exact value, but there was still no noticeable improvement.
  Stock bytecode proves held Space does not write PropellerLift. It raises
  GetTotalThrottle, which increases Physical Energy production; UpdatePropeller
  consumes that resource to apply main-rotor torque, increasing
  PropellerVelocity. Final lift multiplies the independent values
  `TiltControl.UpVector * PropellerVelocity * PropellerLift * heightFactor`.
  PropellerLift and PropellerVelocity are both doubles, and the intermediate
  FVector uses 24-byte double components, so a computed change from 75 to about
  190.7 cannot be lost to rounding. Both proposed explanations are therefore
  ruled out: stock boost neither overwrites the field nor rounds away our
  value.
- Editor build, both generators, and four-package cook pass in
  `artifacts/gyro-keyboard/build-lift-throttle-fixes-20260922-05`. One warning
  is limited to the unshipped dummy parent's generated pure-function return
  node. The public producer stopped safely before stock extraction because
  Voyage was running.
- Pending: do not request more coefficient tuning. The next candidate must
  make the write observable: either show the loaded coefficient/calculated
  lift and read back the live property, or use an exact inherited-property set
  inside the stock-parent child graph. If reflection is proven live, replace
  target-pitch geometry with the actual world vertical component of
  TiltControl.UpVector.
- Active controlled probe: while the Gyro is player-controlled, the helper
  writes a hard-coded `PropellerLift=300` every tick, independent of INI and
  pitch. This is intentionally unsafe production behavior and must be removed
  after one short neutral-pitch test. A strong continuous climb proves the
  reflection target is live; no effect falsifies the helper reflection path;
  a transient or pulsing effect indicates a later writer in the tick cycle.
- Probe generation and cook first completed under
  `build-fixed-lift-probe-20260922-01`, then the public producer stopped before
  stock extraction because Voyage started. A second non-installing run reused
  the fingerprint-matched clean originals and produced
  `artifacts/gyro-keyboard/build-fixed-lift-probe-20260922-02` without touching
  the running game. Archive SHA-256:
  `CAD05670F3A5E433160673C163DCC642E4FDD13F30544BDC5814C1E63B28F8FB`;
  UTOC SHA-256:
  `608CDA6EEA05FD6467B39423C20770322CCF0E9B2BF37C3AB46F5CE4F5E33684`.
  Exact external-container pseudocode is
  `SetDoublePropertyByName(Gyro, "PropellerLift", 300)` with no INI or pitch
  dependency. Pending installation after Voyage closes.
- Probe was rebuilt and installed from
  `artifacts/gyro-keyboard/build-fixed-lift-probe-20260922-03` after Voyage
  closed. Installed UTOC SHA-256:
  `1BC49C209193FB22BE5619D5C850DB0C4F46516F220B3B63C0DF79FF005BD731`;
  UCAS SHA-256:
  `C36AEA92FF58366255BFDC92F87427B9FCFF91DBA87CFAD62D6EF0661A10D084`;
  archive SHA-256:
  `11F4D3CAF2418FFFE261F343B079567B66AD23A0597D6C151433ECB3D0C9AE91`.
  Installed hashes match the manifest, and exact installed-container
  pseudocode again confirms the literal reflection write to 300. Await the
  short neutral-pitch gameplay result before changing anything else.
- Gameplay result: the hard-coded 300 probe produced an obvious rapid climb
  when Space spun up the rotor. This proves the helper writes the live
  PropellerLift used by physics and the game does not overwrite it before the
  force calculation. The crash after extended extreme-speed flight is not part
  of the current diagnosis; the deliberately unsafe probe has been removed.
- The replacement candidate finds the stock `TiltControl` scene component by
  its stable SCS name marker, reads live world-space `UpVector.Z`, and computes
  `75 + multiplier * (75 / max(UpVector.Z, 0.1) - 75)`. It resets
  PropellerLift to 75 whenever the Gyro is not player-controlled.
- `v2-world-lift-test` was built and installed from
  `artifacts/gyro-keyboard/build-world-lift-20260922-01`; archive SHA-256
  `22965088343B4D7580606C53E8A1713EE6D6B34C5F1063BF62198AF593B763BC`,
  installed UTOC SHA-256
  `D07EE4A69DBC806C2239642E0D424D4AB63BD06CCB2C5A80A4BCF3260A59FD4C`.
  Exact installed-container pseudocode confirms component enumeration/name
  match, GetUpVector.Z, clamped division, scaled correction, reflection write,
  and the uncontrolled reset to 75.
- The uncapped world-vector formula was rejected before gameplay: the
  `max(Z, 0.1)` divisor allowed up to 10x geometric lift and an old diagnostic
  multiplier of 100 made the candidate unsafe. Standard `PrintString` cannot
  provide telemetry because UE 5.8 compiles its implementation out of Shipping
  unless shipping logging is explicitly enabled.
- Current installed candidate: `v2-safe-world-lift-test`, built from
  `artifacts/gyro-keyboard/build-safe-world-lift-20260922-01`. Archive SHA-256:
  `207929F493E0BE4C78CCADB2064B13D1B6A143CE1A88D1627A028EAD5400BBED`;
  installed UTOC SHA-256:
  `A0F5512AC8801CE9F6AAA9A7624263EB32A9BF47FBA8F0499F1266E5FC8D383D`.
  Installation migrated the old multiplier 100 to 1.0 and added
  `MaxTiltLiftScale=2.0`.
- The safe formula is
  `75 * (1 + multiplier * (clamp(1/max(Z, 1e-6), 1, maxScale) - 1))`
  only for `Z > 0`; otherwise it writes stock 75. The multiplier is clamped to
  0..1 and maxScale to 1..4. Exact installed-container pseudocode confirms the
  settings parser, live `TiltControl.GetUpVector().Z`, both clamps, the `Z > 0`
  gate, the final `PropellerLift` write, and the uncontrolled reset to 75.
- Gameplay result: even with a user-selected coefficient of 5, the
  PropellerLift-scale candidate did not provide clearly visible altitude
  support at modest pitch. In that candidate the old multiplier was clamped to
  1 and MaxTiltLiftScale to 4, but the architecture also coupled compensation
  to the tilted rotor vector. That approach is now retired.
- Current installed candidate: `v2-world-up-force-test`, built from
  `artifacts/gyro-keyboard/build-world-up-force-20260922-01`. Archive SHA-256:
  `70E781E925743155F796BEB1D7F21F15E180E658FBFA0489F4468A7B96733A8C`;
  installed UTOC SHA-256:
  `21C8FCCB484D067E971C39996EA05F57AAB964F463535BAE4BEED5200AD4B291`.
  The installer removed retired `MaxTiltLiftScale`; the installed multiplier
  is 5.0 and the runtime clamp is 0..5.
- This candidate never writes PropellerLift. Every controlled tick it finds
  exact `TiltControl`, `Propeller`, and `MeshComponent` component identities,
  reproduces stock `PropellerVelocity = VSize(angular velocity) * 50`, and adds
  only `(0,0,F)` at the physical mesh center of mass. `F` is the missing
  vertical factor times PropellerVelocity, stock lift 75, the stock altitude
  factor clamped to 0..1, and the INI multiplier. Exact installed-container
  pseudocode confirms all three component matches and `PhysicsMesh.AddForce`.
- Pending gameplay discriminator: at multiplier 5, visible altitude support or
  climb during pitch proves the separate force path. No effect points to
  component-name/lifecycle or physics-tick ordering despite the correct cooked
  graph; horizontal acceleration should not increase in either case.
- Gameplay result: the user observed no difference with the installed
  world-up formula and confirmed `TiltLiftCompensationMultiplier=5.0` in the
  INI. Do not continue coefficient tuning on that candidate.
- Active controlled probe: only while W or S is held and
  `CompensateTiltLift=true`, add a constant `100000000` world-up force to the
  discovered PhysicsMesh, independent of angle, rotor speed, altitude, and
  INI multiplier. Releasing W/S removes the probe force. This is intentionally
  crash-capable and must be removed after one short test.
- Probe built and installed from
  `artifacts/gyro-keyboard/build-world-up-force-probe-20260923-01`. Archive
  SHA-256:
  `F55F18C809C49E0F302FD956AE9B164B17C160C8E341E7DAE37B5BAC913880EF`;
  installed UTOC SHA-256:
  `697A1ACF3115E1E7835EC47040CB4E3EB7B29271F7D97378A2B03E1D12F02B39`.
  Exact installed-container pseudocode confirms the W/S OR condition, enabled
  flag, literal 100000000, world-up vector, and
  `PhysicsMesh.AddForce(..., false)`.
- Probe discriminator: a violent upward response proves the force target and
  call timing are live, leaving the calculated inputs/magnitude as the defect.
  No response falsifies the current MeshComponent/AddForce path. Unexpected
  rotation or motion identifies a wrong physical target or force semantics.
- Gameplay result: W/S still drove the expected Gyro/rotor behavior, but the
  100000000 probe produced no extra motion. The exact cause is now identified:
  `MeshComponent` is the Blueprint property name, while its referenced runtime
  UObject is named `VehicleMesh`. The helper compared GetObjectName against
  `MeshComponent`, never populated PhysicsMesh, and exited at IsValid before
  AddForce. Retest the same probe with the exact runtime name `VehicleMesh`;
  do not draw conclusions about AddForce timing or force semantics from the
  invalid first probe.
- Corrected probe prepared, not installed, under
  `artifacts/gyro-keyboard/build-world-up-force-probe-vehiclemesh-20260923-01`
  while Voyage remained running. Archive SHA-256:
  `AB573CC6C0DF08B4F61BD2D50DF9F1127F0E870960BE769FE5256FD317358332`;
  UTOC SHA-256:
  `3BA2E313326893FAB231B3949B95A5E1400EE540B159F614C2A1A73201F60740`.
  Exact external-container pseudocode confirms the `VehicleMesh` match,
  IsValid gate, literal 100000000, and PhysicsMesh.AddForce. Install only after
  Voyage closes, then repeat one very short W/S press.
- After Voyage closed, the corrected probe was rebuilt and installed from
  `artifacts/gyro-keyboard/build-world-up-force-probe-vehiclemesh-install-20260923-01`.
  Archive SHA-256:
  `B542ECA4C79B506DF7BB14A1704379E26BF26C62E7A9E64497DC4961F5056C5D`;
  installed UTOC SHA-256:
  `7AE4987055F9F9EC61E7A54A9A17042787EC46A2197359B440693F4BFBDE0842`.
  Installed hash readback matches the manifest, and exact installed-container
  pseudocode again confirms VehicleMesh, the W/S gate, literal 100000000, and
  PhysicsMesh.AddForce.
- Gameplay result: neither a short nor long W/S press caused ascent. A long
  press only produced the normal rotor response. Inspection then exposed a
  second probe defect: the pure W/S equality nodes were evaluated again after
  the graph had replaced raw `TiltForwardInput` with its small integrated
  value. The later force gate therefore saw neither -1 nor +1 and always
  selected zero force. This result did not exercise AddForce and does not
  falsify its target or semantics.
- Current installed probe captures the raw W/S equality result in
  `PitchCommandActive` before writing integrated pitch, then uses that stored
  boolean for the force gate. It was built and installed from
  `artifacts/gyro-keyboard/build-world-up-force-probe-captured-input-install-20260923-01`.
  Archive SHA-256:
  `B8436F8A311A1DC6C96A3A33A81CCDE967E99C945C93838DC168DBF2C00CAB32`;
  installed UTOC SHA-256:
  `C06698F2FB058825D570CDF67FFEFD11D5FF7231A67D454AF37A97390CCF1074`.
  Exact installed-container pseudocode proves the capture occurs before the
  `TiltForwardInput` write and that the later enabled gate reads
  `PitchCommandActive`; it also confirms exact VehicleMesh discovery, literal
  100000000, and `PhysicsMesh.AddForce(..., false)`. Await one very short W/S
  gameplay test; a violent upward response is the expected positive result.
- Gameplay result: the corrected probe produced the expected obvious upward
  effect. This proves exact VehicleMesh discovery, AddForce call timing, and
  world-up force semantics are all live in gameplay. The earlier no-op results
  were probe defects, not evidence against this compensation architecture.
- The crash-capable literal and its temporary captured-W/S state have now been
  removed from source. The production candidate applies only
  `(1 - clamp(UpVector.Z, 0, 1)) * PropellerVelocity * 75 * heightFactor * multiplier`
  along world up whenever compensation is enabled. It deliberately does not
  depend on W/S being held, because released input preserves the current pitch.
- The corrected production container was built under
  `artifacts/gyro-keyboard/build-world-up-force-corrected-install-20260923-02`,
  but Voyage started during the run, so the public producer stopped safely
  before archive creation or installation. External UTOC SHA-256:
  `9D02272ACF6AA91248ACC20B0B9DF6D29F17618B25442105FD82A6C96DC69F93`.
  Exact external-container pseudocode confirms VehicleMesh, the calculated
  force formula, the INI enable gate, and AddForce; it contains neither
  PitchCommandActive nor the 100000000 literal. Install only after Voyage
  closes, preserving the user's current multiplier 5.0 for the next test.
- After Voyage closed, the corrected production candidate was rebuilt and
  installed from
  `artifacts/gyro-keyboard/build-world-up-force-corrected-install-20260923-03`.
  Archive SHA-256:
  `70F8C6D3C8A0BEFCBE31CE03DA7441FC38CDB14B39E665D3F4BE256C82A3834E`;
  installed UTOC SHA-256:
  `4C9E656E7278AD9400FBA0A0B22DE0B6AD47CCA3B25DD8F24D530EEF2FC2F478`.
  Installed-container pseudocode confirms exact VehicleMesh discovery, the
  calculated missing-vertical-force formula, the INI enable gate, and
  AddForce. It contains neither the 100000000 probe literal nor
  PitchCommandActive. The installed INI preserves multiplier 5.0. Await
  gameplay comparison at modest held pitch and after releasing W/S.
- Gameplay result: with `TiltLiftCompensationMultiplier=5.0`, forward pitch
  now produces an unmistakable increase in altitude. This validates the
  complete production path in game: live TiltControl world-up sampling,
  Propeller angular-speed scaling, exact VehicleMesh targeting, calculated
  force magnitude, and world-up AddForce. The remaining work is calibration,
  not architecture. Because 5.0 visibly overcompensates and the formula at
  1.0 replaces exactly the stock vertical component lost to tilt, test 1.0 as
  the next neutral-height candidate before changing the formula.
- Follow-up gameplay result: multiplier 1.0 was clearly insufficient, and 5.0
  only produced an initially visible climb before longer flight and maneuvers
  again led to descent. The user rejected further physical-force tuning and
  requested an arcade altitude mode instead.
- Arcade contract: when stock Blueprint `CurrentThrottle` reaches 1.0, capture
  current actor Z once. Without Space, restore that exact Z every helper tick.
  While Space is held, treat the stored Z as a one-sided altitude floor: if
  current Z rises, do not move the actor and raise the stored floor to that Z;
  if current Z is equal or lower, restore the floor. Disable and clear the lock
  below 1.0. During correction preserve X/Y position and horizontal physics
  velocity while setting vertical velocity to zero.
- A first cooked implementation put this graph in the replacement child's
  ReceiveTick. Inspection showed its parent call compiled as the native final
  `AActor::ReceiveTick`, not a proven call to the stock Blueprint Tick. It was
  rejected before installation because it could suppress the stock Gyro
  physics graph.
- The replacement architecture now leaves the child without ReceiveTick and
  augments the already separate ModActor helper during inheritance generation.
  The helper casts its parent to the exact relocated stock Blueprint identity,
  reads inherited `CurrentThrottle`, checks physical Space independently, and
  applies the arcade location/velocity correction. The old world-up-force
  branch is hard-disabled in cooked bytecode and calls AddForce only with zero;
  remove that dead branch and retire its settings after gameplay validation.
- The arcade candidate was built and installed from
  `artifacts/gyro-keyboard/build-arcade-altitude-hold-install-20260923-01`.
  Archive SHA-256:
  `D4447F10B625E6D0C39385DCE12D1E161E964B19EE3B52F77B383EA611A79046`;
  installed UTOC SHA-256:
  `C460318BD8564ABD91323469016300A5512B46EB112BD879E6722097E14CAB0F`.
  Exact installed-container inspection confirms the child has no ReceiveTick,
  while the helper has the exact CurrentThrottle/Space gates, first-frame Z
  capture, subsequent K2_SetActorLocation, horizontal-velocity preservation,
  vertical-velocity zeroing, and lock reset on control exit. Await gameplay
  validation of full throttle, Space hold/release, below-full throttle, pitch,
  exit, and re-entry.
- Gameplay result: exact full-throttle stabilization works. At high pitch,
  pressing Space exposed a transition gap: clearing the lock immediately let
  the Gyro fall before the stock physical boost spun up the rotor.
- The one-sided Space-floor candidate was built and installed from
  `artifacts/gyro-keyboard/build-arcade-altitude-floor-install-20260924-01`.
  Archive SHA-256:
  `45B0F12E5238ED3EC6D5A03B07B977F63A6A4B3E8BD7554496633F44F65D3A44`;
  installed UTOC SHA-256:
  `E7E6B8B7415FA2B233D84C70D90DA1B19F2CDB43583B26363FF7496CA4660849`.
  Exact installed-container pseudocode confirms: below full throttle clears the
  lock; an inactive full-throttle lock captures Z; without Space it restores
  exact Z; with Space and `current Z > LockedAltitude` it only advances
  LockedAltitude, otherwise it restores Z and zeroes only vertical velocity.
  The child still has no ReceiveTick. Gameplay validation passed: at full
  pitch, holding Space before the stock boost spins up prevents the fall; once
  physical lift exceeds the floor the Gyro climbs normally, and releasing
  Space holds the latest reached altitude.
- The next arcade refinement delays only the initial full-throttle lock while
  the Gyro is still ascending. `AltitudeStabilizationDelaySeconds` is a
  fractional INI value, defaults to `1.0`, and is clamped to `0..60`; `0`
  restores immediate activation. If world vertical velocity reaches zero or
  becomes negative before the timeout, the current altitude is locked
  immediately. Dropping below full throttle or losing player control clears
  both the lock and elapsed delay.
- The non-installing candidate was built under
  `artifacts/gyro-keyboard/build-altitude-lock-delay-20260924-02`. Archive
  SHA-256:
  `3DC4590E142273246C5A797B470819A6D8E9B62C01DBFD2F1EF2DEBDFD6F69A3`;
  UTOC SHA-256:
  `08D95D862E5F5E411B991983E0A383E051DCA7D2F4D55AC865B852874DB9DB7E`.
  Exact external-container pseudocode confirms default `1.0`, INI clamp
  `0..60`, `Velocity.Z > 0`, elapsed `+ DeltaSeconds`, timeout activation,
  immediate activation for `Z <= 0`, and timer resets after activation, below
  full throttle, and loss of control. The existing exact hold and one-sided
  Space floor remain unchanged.
- The gameplay candidate was rebuilt and installed from
  `artifacts/gyro-keyboard/build-altitude-lock-delay-install-20260924-01`.
  Archive SHA-256:
  `3EEDAEF42B99D5FEA371C12D77566B4CF61E1C43610E3EDB74AEC9F4FB2AC0ED`;
  installed UTOC SHA-256:
  `6B3D0BEBDE2AA54C106A00606670C47C33356581B4D6626E654924259AAE7EBF`.
  Installed hashes match the release manifest. Installation preserved the
  user's existing settings, including multiplier `5.0`, and appended
  `AltitudeStabilizationDelaySeconds=1.0`. Exact installed-container pseudocode
  confirms the same delay contract. Pending real-game validation: while still
  ascending at full throttle, stabilization waits up to the configured time;
  if ascent ends or descent begins first, it locks immediately; below-full
  throttle resets the pending delay; setting `0` restores immediate locking;
  existing Space-floor behavior remains intact.
- Gameplay result with delay `7.0`: the delay was only occasionally visible.
  Releasing Space stopped the climb immediately and allowed a small visible
  drop before the held altitude was established. Cooked inspection identified
  two causes: the inactive path captured only the current Z after vertical
  velocity reached zero or became negative, and activation returned without
  applying location or vertical-velocity correction until the next helper
  tick. The delay itself correctly bypassed on `Velocity.Z <= 0`.
- The corrected candidate records the maximum actor Z throughout the pending
  interval. On activation it locks that peak and, when Space is released,
  applies the location correction and zero vertical velocity in the same tick.
  If Space remains held and vertical velocity is still positive, immediate
  correction is skipped so the commanded climb continues and the established
  one-sided floor can advance normally.
- The corrected candidate was built and installed from
  `artifacts/gyro-keyboard/build-altitude-lock-peak-install-20260924-01`.
  Archive SHA-256:
  `DAB7C16898F172C168EF88F70116D200442A032ABDBB16EE856AE1D9E128191D`;
  installed UTOC SHA-256:
  `DE989E2AB36D28CB9F71D64BC3A288238FE7CEA145A2652DBCF53386B47228D4`.
  Installed hashes match the manifest, and the user's delay `7.0` was
  preserved. Exact installed-container pseudocode confirms peak-Z tracking
  before the velocity decision, peak capture into `LockedAltitude`, same-tick
  correction without Space, and the Space-plus-positive-ascent bypass.
  Gameplay result: stabilization still activated immediately after releasing
  Space and the Gyro still settled slightly lower. Peak tracking did not make
  the transition clear enough; the remaining immediate activation was the
  intentional `Velocity.Z <= 0` bypass.
- The next discriminator makes `AltitudeStabilizationDelaySeconds` absolute.
  Once `CurrentThrottle >= 1`, the inactive lock accumulates `DeltaSeconds`
  regardless of altitude, velocity, direction, or Space state. It captures the
  current Z and activates only when elapsed time reaches the configured value.
  Dropping below full throttle or losing control still resets the timer; `0`
  still means immediate activation. Peak tracking and its state were removed
  to keep this experiment single-variable.
- The absolute-delay candidate was built and installed from
  `artifacts/gyro-keyboard/build-absolute-altitude-delay-install-20260924-01`.
  Archive SHA-256:
  `2AE22BEF1A2971EC8B4B148D7DDFD683040833BB213F13C05930907CDFFA4A3A`;
  installed UTOC SHA-256:
  `1678EF5378300DCC3892FD0E0ABA3B6A96EB3DB6CC29EC8D0FFFA80F6C67D775`.
  Installed hashes match the manifest, and the user's delay `7.0` was
  preserved. Exact installed-container pseudocode confirms that the inactive
  path contains only elapsed-time accumulation and comparison before capturing
  Z; `GetVelocity` occurs only after activation for the established Space-floor
  behavior. This experiment exposed that the timer's trigger must be the Space
  release, not merely full throttle.
- An intermediate Space-release candidate was installed from
  `artifacts/gyro-keyboard/build-space-release-delay-install-20260924-01`;
  archive SHA-256
  `4EED372A61FECA364ABC5678DDF8B074DA4761AE898938FE642A7651C5F5E272`,
  installed UTOC SHA-256
  `7D319FB27DACC5EE13B740143626B9DA5D6A4ECDA6B80D253D788D10A2427019`.
  It correctly reset the absolute timer while Space was held and started it on
  release, but could still initialize later if Space had been released below
  full throttle and throttle subsequently reached 100%. Do not validate that
  edge against this intermediate installed build.
- Final clarified contract: only a `pressed Space -> released Space` transition
  whose release tick has `CurrentThrottle >= 1` arms the absolute delay. A
  release below full throttle does not arm stabilization. To try again, hold
  Space until throttle reaches 100%, then release it; that release starts the
  timer. While Space is held at full throttle, the one-sided floor remains
  active and the delay is disarmed and reset. Dropping below full throttle or
  losing control clears the lock, release history, armed state, and elapsed
  time.
- The gated candidate is prepared, not installed, under
  `artifacts/gyro-keyboard/build-space-release-gated-delay-20260924-01`.
  Archive SHA-256:
  `5ADE98B93578DBB1F72D91BB4050EFC3B2910B6A9729BBACCD6E875FA03209A6`;
  UTOC SHA-256:
  `D24623EEA37F4CD3E61F5A784A23A9AB21A0903616EB9C3D7B3B9AB597D1B7D4`.
  Exact external-container pseudocode confirms that full throttle without
  Space returns without initializing a lock; Space held at full records the
  release precondition and resets/disarms the timer; release arms it; only the
  armed state accumulates time; and below-full throttle clears every state.
  Installation was attempted but stopped before mutation because Voyage was
  running. Install after the user closes the game, then validate with delay
  `7.0`.
- The timer experiment is now retired. The user-approved transition preserves
  the same full-throttle Space-release gate but replaces elapsed-time waiting
  with kinematic braking. After a valid release, each tick computes
  `new Vz = current Vz - AltitudeStabilizationVerticalDeceleration * DeltaSeconds`.
  While `new Vz > 0`, only the physical root's Z velocity is replaced; X/Y are
  preserved and actor location is untouched. When the result reaches zero, or
  when observed Vz is already non-positive, the current actor Z is captured
  and exact altitude hold activates in that same tick. Space cancels braking
  and restores the validated one-sided floor; throttle below 100% or loss of
  control clears every stabilization state.
- `AltitudeStabilizationDelaySeconds` was removed from the canonical INI,
  schema, generated runtime settings, and cooked graph. Its replacement is
  `AltitudeStabilizationVerticalDeceleration`, in cm/s^2, default `100.0`,
  clamped to `1..10000`. The installer removes the exact retired key and its
  canonical comments, preserves all unrelated user settings, appends the new
  default only when missing, and deduplicates its own upgrade heading.
- The first vertical-braking candidate was built and installed from
  `artifacts/gyro-keyboard/build-vertical-braking-install-20260924-03`.
  Archive SHA-256:
  `CE6836E873A35B1DEB189638C921873A3CE93B0FE58874EE5B8445EA81F0C456`;
  installed UTOC SHA-256:
  `F26071A9C8BAFF5A0365F30D7BD6DE43FE6027CB2B37F187A507854A130735CC`;
  installed UCAS SHA-256:
  `079FE076613A627B4004F38B900393567DD851229CCC0D11D9BF34EE05ED0BD7`.
  Installed hashes match the release manifest. The migrated installed INI
  preserves `PitchRampSeconds=3.0`, `CompensateTiltLift=true`, and the user's
  `TiltLiftCompensationMultiplier=5.0`; the retired delay is absent and the new
  deceleration is `100.0`.
- Exact inspection of the final cooked UTOC confirms the Space-release OR
  braking-state gate, `deceleration * DeltaSeconds`, subtraction from live
  root-component Vz, preservation of live X/Y in `SetPhysicsLinearVelocity`,
  same-tick current-Z capture and exact-hold activation when the reduced speed
  is not positive, and complete absence of the retired delay setting/state.
  Pending real-game validation: release Space at full throttle during a clear
  climb and judge whether ascent decelerates smoothly without a downward snap;
  test an already-descending release, Space re-press, below-full release,
  pitched flight, exit, and re-entry. Tune the new coefficient through the INI
  only after confirming the mechanism is perceptible.

- Cleanup after cooked inspection found that the physical compensation enable
  pin was never connected: both INI settings were parsed, but the cooked graph
  selected zero force unconditionally. The user chose to remove this dead
  experiment. `CompensateTiltLift`, `TiltLiftCompensationMultiplier`, the
  component scan, rotor/orientation/height calculations, cached probe state,
  and `AddForce` node are now absent from source and cooked output. Arcade
  stabilization attaches directly after the controlled `TiltForwardInput`
  write. The settings generator now emits `TArray` setting groups so an empty
  boolean group is valid C++.
- The cleaned candidate was built and installed from
  `artifacts/gyro-keyboard/build-remove-lift-compensation-install-20260924-03`.
  Archive SHA-256:
  `2DBE231F13536A34760E1D6900178F5EEA68855C6515A1D7999776F9CD2E6581`;
  installed UTOC SHA-256:
  `6699C19248414448EE2605EAF47F89FA2892A92B1706431AE8BA347F0E8783CF`;
  installed UCAS SHA-256:
  `995697408A255B007F2B1A84361A5E7A78AC879830D85D761C6D7CC66AFBBD8D`.
  Installed hashes match the release manifest. The first install migration
  removed exactly both retired lift-compensation keys; the final idempotent
  install removed none. The installed INI now has only `PitchRampSeconds=3.0`
  and `AltitudeStabilizationVerticalDeceleration=100.0`. Exact cooked
  inspection finds no retired setting/property names, probe state, PhysicsMesh
  cache, or AddForce, while the pitch write, full-throttle gate, Space-release
  braking formula, and both velocity writes remain present. The user then
  confirmed both active settings work in the real game.
- The installed INI was promoted verbatim to the canonical default after that
  validation, including the user's blank-line layout, warning comment, and
  inline `# cm/s` unit comment. Build-time validation strips a whitespace-led
  inline comment only for numeric contract checking; packaging preserves the
  source INI bytes exactly.
- Final pre-commit verification was prepared without installation under
  `artifacts/gyro-keyboard/build-game-validated-defaults-20260924-01`.
  Archive SHA-256:
  `F90AE857CE81743BBF7E1DF248E05D1CAC5E98B9484F261A823A1E090D69F28C`;
  UTOC SHA-256:
  `99FD753B3E37EC6413B102B6331674480D3190CD8ECF741249D9E6323444B2AB`.
  The generated, packaged, source, and installed INI hashes are all
  `DDED605386E1EE3D1B73DECC2E35FBA713C6066139CE4EE693B22758F053FD9A`.
  The generated runtime default remains numeric `100.0`, while the packaged
  file retains the inline unit comment byte-for-byte.

## Owning references

- `AGENTS.md`
- `GAME_DERIVED_SOURCES.md`
- `docs/research-pitfalls.md#управление-gyro`
