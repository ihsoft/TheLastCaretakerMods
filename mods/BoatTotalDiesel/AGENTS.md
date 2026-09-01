# BoatTotalDiesel rules

These rules apply to the loader-free BoatTotalDiesel mod in this directory.
Repository rules in `../../AGENTS.md` also apply.

- The aggregation behavior was game-validated on Steam build `23962331`, but
  that cooked package is invalid after the UE 5.8.1 update. A test installation
  for build `24990438` is allowed only from a fresh original, the installed UE
  5.8.2 editor, the reviewed retoc compatibility build, and the exact static
  gates. The resulting container passed primary real-game validation on
  2026-08-31: it loaded, the intended total-Diesel HUD worked, and Petrol
  present in ship tanks did not enter the sum. Do not call leave/re-enter
  behavior validated without explicit evidence.
- The Tick candidate is rejected: its incomplete ancestor mirrors crash
  shipping async loading with `Bad export index 66559/5` while deserializing
  the generated child CDO. Do not restore or layer changes onto that
  architecture.
- The bound `PreConstruct` candidate is game-validated: it changed the render
  opacity of the live inherited `PetrolTB` to `0.15`, while stock logic restored
  the normal litre text afterward. This proves class interception and inherited
  field access, and proves that a one-shot text write cannot replace the stock
  updater.
- The replacement-slot candidate is game-validated: `TOTAL SLOT` replaced only
  the lower-right litre value while retaining its stock icon, panel position,
  and Boat HUD lifecycle.
- The aggregate architecture was game-validated on Steam build `23962331`: the
  lower-right Boat HUD displayed `18966 L`, matching the intended total diesel
  supply while preserving the stock icon, position, and neighboring values.
  It keeps the replacement-slot composition and gives the separate widget a
  direct `UUserWidget::Tick`. It obtains the possessed Boat's
  `VoyageModuleComponent`, asks `VoyageModuleSubsystem` for modules in the same
  grid, and sums only `GetResourceAmount(EModuleResourceType::Diesel)`.
- `PetrolTB` is only the stock widget-property name. Any later resource query
  must use `EModuleResourceType::Diesel`; petrol must never be included.
- Petrol is enum value `1`; Diesel is enum value `16` in the current build
  `24990438` mapping.
  Installation must reject a build manifest that does not name and encode
  Diesel exactly.
- The relocated original is version-bound game data. Keep it under ignored
  `artifacts/`, never in Git, and rebuild it from a clean current extraction.
- UE 5.8 extraction and packaging must use the hash-gated retoc compatibility
  build from `tools/Build-RetocUe58Compatibility.ps1`; upstream retoc 0.1.5
  misreads `FObjectImport.PackageName` in filtered cooked packages.
- Never install or remove the mod while Voyage is running. Keep its unique
  container name so removal is exact and does not touch another mod.
- Do not add Tick to the inherited Boat HUD child. Any repeating logic belongs
  in the separate widget whose direct native parent is the engine `UUserWidget`.
- A successful build, cook, container verification, or load is not UI
  validation. Before a release checkpoint, verify the value after loading a
  save and after leaving and re-entering the Boat. Petrol exclusion was already
  game-validated on build `24990438`.
