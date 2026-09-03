# Boat resource socket hover cards

This document preserves the durable producer/consumer findings from the Boat
side-connector investigation. Exact artifacts, package hashes, addresses, and
version-specific reports remain under ignored `artifacts/`.

## Observed data path

The original producer result from Steam build `24990438` was structurally
revalidated on build `25056839`, executable SHA-256
`CA84428CF4562C703BEDFF053DB727D14CC70C593451C09BE75A92828EFD9933`.
`BP_Boat_Base` loads `/Game/Maps/Boats/TestBoat` through its
`VoyageLevelInstanceComponent`. The level contains a real
`BP_Module_Battery` and a real `BP_Module_Diesel_Container`; the large side
connectors are separate virtual socket actors attached to those modules.

`BP_AttachmentVirtualSocketActor_Electricity` derives from native
`VoyageVirtualActorSocket`. On begin play it walks its attachment parents,
resolves the first `VoyageModuleComponent`, and registers its socket through
`AddExternalSocket`; it unregisters through `RemoveExternalSocket` on end play.
The Petrol variant derives from that Blueprint and changes the socket data and
resource. In `TestBoat`, its per-instance port is overridden to
`ResourceType = Diesel` and `ResourceCategory = Fuel`. Therefore neither the
Blueprint class name nor its inherited Petrol default identifies the live
resource; `Port.ResourceType` is authoritative.

All surveyed Boat-side Electricity sockets attach to the built-in battery's
mesh. All surveyed Boat-side Diesel sockets attach to the built-in Diesel
container's mesh. The side connectors are consequently external sockets of
ordinary modules, not special sockets owned directly by `VoyageBoatPawn`.
The built-in Diesel module has local capacity `2000`, matching the stock card's
`1897 L / 2000 L`.

Native `ModuleSocketComponent::GetScanText` is the identified producer of the
middle line. Current-build disassembly revalidated that it resolves
`ModuleOwner`, reads the port resource type, calls that component's virtual
`GetResourceAmount(type)` and `GetMaxResourceAmount(type)`, and writes the
formatted value into `VoyageScanText.Description`. It has no same-grid walk,
no `VoyageBoatPawn` call, and no Electricity-specific aggregation branch. The
interaction HUD is a downstream display consumer of already formatted
action/scan data.

Current-build `VoyageBoatPawn::GetModuleResourcesAvailable(type)` and
`GetModuleMaxResources(type)` are distinct aggregators. They obtain the Boat's
module grid, enumerate eligible modules in that grid, call the same two module
getters, and sum the results. This is the established aggregate operation; the
stock socket scan producer does not use it.

The apparent network-wide Electricity observation was a test-interpretation
error: `6.0 kWh` was the local capacity of the one built-in Boat battery. The
user subsequently confirmed that socket cards do not show whole-network
storage for either Electricity or liquid resources. Battery and Diesel
container both own the same concrete `VoyageModuleComponent`; neither
Blueprint overrides the resource getters or initializes special resource
state in Blueprint `BeginPlay`. There is no resource-specific socket behavior
to fix.

The surveyed indicator creation path does not call
`VoyageBaseUserWidget::SetInteractiveSource` on its button container. A global
replacement consumer therefore cannot reliably recover the exact hovered
Diesel socket through `GetInteractiveSource()` and must not infer it from child
index, localized text, or rendered key labels.

`VoyageModuleResourceWidget` and `DA_Socket_LiquidData_Diesel` are false leads
for this value. The former had no active surveyed reference/call site; the
latter owns meshes, materials, sounds, rotation, and leak effects rather than
text or action data.

## Rejected interception points

- A relocated stock-path `WBP_InteractIndicator` child crashed during async
  loading before its visual marker ran.
- Redirecting the shared indicator import changed both `IndicatorSubClass` and
  an embedded widget-tree instance, so it was not a narrow replacement.
- A structural change to only the HUD `IndicatorSubClass` loaded but produced
  no visible marker on this card. Do not continue by adding content to that
  consumer path.
- A stock-path child of `BP_Module_Diesel_Container` crashed on the GameThread
  both with and without a `GetDescriptionFooter` override. The duplicate
  `DefaultSceneRoot_GEN_VARIABLE` parent template is the leading SCS/archetype
  collision. Success of Actor inheritance on DonkLift does not make this Actor
  package safe.
- Redirecting the two native socket component templates to a Blueprint child
  loaded but stuck the player to the Boat and detached cables visually. The
  exact native component identity is part of attachment, cable, and interaction
  behavior, not just scan-text production.

Do not reinstall or extend these probes. Network-wide Diesel and Electricity
belong in the possessed-Boat HUD feature, not in socket scan text. Preserve the
stock local socket behavior and the Actor, SCS template, component class, and
global interaction-widget consumer.
