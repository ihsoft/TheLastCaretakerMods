# Research pitfalls: observed results and limited conclusions

These plausible approaches caused a crash, a no-op, or an incorrect architecture.
Keep the distinctions below in mind after game updates or context compaction.

## General

- A successful compile or cook proves only that the editor setup accepted the
  graph, not that runtime calls it. Each new callback needs a distinguishable
  call marker and an in-game check.
- Repeated no-ops usually point to the wrong ownership chain or producer rather
  than another wrong structure field. After one or two no-ops, strengthen the
  marker or revisit the architecture.
- Do not change controls and HUD in the same experiment. That combination has
  caused autonomous throttle or steering and obscured the cause.
- Do not replace installed mod files while the game is running.
- A negative process lookup from a restricted sandbox does not prove the game is
  closed: a running Voyage process was invisible there. Before installation or
  live diagnostics, check through an allowed outside-sandbox path and treat an
  unknown process state as blocking.
- Do not preserve an extracted game snapshot as knowledge. Without provenance,
  it soon becomes a plausible but stale trap.
- `UKismetStringLibrary::IsNumeric` rejects values such as `150.0 # comment`.
  If a runtime graph uses this check, an inline comment silently leaves the
  built-in default in place. Put canonical INI comments on separate lines and
  have the producer reject inline comments before building.

## UObject and Blueprint

- Different Lua wrappers may refer to the same UObject. Wrapper identity is
  not a stable actor identity.
- An editor mirror for a child Blueprint with unversioned properties needs more
  than the field accessed by a new graph. Cook serializes the inherited CDO by
  editor-class schema indices, while runtime reads it against the full shipping
  schema. An incomplete mirror can compile, cook, and pass `retoc verify`, yet
  fail during async loading with `Bad export index` before any graph runs. The
  Boat HUD child failed this way at
  `BP_VoyageIngameBoatHud.Default__BP_VoyageIngameBoatHud_C`, index `66559/5`.
  Establish the exact native property prefix and check the cooked CDO against
  the shipping mapping before runtime testing.
- Writing a new UObject into a widget field does not guarantee that the
  compiled Blueprint or `WidgetTree` uses the reference.
- A copy of the original Blueprint at a new package path preserves behavior
  only as a snapshot of the current game. Re-extract it for each applicable
  game version; do not commit it.

## DonkLift controls

- A helper attached to each forklift must not target global `GetPlayerPawn(0)`.
  After exit, the cast loses the abandoned forklift and cannot zero its
  throttle; with two forklifts, multiple helpers can write to the same active
  pawn. Use the child actor's `GetParentActor()` and `IsPlayerControlled()` as
  the ownership boundary.
- Exact `-1`, `0`, and `1` are also digital game commands. Custom active state
  cannot stay at those values; working sentinels are `0.9999` and `0.0001`.
- Polling the keyboard separately from the game's axis write caused races and
  stuck input. The validated UE4SS path is a pre-hook on the native getter
  after the game's write.
- Clearing `SteeringInput` on exit stops input but need not visibly straighten
  the wheels; forklift physics stores their position elsewhere as well.

## Gyro controls

- Do not relocate stock `BP_GyroCopter_Possessable` into a mod package and put
  a child at the original path merely because this inheritance pattern works
  for DonkLift. In the tested configuration, the Gyro appeared fixed to the
  world and could not be exited after entry. Compile, cook, and `retoc verify`
  all passed. The child's utility `DefaultSceneRoot` is not a discriminator:
  it also exists in the working DonkLift container.
- Keep the stock Gyro Blueprint at its original package identity. The narrow
  class-selection point is `DA_Item_Module_GyroCopter.DroppedActor`; connect a
  separate mod subclass through that field instead of relocating the original.
- Overriding `ReceiveTick` in a generated child is not a proven safe way to
  extend stock Blueprint Tick. The graph's parent call compiled to native final
  `AActor::ReceiveTick`, not a proven stock Blueprint call, and could suppress
  stock physics. Put added movement in a separate helper unless the exact
  Blueprint parent call is established.
- Writing `PropellerLift` reached the live field but gave no useful
  compensation: thrust still follows the tilted `UpVector`, and even at a
  10-degree tilt the `1/cos(10 degrees)` factor adds only about 1.54%. A
  reachable field and an observed write do not establish the right control.
- A reflected Blueprint component-property name can differ from the runtime
  UObject name. Gyro's `MeshComponent` points to `VehicleMesh`; looking up
  `GetObjectName == "MeshComponent"` found no target, making even a huge
  `AddForce` a no-op.
- After using `VehicleMesh` and saving the W/S discriminator before changing
  input, a world-up `AddForce=100000000` probe caused the expected strong lift.
  The `AddForce` path and its timing worked; the physics-compensation tuning,
  rather than the ability to apply force to Gyro, was rejected.
- Blueprint pure nodes reevaluate at each use. A probe that first changed W/S
  state and then reused pure equality nodes as a gate saw the changed value.
  Save the discriminator before mutation or carry it through the exec chain.
- Re-reading a pure integrator expression after its setter can put the written
  native output one additional step ahead, while the stored accumulator still
  changes only once per tick. That graph alone does not establish sustained
  doubling of integration speed.
- `Inspect-VoyageAsset` pseudocode may print `return` at the end of a standard
  `ForEachLoop` body even though the cooked opcode is `EX_PopExecutionFlow`,
  followed by loop increment and `EX_Jump`. Distinguish early return from loop
  continuation in raw bytecode; a true function exit uses `EX_Return`.
- `PrintString` is unreliable telemetry in a Shipping build. Its absence from
  the screen cannot distinguish an uncalled graph from stripped shipping
  output. Use an observable physical marker or a real UI widget.
- A pre-physics velocity write cannot counter force applied by stock code and
  Chaos later in the same frame. A Gyro stabilization helper must observe and
  correct the result in `TG_PostPhysics`; horizontal correction must be an
  additive XY delta so a full XYZ write does not compete with vertical mode.
- Setting `TickGroup` only on the generated helper CDO does not ensure the
  same order for a `ChildActorComponent` instance. One-argument
  `SetChildActorClass` left a `TG_PrePhysics` override on `ChildActorTemplate`
  even though the separate cooked helper CDO had `TG_PostPhysics`. The
  generator must explicitly set and check the child template's tick group and
  enable flags. Cooked readback must confirm either those values or the
  absence of an override with a correct CDO. This establishes a serialization
  contract, not gameplay validation.

## Standard input hints

- `BP_VoyageIngameForklift.KeybindRoot` exists but is not the runtime host of
  the visible `E/H` row. That host is
  `BP_DynamicPlayerInputHorizontalWidget_Bottom.ContextInputActionsRoot`.
- Adding Enhanced Input mappings, action events, or even a serialized
  replacement of `IMC_Forklift_Keyboard` does not make the standard HUD show
  new actions automatically.
- A separate viewport overlay proved the HUD child ran but had the wrong
  lifecycle: it remained visible in the pause menu and did not follow the
  standard hints.
- Do not declare `VoyageVehiclePawn.GetProvidedActionsBP` on an editor-only
  `VoyageVehicleForkliftPawn` stand-in, even if a same-named Blueprint graph
  compiles. The cooked override binds to an exact parent UFunction. Native
  registration records identify the base `VoyageVehiclePawn` as the owner.
  This mistake explains the former `QuitGame` no-op; repair the stand-in
  hierarchy before another probe.
- Do not invent missing enum members for the editor setup. The runtime
  `EPlayerInputInterfaceActionType` is `Central=0`, `Context=1`, `Hidden=2`.
  The old `Action=0` compiled locally but was absent from the runtime enum.
- `InteractiveInterface.GetInteractiveProvidedActions` serves interaction
  with an object from outside. Its presence in the original forklift Blueprint
  does not connect it to the driver's hint panel.
- A direct `VoyagePlayerInputInterfaces.GetProvidedActions` override on the old
  forklift-child stand-in was also uncalled. This does not prove that the
  dynamic HUD queries a different object: the mirrored native/interface
  hierarchy did not yet match the game.
- Do not test a producer by replacing one action field with another field
  already used on the same object. Horn-to-Exit removed a HUD duplicate but
  bound two incompatible behaviors to E: exit stopped working, the horn
  sounded, and the game crashed. Use a unique action with no second active
  binding on that forklift for a substitution probe.
- After independent no-ops from `GetProvidedActionsBP` and
  `GetProvidedActions`, further action-structure changes were uninformative.
  Static analysis then found the wrong owner and exact native producer.
  Another override requires the real base-class hierarchy first.
- Do not assign a function to a class merely because its string is near that
  class's field names in the executable. Generated registration records for
  different types are adjacent. This wrongly assigned
  `SetupPlayerControls`/`TeardownPlayerControls` to
  `VoyageInputControlsComponent`; their owner is `VoyageInGameVehicleWidget`.
  Confirm ownership through the registration table.
- `UUserWidget::GetWidgetFromName` was not available as a Blueprint-callable
  UFunction in the 5.7.4 editor setup. For a HUD child whose real parent is
  substituted only at packaging, declare a same-named field of the right type
  in the placeholder parent and access it as an inherited Blueprint variable.
  Do not invent a native UFUNCTION absent from the shipping class.
- UAssetAPI 2.0 with UE 5.7 support may still leave a game-specific Widget
  Blueprint export as `RawExport`, even when CUE4Parse reconstructs its
  properties with the same mapping. `RawExport` does not mean the object is
  absent, and one matching byte sequence is insufficient for mutation. The
  direct E/H probe first compared Bottom/Center and established the unversioned
  header/value/slot boundaries; its patcher accepts only the full known
  17-byte Voyage 5.7.4 signature.
- Writing `bRegisterFromWidgetOwner=true` from the parent HUD's `Construct`
  had no effect: the nested dynamic-input panel had already chosen its
  registration branch. A late runtime write does not test the serialized
  default at child-widget creation.
- In the investigated game version, the native action path uses a `TSet`,
  not stable sorting by `Priority` or extension-array order. Moving the
  “first child to the end” produced `E H X C` for only one observed sparse-set
  order. Even a correct one-time X/C rearrangement by object identity did not
  survive ESC/resume: the game repopulated the row without reconstructing the
  owning HUD. Continuous polling for cosmetic ordering was rejected. The
  production mod leaves standard-action order to the game and does not
  replace the HUD.

### Description generation and row placement: limits of the evidence

Shared contracts and evidence levels are in the
[action/HUD patterns](vehicle-and-hud-modding-patterns-knowledge.md#evidence-levels-for-actions-and-hints).
They are independent of the mod where the behavior was found. The known native
collection structure alone does not explain the layout failure; a causal link
needs separate evidence.

- **Confirmed statically and in game:** `DefaultObject` on an unconnected
  `UK2Node_VariableGet` target pin did not read the intended InputAction's
  `ActionDescription`. Check `EX_Context` and the actual read object in cooked
  bytecode; explicitly connect a typed object reference. Do not require only
  `EX_ObjectConst`: `UK2Node_Literal` can compile to a reference field with
  the correct CDO default.
- **Confirmed by observation:** Blank labels hide a one-frame text overlap,
  while an always-enabled action avoids the problematic transition. Neither
  proves the layout works with the real `bEnabled` behavior.
- **Confirmed together, not individually:** A serialized stock hint widget
  placed directly in a Canvas under an InvalidationBox worked without the
  observed overlap. Three changes were made together, so the outcome cannot
  be attributed to the InvalidationBox alone. The base HUD class was unchanged.
- **Plausible hypothesis, not an established mechanism:** An intermediate
  layout/initialization state during native row updates could explain a frame
  with missing icons and left-overlapping text. The native child-recreation
  order and need for `ForceLayoutPrepass` are unproven.
- **Confirmed by build:** A stock Blueprint present in the game may be absent
  from the editor project. An empty exact-identity stub can satisfy an external
  reference but must be omitted from the release. Shipping an empty tree in
  place of the real stock class is a different implementation, not an
  equivalent standard HUD.

## IoStore build and delivery

- Blueprint bytecode jump addresses use VM `iCode`, not file offsets or the
  offsets shown by CUE4Parse. Inserting instructions requires recalculating
  affected jump targets and wrapper entry points after serialization. Matching
  displayed offsets prove nothing. The current DonkLift mod no longer uses
  this fragile approach.
- `retoc` v0.1.5 offered a UE 5.7 mode but lacked the import type hierarchies
  needed to parse these Voyage assets through UAssetAPI/UAssetGUI. A CLI
  version option alone is insufficient; test a real round trip and `to-zen`
  on a narrow asset before relying on a particular tool release.
- A legacy `.uasset` extracted by an older `retoc` may have an identical
  `.uexp` and plausible contents yet fail `to-zen` after relocation with
  `failed to fill whole buffer`. Re-extract the original with the selected
  current tool before building, and require successful `to-zen`/`verify` to
  check the header format.
- Extracting from the whole `Paks` directory while an override mod is
  installed may return the replacement instead of the stock asset. Remove
  extra containers with the game closed before refreshing a snapshot. The
  extraction tool now stops on this ambiguity by default.
- A substituted child may look like a plausible “original” and even have the
  expected two occurrences of its own package path. Moving it to the parent
  path turns its existing parent reference into self-inheritance: the
  container passes `retoc verify`, but the game crashes with `STACK_OVERFLOW`.
  Before replacement, reject a source asset that already contains the
  relocated-parent path; after extraction, verify that filtering returned at
  least one `.uasset`.
- A relocated stock forklift Blueprint is a snapshot of one game version.
  Do not commit it as source or reuse it after a game update. Re-extract it
  and verify its provenance before building.
- Substitute package names only between previously checked, equal-length
  paths. Do not weaken assertions to patch a merely “similar” number of
  string occurrences; length changes break the cooked layout.
- Broad `CookDir` followed editor dependencies, compiled global shaders, and
  cooked unrelated Engine/OpenWorld packages. Production cooking uses five
  narrow `-CookSinglePackageNoRefs` runs.
- Separate control/HUD containers helped diagnosis, but the HUD replacement
  was removed from production after forced ordering was abandoned. The
  accepted result is packaged as one `DonkLiftKeyboardControl_P` and checked
  without a leftover probe package.
- `retoc verify` establishes container integrity, not runtime Blueprint,
  input, or HUD compatibility. Cleanup or consolidation still needs a full
  in-game check.
- A `WBP_InteractIndicator` relocation probe for build `24990438` passed
  cook, `retoc verify`, and independent bytecode/CDO analysis but crashed on
  `FAsyncLoadingThread` with a null read before `PreConstruct`. A clean
  two-asset inventory and plausible child CDO do not prove a reconstructed
  native parent is compatible with that Widget Blueprint. Reject that whole
  architecture rather than changing another graph on top of it.
- Do not redirect a package import just because the target path appears once
  in the `.uasset`. In `BP_VoyageIngameHud`, the sole
  `WBP_InteractIndicator` import serves both CDO property `IndicatorSubClass`
  and an embedded widget-tree instance. Equal-length path replacement changes
  both consumers. That probe was rejected statically and never installed. A
  targeted replacement needs a separate CDO delta or a structural new import.
- An inherited CDO property is not safe merely because a reconstructed native
  prefix has the expected size and the generated Blueprint cooks. In
  `BP_VoyageIngameHud` probe3, a value set as `IndicatorSubClass` decoded as
  `ButtonInfoContainer_Action_1` when independently parsed with the original
  parent. This unversioned-property index mismatch was found before
  installation. Such a change needs a structural CDO patch that checks the
  exact property name, not another mirror/child variant.
- In Voyage UE 5.8, `FObjectImport.PackageName` is serialized even for
  filtered cooked packages. UAssetAPI and the original retoc 0.1.5 wrongly
  skipped it under `IsFilterEditorOnly`, shifting an otherwise plausible
  import map. Removing that condition globally is also wrong: filtered UE4
  and UE 5.3–5.7 fixtures use the older layout without `PackageName`. Read
  and write this field only with an explicitly selected `VER_UE5_8`.
  Unchanged round trips must remain byte-identical for both current Voyage
  and older regression fixtures. A new import's `PackageName` must also
  match its `ObjectName`.
- Comparing only two successive local commits cannot establish that existing
  test failures are an acceptable baseline. Commit `48b6096` already had an
  overly broad filtered-import patch that introduced ten binary-roundtrip
  regressions; comparisons with it or `b5c47b7` hid them. For shared
  serialization changes, run the same suite against the actual upstream
  merge base. The control result before and after the correct UE 5.8 gate was
  `27/27`.
- A `.uasset` package name need not match its internal Blueprint asset name.
  In marker package `WBP_InteractIndicator_M`, the internal asset remains
  `WBP_InteractIndicator`, so the generated class is
  `WBP_InteractIndicator_C`. Do not derive the class name from the package
  suffix; compare with a reference import from an actually cooked child.
- `AddFunctionGraph` for a `BlueprintNativeEvent` override may automatically
  create a parent call and connect its return to the result pin. Setting a
  default value without breaking that connection has no effect: the first
  `GetDescriptionFooter` probe decompiled back to a parent call. For a
  constant marker override, disconnect the auto-parent node's exec and value
  links and check the final cooked bytecode.
- A single actor-child probe cannot isolate inheritance when it also adds a
  `BlueprintNativeEvent` override. A `BP_Module_Diesel_Container` footer probe
  loaded its package but crashed on GameThread after 22 seconds while writing
  to `0xe0`. Unlike the earlier async-load/CDO crash, this test combined the
  class substitution and `GetDescriptionFooter`. A control keeping the
  relocation/child while removing only the override was needed; until then,
  only that combined package was rejected.
- That no-override `BP_Module_Diesel_Container` control reproduced the exact
  GameThread crash after 24 seconds: write to `0xe0` with the same stack. Thus
  the stock-path actor-child/SCS substitution itself is rejected
  for this package, independently of `GetDescriptionFooter`. The cooked child
  had a second `DefaultSceneRoot_GEN_VARIABLE` templated from the relocated
  parent's same-named component, the leading archetype-collision mechanism.
  A working child of another Actor, such as DonkLift, does not establish
  safety here; compare SCS/component-template identity and lifecycle too.
- Replacing a native component import with a Blueprint child can preserve the
  original Actor, CDO, SCS owner, and byte-identical `.uexp`, pass cook and
  `retoc verify`, yet violate the class's broader runtime contract. For two
  Diesel socket templates, this did not crash but attached the character to
  the ship and visually detached some cables. Therefore
  `VoyageModuleSocketViewComponent` is more than a text producer: its exact
  native identity also participates in attachment, cable, and interaction
  behavior.
- A fresh file named `Mappings.usmap` does not prove schemas are present. A
  false automatic `GUObjectArray` resolver for Voyage UE 5.8 produced a valid
  28-byte USMAP header with no payload; a complete manual dump was 2,141,952
  bytes. Before asset parsing, a mapping workflow must check its manifest,
  Steam build ID, payload structure, and several required schema
  names. Even a jmap with the corrected UE 5.6+ `MinAlignment` repeated the
  false automatic result. A structural `.data` scan and explicit
  `GUObjectArray` produced the full dump with the required schema content.
  A structurally plausible snapshot alone was still insufficient: while the
  process was warming up, one candidate passed pointer/count checks, then
  requested 231 GB of memory and exited with `0xC0000409`. The same address
  and process yielded the known mapping after stabilization. Require both a
  minimum process age and several consecutive matching snapshots of address,
  pointers, and object/chunk counts before dumping.
- Windows exit code `0xE0434352` by itself means an unhandled managed
  exception, not a broken CLR. For the recurring dialogs, the `.NET Runtime`
  log showed expected `InvalidDataException`, `DirectoryNotFoundException`,
  and LINQ assertion failures from earlier asset tools and
  `VoyageAssetInspector`, not a runtime fault. CLI tools should catch
  top-level exceptions, print a concise error, and exit with code `1` so a
  diagnostic failure does not look like a .NET crash.
- Two `retoc to-zen` calls on an identical staging tree may write chunks in
  different physical order. Whole-file `.ucas/.utoc` SHA-256 values then
  differ although chunk IDs, asset paths, sizes, and content hashes match.
  Compare independent builds through a sorted semantic inventory. Temporary
  copy-integrity checks are still needed during installation.
- Passing `UAsset.VerifyBinaryEquality()` does not prove UAssetGUI parsed every
  export. `RawExport` preserves unknown bytes and can yield byte-identical
  unchanged serialization while the UI reports `Failed to parse N exports`.
  The reverse is possible too: an asset without raw fallback may change bytes
  when saved. Batch open checks must independently require zero `RawExport`
  and positive binary equality. Exceptions and unapplied mappings are separate
  failures.
- CLI `UAssetGUI fromjson` does not accept an engine version and is therefore
  not equivalent to a normal GUI Save of an asset opened explicitly as UE 5.8.
  For Voyage, it may rebuild JSON into a mixed layout: one layer follows the
  JSON or older-version data while the filtered-import header follows the
  current API condition. Until the interface and regression test are fixed,
  validate a modified pair by direct opening with `VER_UE5_8`,
  `VerifyBinaryEquality`, rewriting in the same mode, and an independent
  CUE4Parse/`retoc` pass.
- Dynamically building a Blueprint schema requires more than its immediate
  dependency asset. Unversioned-property indices cover the entire parent
  chain. Also, do not leave an asset path permanently in
  `PathsAlreadyProcessedForSchemas` if preload ended with missing dependencies:
  after extracting a transitive parent, the next pass must retry that asset.
- A fresh USMAP does not turn native custom serialization into an ordinary
  reflected structure. `InstancedPropertyBag` in `ABP_HologramPedestal` began
  with its own four-byte `bHasData`. Parsing it as mapped
  `InstancedPropertyBag.Value` shifted the stream to a false `FPackageIndex`.
  Support only established format variants and explicitly reject unknown
  ones rather than reading adjacent bytes as properties.
- A native enum imported but absent from mappings does not require invented
  members. For an unchanged round trip, an explicitly marked numeric fallback
  (`UASSETAPI_INVALID_ENUM_IDX_<n>`) is acceptable if the underlying property
  width is known, bytes are preserved, and the stress report remains a
  `notice`. In current Voyage this applies to
  `/Script/FunctionalTesting.EComparisonMethod`.
- `RigHierarchy` and `RigVM` have their own native serializers. Do not treat
  them as ordinary `NormalExport` or claim they are fully parsed. Exact opaque
  preservation may pass unchanged binary equality, but must remain a separate
  `notice` and does not permit structural edits.

## UE4SS HUD

- `pcall` does not protect against a crash inside native marshaling.
- A Lua table such as `{ KeyName = "W" }` is not a real `FKey` for
  `IsInputKeyDown` and has caused a native crash.
- Constructing a new `FText` required a version-pinned constructor signature.
  Recheck even a unique pattern after every executable change.
- `KismetInternationalizationLibrary.GetCurrentLanguage` did not reflect the
  Voyage menu language. The validated source was
  `VoyageGameUserSettings.CustomSettings.LanguageType`.
