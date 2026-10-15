# Large-container capacity through INI: feasibility

## Restart and decision

User subsequently reported personally installing the supplied v4-test candidate
and confirming that it works. This is user-reported runtime success; the exact
installed hashes and game fingerprint were not independently read back, and
individual cache/restart/overfilled-inventory test cases were not enumerated.
The earlier installation/runtime gate below describes the build-time state.

CAP-02 flat-format candidate v5-test-20261015-054306 is built and accepted by
static review: 19 cooked assertions passed, section state/checks removed,
standalone comments and shared CDO cache preserved. It was subsequently installed
with hash readback and the user confirmed it works. Individual edge cases were
not enumerated. Identity and effort are in the ledger. The next narrow change,
CAP-03/v6-test, renames the external key to LargeContainerMaxWeightKg; parent
performs this known bounded edit/build directly under the routine-operation rule.
Candidate v6-test-20261015-065220 built successfully with 19 cooked assertions,
was installed with hash readback, and was user-confirmed working with a temporary
1000 kg test value. Authored INI and embedded default remain 200 kg. Exact
installation identity and validation boundaries are in the mod backlog.

CAP-01 implementation authorized 2026-10-14 (UTC 2026-10-15). User chose
extending ContainerLidLabel rather than a separate DML lifecycle manager, with
200 kg as the default. No hot reload/open hook. Parent owns research/review;
/root/container_capacity_coder owns the existing mod's source and build.
At that initial gate only build preparation was authorized; subsequent
installation and documentation-only commit instructions are recorded in the
mod backlog and process ledger. After runtime confirmation the user authorized
committing the implementation while excluding the personal 1000 kg test setting.
Process observations live in [the task ledger](container-capacity-process.md).

Selected candidate: preserve stock lifecycle calls, then apply the INI limit
in ReceiveBeginPlay and OnPersistentActorPostLoad(int32 Version). Use the native
setter, preserve inventory contents, and refresh the existing load-bar fraction.
One global Large-only value in Content/Paks/ContainerLidLabel.ini,
LargeContainerMaxWeightKg (previously MaxWeightKg). CAP-02 removes the unnecessary section header and section gating
at the user's request, matching the flat Railgun/Gyro convention. Standalone
semicolon/hash comments remain supported; inline comments are outside scope.
Missing/invalid setting defaults to 200 kg.

User refinement: do not read the file per container. Cache once on the mod's
generated Large child class-default object (CDO), with owned transient loaded
and value fields. Both lifecycle paths obtain that exact CDO, initialize its
cache if necessary (including caching fallback), and apply its value to the
instance-owned inventory. Parser execution on the CDO must have no world,
latent, inventory or visual side effects. Never mutate a stock CDO. This avoids
an extra runtime package, world manager and instance discovery. Cache lifetime
is the loaded class/CDO lifetime, not a promise of permanent process rooting;
class unload/reload may reread. Document full game restart to apply INI edits.

Current native metadata confirms the bridge
VoyageMiscBlueprintFunctionLibrary.GetClassDefaultObject: Class ClassProperty
at offset 0, ReturnValue ObjectProperty at offset 8, parameter size 16; function
outer constructor matches that library's class registration. Evidence:
Tmp/container-capacity/cache-api-{candidates,owner}.txt and
cache-api-signature.json. UE5.8 Actor.cpp ProcessEvent explicitly permits CDO
execution without a world; the selected parser uses only file/path/string/math
calls. Runtime cache behavior still requires a real-game test.

Current fingerprint is now Steam 25929985, executable
608A4BCD1A0A392477970F455234571946D061E61C1D27F82DE9589EBBA3D69F.
The reviewed resolver succeeds with mapping hash
4EA5D93EBF41FDE3C45C7DB938763E942179DC8EF24951A97EDB94FB1FA9E2FF.
The earlier 25902891 missing-mapping result below is superseded history.

Fresh static evidence: Large has no own functions and its overridden inventory
template sets MaxWeightLimit=200. PlayerBox BeginPlay calls the weight-limited
base; that base calls SetMaxWeightLimit(MaxWeightLimit). PlayerBox PostLoad
has one int Version parameter and a BP_Container_Base super function; its body
updates color/icons. Inventory pointer ownership remains
BP_Inventory_Container_Base_WeightLimited. These are build inputs, not proof
that the candidate covers all restore timing in gameplay.

Current native metadata confirms LoadFileToArray(FString InPath) returns
TArray<FString>: string parameter at offset 0, string inner plus return array at
offset 16, parameter structure size 32. Its outer constructor matches the
UVoyageEditorBlueprintFunctionLibrary registration. SetMaxWeightLimit has
NewDefaultMax float at offset 0, structure size 4; current stock bytecode
establishes VoyageInventoryWeightLimitedComponent ownership. The bounded
metadata read used current executable-inspector references and UE5.8
UObjectGlobals.h FFunctionBaseParams/FPropertyParams layouts; no code injection.
Evidence is Tmp/container-capacity/current-native*. Game-derived outputs remain
untracked. Do not transplant these addresses into production.

Candidate v4-test-20261015-050916 passed 17 static assertions and parent review;
the user subsequently installed it and reported runtime success as stated above.
Exact release/evaluation identity is in the process ledger. CAP-02 is a scoped
parser simplification and rebuild; installation is not requested in this turn.
Detailed unenumerated runtime checks remain: nondefault value, old/new containers
and save/load.
Default 200 alone cannot distinguish a working reader from fallback. Change
INI after the first initialized box to test cache retention, then restart to
verify reread. The selected native bridges are statically revalidated; actual
CDO cache lifetime and restore ordering remain runtime gates.

## Evidence and version boundary

- Current Steam build: 25902891. Executable SHA-256:
  AF69E3A0D24A9D3BA0AE0C5424F7803F3401D5E4B7A4417EAF7F41CD8B6E9FE5.
- Get-VoyageMappings.ps1 reports no reviewed mapping for this fingerprint.
  No generation or extraction with an older mapping was attempted.
- Current executable inspection finds names SetMaxWeightLimit,
  GetCurrentMaxWeightLimit, MaxWeightLimit, bAllowBeyondWeightLimit,
  OnInventoryMaxWeightLimitChanged and LoadFileToArray. Name presence is not
  proof of signatures, property flags, owner, implementation or event coverage.
- Earlier native and game evidence in
  [inventory architecture](voyage-item-fabrication-and-pickup.md#component-and-interaction-contract)
  establishes a mass limit separate from integer Capacity. SetMaxWeightLimit
  updates the base value and recomputes an effective value with modifiers.
  Railgun gameplay showed that setting the serialized field alone left an
  effective 100-kg inventory; calling the setter corrected the behavior.
  These are prior-build observations, not validation on 25902891 or Large boxes.
- Current Railgun source still emits this setter call in RailgunInventory.cpp.
  StationSettings.cpp reads an external INI through the game's LoadFileToArray.
  StableGyro documents game-validated rereading on vehicle entry on 25880952.
  Reuse the method after revalidating native contracts, not old extracted input.

## Candidate behavior

1. Read a mod-owned mass limit from an external INI, validate it, and apply it
   using SetMaxWeightLimit on the exact Large container's existing inventory.
   Do not recreate inventory or remove contents to make them fit a lower limit.
2. For restart-based configuration, apply to restored/existing containers and
   new or streamed instances after stock initialization. Creation-only handling
   does not establish coverage for saved boxes. A DML implementation needs an
   event ownership path; one initial enumeration may cover already-live boxes.
3. Rereading on opening could allow edits without restarting the game. The
   reliable pre-transfer interaction event has not been located on this build.
   An additive DML actor cannot simply override the stock actor's Open function.
4. Applying only on opening may leave other transfer routes and closed-container
   indicators on the previous value. Inventory admission and presentation must
   consume the same effective limit; the feature is not merely UI formatting.

The most economical initial scope is restart-based configuration applied to
loaded and newly appearing instances. Entry-based rereading is a subsequent
option once a suitable event is established. INI parsing and setter application
have repository precedents; instance coverage/order is the main unknown cost.

## Limits and next discriminator

Ordinary Unreal automatic INI loading requires configured classes/properties;
arbitrary Blueprint component defaults are not made configurable merely by
writing a matching key. See Epic's
[configuration documentation](https://dev.epicgames.com/documentation/en-us/unreal-engine/configuration-files-in-unreal-engine).
The exact Config flags of the current game property were not established.

A later authorized test should distinguish effective limit from display: change
one existing and one newly created Large box, attempt a transfer across the new
limit, and reload. Lowering below existing contents additionally requires proof
of preserved contents, available withdrawal and correct further-deposit rules.
Batch, partial, automated/proximity transfers and modifier interactions remain
unverified. No game launch/test is requested for this quick feasibility pass.

## Tool-use report

Public fingerprint succeeded; mapping resolver stopped at the new-build gate.
Public executable inspector succeeded with seven string matches. Temporary
evidence is under Tmp/container-capacity. An initial output extension was
rejected (.txt is required); a comma-joined powershell.exe -File query produced
a literal no-match and was rerun with an actual PowerShell string array.
The corrected report is native-weight-separate.txt. Existing source/docs and
Epic documentation supplied the remaining evidence. No tool source inspection,
tool changes, Git mutation or reusable pipeline expansion was necessary.
