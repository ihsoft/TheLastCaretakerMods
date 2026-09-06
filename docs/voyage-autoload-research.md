# Voyage autoload discovery: retained conclusions

The disposable DmlAssetRegistryProbe project was retired after the independent
VoyageAutoLoader C8 checkpoint passed its scoped user test. These conclusions
replace the need to retain the probe project, generated content or old backups.
They are observations for Steam build 25056839 / UE5.8.1, executable SHA256
CA84428CF4562C703BEDFF053DB727D14CC70C593451C09BE75A92828EFD9933, not universal
Unreal guarantees. Revalidate game-specific reflection on fingerprint change.

- Working mod classes were loadable while path/all-assets Registry queries did
  not discover their on-disk packages. GetAssetsByPath, GetAllAssets and forced
  ScanPathsSynchronous, including delayed tests and stock controls, did not
  provide complete discovery in the tested game. A zero result is not proof
  that a mod container is absent or that a class cannot be loaded.
- Explicit demand-loading exposed in-memory objects without establishing a
  complete disk registry. Hard references/overrides/shared anchors did not
  establish an enumerable catalog of independent entry actors. A dependency
  from an entry actor to a shared class does not invert that dependency.
- Existing VoyageEditorBlueprintFunctionLibrary.GetAllFilesInDirectory and
  LoadFileToArray are the tested Blueprint-callable route to physical sidecars.
  GetCurrentLevelName/IsPlaying provide positive phase gates; Pawn is not a
  loader requirement. The mod itself owns readiness for controls and UI.
- The independent test13 proved filename discovery to demand-loaded class.
  Test14 added actor creation and BeginPlay with a sequential duplicate guard:
  before counts [0,1], one spawn, final count [1]. This did not prove concurrent
  startup ordering, all-mod compatibility or hot mounting.
- Preserve native reflection identity exactly in editor stand-ins. Same-name
  functions with the wrong declaring owner do not implement the game's contract.
- Blueprint array wildcard types need UK2Node_CallArrayFunction; macro exec-pin
  identities may differ from ordinary K2 function exec pins. Deferred actor
  creation needs both its transform and FinishSpawningActor wiring.
- Shipping PrintString output was not observable in the original tests.
  Concrete SaveGame markers discriminated execution, but compressed markers
  required bounded decoding; file size/plain-text search was not sufficient.
- Empty-map replacement changes menu lifecycle, not just an entry hook. The
  successful independent loader recreates required menu setup and treats DML
  as optional; it does not redistribute DML assets or depend on its parent class.
- C7 and C8 menu/exit failures with the experimental HUD sidecar disappeared
  when only that sidecar was disabled. This implicates HUD activation/UI-lifecycle
  interaction but does not establish a specific root cause.

Current contract, tests and limitations: ../mods/VoyageAutoLoader/STATE.md.
Historical version-specific raw output and rollback packages were explicitly
approved for deletion; old artifact links in the chronology may no longer exist.
