# BoatHUDTotalResources rules

These rules apply to the loader-free BoatHUDTotalResources mod in this directory.
Repository rules in `../../AGENTS.md` also apply.

- The last pinned Steam build is recorded in `game-build.json`. The public
  builder may retry a changed build with `-IgnorePinnedGameVersion`; it still
  requires a fresh current original, the supported editor and retoc profile,
  and its structural checks. Offer a game test after a successful candidate.
- Do not add Tick to the inherited Boat HUD child: incomplete ancestor mirrors
  can break Shipping deserialization. A one-shot text write also cannot replace
  the stock updater. Keep repeating aggregation in the separate widget with a
  direct `UUserWidget::Tick`. It obtains the possessed Boat's
  `VoyageModuleComponent`, asks `VoyageModuleSubsystem` for modules in the same
  grid, and sums only `GetResourceAmount(EModuleResourceType::Diesel)`.
- `PetrolTB` is only the stock widget-property name. Any later resource query
  must use `EModuleResourceType::Diesel`; petrol must never be included.
- `BatteryTB` is likewise only the stock electricity field. Its replacement
  must use `EModuleResourceType::Electricity`, convert the base amount to one
  decimal kWh, and must not alter local socket scan text.
- Electricity is enum value `0`, Petrol is `1`, and Diesel is `16` in the
  reviewed mapping used by this mod. Installation must reject a build manifest
  that does not name and encode both displayed resources exactly.
- Replacement text must copy the corresponding stock field's font, color, and
  text-transform policy during `PreConstruct`. Do not rely on UMG's default
  Roboto Bold/white style; it is visibly different from the stock HUD.
- The relocated original is version-bound game data. Keep it under ignored
  `artifacts/`, never in Git, and rebuild it from a clean current extraction.
- UE 5.8 extraction and packaging must use the published retoc compatibility
  build; upstream retoc 0.1.5
  misreads `FObjectImport.PackageName` in filtered cooked packages.
- Never install or remove the mod while Voyage is running. Keep its unique
  container name so removal is exact and does not touch another mod.
- Do not add Tick to the inherited Boat HUD child. Any repeating logic belongs
  in the separate widget whose direct native parent is the engine `UUserWidget`.
- A successful build, cook, container verification, or load is not UI
  validation. Before a release checkpoint, verify both independently known
  resource totals and visual parity after loading a save and after leaving and
  re-entering the Boat whenever behavior changes. An identity-only rename still
  requires at least one real-save load before its checkpoint.
