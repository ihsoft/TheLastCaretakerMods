# MooringCable60m architecture

The public behavior and build interface are in [README.md](README.md).
The mod supports Steam build `25956314` (game version `0.10.0.653158`).
Connected cables retain the 2000 cm limit; a free end can deploy to 60 m.
Existing saves resolve the replacement cable class. The native payout and
force contract below was inspected on build `25191271`; it was not independently
re-derived for `25956314`. The builder extracts and checks fresh stock assets
on each run.

## Stock length ownership

`/Game/Blueprints/Cables/BP_ModuleCable_Base` derives from native
`ModulesConnectCable` and sets `MaxLengthBase = 2000` cm. The stock
`BP_ModuleCable_Mooring` child inherits that value. The male mooring socket's
`VoyageModuleSocketViewComponent` selects `DA_Cable_Mooring` as its
`IntegratedCable` and sets `IntegratedCableLength = 2000` cm. The item selects
`BP_ModuleCable_Mooring_C` as its cable class.

Native integrated-cable creation copies the socket's `IntegratedCableLength`
into the cable actor's `MaxLength`. Deployment checks both positive limits,
`MaxLength` and `MaxLengthBase`. `CableSnapLength` is separate from this
deployment limit.

## Runtime packages and class identity

The release contains exactly three packages:

- The male mooring socket keeps its original virtual identity and supplies
  `IntegratedCableLength = 6000` cm for newly created integrated cables.
- A fresh stock `BP_ModuleCable_Mooring` is relocated to
  `/Game/Blueprints/Cables/BP_ModuleCable_Orig060`; its only intentional data
  change is `MaxLengthBase = 6000` cm.
- A generated child of that relocated class occupies the original
  `/Game/Blueprints/Cables/BP_ModuleCable_Mooring` identity. Keeping that
  identity allows existing saved cables to resolve the replacement class.

The child's `ReceiveBeginPlay` starts a repeating 0.05 s object-bound timer.
`MooringRefreshLimit` checks native `CachedSocketA` and `CachedSocketB`:
when both are valid, it sets `MaxLength = 2000` cm; otherwise, it sets
`MaxLength = 6000` cm. The resulting behavior keeps the stock automatic
payout ceiling for connected cables while allowing manual deployment of a free
end to 60 m. Strength, stretch, tightness and snap defaults remain stock;
other cable classes are untouched. Existing long connected cables are not
forcibly shortened by this graph; native reeling continues.

The child has no active SCS root nodes and its CDO overrides only
`UberGraphFrame` and the inherited `RootComponent`. The minimal native mirrors
exist only in the editor build. The child is cooked with tagged properties so
mirror property indices cannot enter an unversioned runtime CDO. The relocated
stock package retains its original property serialization. No editor placeholder,
native DLL or autoload actor is shipped.

## Connected cable mechanics

The native payout path adds segments using the actor's `MaxLength` as a ceiling
for eligible endpoint ownership. The exact meaning of its endpoint-owner
predicate has not been established as a reusable contract. The 20 m connected
payout setting is not a hard geometric limit under load: connected cables can
stretch farther while towing.

The connected force path calculates
`D = CableLengthCurrent - (1 + CableTightness) * NumSegments * CableWidth`.
It applies no force from that path when `D <= 0`. For positive `D`, force
includes
`(D * CableStretchForceMultiplier + CableStretchForceOffset) * CableStrength`,
followed by velocity, mass, clamping and timestep terms. The mod does not
adjust those force parameters.
