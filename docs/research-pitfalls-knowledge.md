# Research pitfalls: established failure boundaries

Use the section relevant to the current failure. This document keeps reusable
diagnostic distinctions; workflow, accepted implementations and experimental
history belong to their respective owners. An observed failure in one package
does not establish that every similar design fails.

## General

- A successful compile, cook or load does not prove that a callback executes.
  When execution is uncertain, choose a distinguishable observation before
  changing more fields. Repeated no-ops can indicate the wrong owner or producer.
- A restricted process lookup has missed a running Voyage process. For an
  installation check, use a permitted process-detection path with sufficient
  visibility; unknown state is not evidence that the game is closed. Follow
  the [game-experiment rule](game-experiment-rule.md) for mutation safety.
- `IsNumeric` does not strip an INI comment from a value such as
  `150.0 # comment`. This is a preprocessing issue, not a prohibition on
  comments in the [supported INI parsers](runtime-configuration-patterns-knowledge.md#author-one-settings-contract).
- `PrintString` is unreliable telemetry in a Shipping build. Missing text does
  not distinguish an uncalled graph from stripped output. Select an observable
  marker for the question; a diagnostic must not become a permanent feature.

## UObject and Blueprint

An incomplete native mirror can compile and cook while producing an invalid
inherited CDO. Unversioned property indices follow the full shipping schema,
not merely the fields a new graph accesses. A mismatch can decode a value as
another property or cause async-load failure before graph execution.

Establish the required schema or use tagged serialization for the affected
authored package, then inspect the CDO independently. In the inspected UE 5.8.2
cook, omitting `-unversioned` preserves tags. Do not disable
`CanUseUnversionedPropertySerialization` globally: editor duplication also
uses it. Separate cook passes need staging between runs because later cooks
can clear the output directory.

A stored widget field is not proof that the compiled graph or WidgetTree uses
that reference. Likewise, an SCS template and the runtime member can have
different names. Check the actual consumer and identity before treating a
successful write as the requested behavior.

For a relocated game-derived asset, Git eligibility and refresh requirements
follow the [game-derived data rule](game-derived-data-rule.md). Relocation does
not make the asset inherit future stock changes.

## Gyro controls

These are bounded stock-consumer findings, not a complete stabilization recipe:

- Relocating `BP_GyroCopter_Possessable` and substituting a child at its stock
  path has failed at runtime despite successful compile, cook and container
  verification. Compatibility of another vehicle's inheritance arrangement is
  insufficient evidence for this one.
- The stock class-selection field is
  `DA_Item_Module_GyroCopter.DroppedActor`. It identifies a separate boundary
  to inspect when selecting a vehicle class; it does not validate an arbitrary
  replacement by itself.
- A child `ReceiveTick` parent call can resolve to native
  `AActor::ReceiveTick` instead of the intended stock Blueprint behavior.
  Check the declaring function and cooked call target before using it to extend
  the stock Tick.
- The reflected `MeshComponent` property points to runtime `VehicleMesh`.
  Searching UObject names for the property name can select nothing.
- A `PropellerLift` write alone does not establish world-up compensation:
  thrust follows the tilted component's `UpVector`.
- Post-physics correction and child-template tick overrides are separate
  concerns. Check the instantiated component/template settings as well as the
  helper CDO. See [post-physics boundaries](vehicle-and-hud-modding-patterns-knowledge.md#post-physics-kinematic-correction)
  for timing, axis ownership and contact handling.

## Standard input hints

The visible vehicle hint row is not implied by Enhanced Input mappings or a
conveniently named HUD root. Identify the provided-action producer and the
registered renderer using [action/HUD patterns](vehicle-and-hud-modding-patterns-knowledge.md#discovering-voyages-standard-action-ui).

- Do not assign reflected functions to a class from adjacent executable strings.
  Registration tables distinguish, for example, `VoyageInGameVehicleWidget`
  control-setup methods from similarly named input component responsibilities.
- A substitution probe must use an action identity without another active
  conflicting binding. Reusing an existing action can exercise two behaviors.
- Setting `bRegisterFromWidgetOwner` after a nested widget has selected its
  registration branch does not test its creation-time default.
- A free viewport overlay is a diagnostic, not proof of standard-hint lifecycle:
  pause hiding, remapping and device glyphs are separate consumers.
- Description object context, serialized hint composition and the limits of a
  successful overlap correction are owned by
  [the hint contract](vehicle-and-hud-modding-patterns-knowledge.md#generated-property-reads-need-an-explicit-object-context).
  Hiding descriptions or making an action always enabled does not test the
  required enabled/disabled presentation.

## Unsafe class substitution

Replacing a stock Actor, component or widget class changes more than the field
being investigated. Cook and container verification cannot establish its
native lifetime, SCS ownership or interaction behavior.

For stock `BP_Module_Diesel_Container`, a relocated-parent/stock-path-child
substitution failed independently of an added footer override. The failed
arrangement included duplicate scene-root templates; this observation does not
establish the crash's unique cause. For `VoyageModuleSocketViewComponent`,
substituting a Blueprint component child also changed character attachment and
cable behavior. A text producer is not necessarily an isolated text boundary.

An import can have several consumers even when its package path occurs once.
In `BP_VoyageIngameHud`, the `WBP_InteractIndicator` import serves both a CDO
property and an embedded widget instance. A package-path substitution changes
both. Inspect the consumers before choosing a structural edit.

## Asset inspection and delivery

- Use the public [stock asset query](../tools/asset-inspection.md) to separate
  game inputs from installed overrides. A plausible path or container inventory
  does not prove the selected asset is stock. Normal queries do not require
  removing installed mods.
- A package name need not match its internal asset name. Read the generated
  class identity from the asset or a verified reference rather than deriving it
  from a renamed package suffix.
- Blueprint jump addresses use VM `iCode`, not file offsets. A pseudocode
  emitter can display a loop's `EX_PopExecutionFlow` as a return; a genuine
  function exit uses `EX_Return`. For an unexpected decompilation result,
  inspect the affected bytecode rather than changing gameplay to match the text.
- Pure nodes can be reevaluated at later consumers after a mutation. Capture
  values whose pre-mutation state matters and follow
  [graph semantic-preservation rules](blueprint-graph-design-rule.md#preserve-execution-semantics).
- An override created by editor tooling may already contain a connected parent
  call. An unused default return value does not replace that call's result;
  review the saved BPT graph and cooked behavior.
- A USMAP header alone does not establish schema content. A mapping producer
  needs payload/schema validation and a stable live-object snapshot; plausible
  pointer/count checks during process startup have still led to invalid dumps.
  Use the supported [mapping workflow](../tools/mappings.md) and its diagnostics.
- Serialization-layout, opaque-export, JSON-write and reproducibility boundaries
  are owned by [cooked-asset knowledge](voyage-cooked-asset-toolchain-knowledge.md#known-boundaries).
  Keep those checks distinct from runtime validation instead of duplicating
  historical tool versions and completed test results here.
