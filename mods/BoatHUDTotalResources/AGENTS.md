# BoatHUDTotalResources rules

These rules apply to the loader-free BoatHUDTotalResources mod in this directory.
Repository rules in `../../AGENTS.md` also apply.

- The two-resource HUD and its final `BoatHUDTotalResources` identities passed
  real-game validation on Steam build `25056839`: the exact renamed container
  loaded and retained the accepted Diesel and Electricity behavior. Rebuild or
  test installation is allowed only from a fresh current original, the
  installed UE 5.8.2 editor, the reviewed canonical retoc build, and the exact
  static gates. A changed game fingerprint invalidates this checkpoint.
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
- `BatteryTB` is likewise only the stock electricity field. Its replacement
  must use `EModuleResourceType::Electricity`, convert the base amount to one
  decimal kWh, and must not alter local socket scan text.
- Electricity is enum value `0`, Petrol is `1`, and Diesel is `16` in the
  current build `25056839` mapping. Installation must reject a build manifest
  that does not name and encode both displayed resources exactly.
- Replacement text must copy the corresponding stock field's font, color, and
  text-transform policy during `PreConstruct`. Do not rely on UMG's default
  Roboto Bold/white style; it is visibly different from the stock HUD.
- The relocated original is version-bound game data. Keep it under ignored
  `artifacts/`, never in Git, and rebuild it from a clean current extraction.
- UE 5.8 extraction and packaging must use the hash-gated retoc compatibility
  build from `tools/Build-RetocUe58Compatibility.ps1`; upstream retoc 0.1.5
  misreads `FObjectImport.PackageName` in filtered cooked packages.
- Run `Build-BoatHUDTotalResources.ps1` in an environment that permits UnrealBuildTool
  to rotate `%LOCALAPPDATA%\UnrealBuildTool\Trace*.uba`. A sandbox-denied trace
  rotation appears as a `dotnet.exe` dialog and exit `-532462766` / `0xE0434352`
  before UBT reads this project; it is not evidence of a Blueprint, .NET, or
  engine incompatibility. Inspect the captured UBT output before retrying, and
  grant the build that narrow AppData access instead of repeatedly relaunching.
- Never install or remove the mod while Voyage is running. Keep its unique
  container name so removal is exact and does not touch another mod.
- Do not add Tick to the inherited Boat HUD child. Any repeating logic belongs
  in the separate widget whose direct native parent is the engine `UUserWidget`.
- A successful build, cook, container verification, or load is not UI
  validation. Before a release checkpoint, verify both independently known
  resource totals and visual parity after loading a save and after leaving and
  re-entering the Boat whenever behavior changes. An identity-only rename still
  requires at least one real-save load before its checkpoint.
