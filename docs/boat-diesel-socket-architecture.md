# Boat Diesel socket hover card

This document preserves the durable producer/consumer findings from the
paused Boat side-connector investigation. Exact artifacts, package hashes, and
version-specific reports remain under ignored `artifacts/`.

## Observed data path

On Steam build `24990438`, the large side connector belongs to
`/Game/Blueprints/Modules/Storage/BP_Module_Diesel_Container`. Its two visible
ports are native `VoyageModuleSocketViewComponent` templates with
`Port.ResourceType = Diesel`; the owning module has local capacity `2000`.
That local module amount/capacity is why the stock card showed
`1897 L / 2000 L` instead of the ship-grid total.

Native `ModuleSocketComponent::GetScanText` is the identified producer of the
middle line. It resolves `ModuleOwner`, reads the port resource type, calls the
module-local amount and capacity getters, and writes the formatted value into
`VoyageScanText.Description`. The interaction HUD is a downstream display
consumer of already formatted action/scan data.

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

Do not reinstall or extend these probes. If this feature resumes, first
revalidate the producer chain on the current game fingerprint, then seek a
narrow hook around the existing native producer without changing the Actor,
SCS template, component class identity, or global interaction-widget consumer.
