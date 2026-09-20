# Harpoon pipeline: durable pitfalls and recovery

Use tools/README.md for current public interfaces. This document records
contracts and failure modes, not a chronological series of experiment reports.

## Native GLB material import

The import commandlet must enable client material data (`IsClient=true`, as the
stock Python commandlet does). With `IsClient=false`, native Interchange can
report missing parent parameters and serialize colorless material instances
despite successful generation/cook. Require cooked color/PBR parameter evidence;
parent references and exit0 alone do not validate imported materials. The GLB
shell preserves native hierarchy and uses a derived package inventory rather
than the former fixed four-mesh/palette layout. Runtime acceptance is separate.
Current acceptance, pending gates and installation receipts live only in the
[active backlog](../../docs/harpoon-cannon-backlog.md).

## Build and Blueprint contracts

- Start Unreal producers outside the sandbox. Exit -532462766 /0xE0434352
  with UBT Trace.uba denial is a launch-permission failure, not a project defect.
- Compile/cook/container success cannot validate guessed native CDO layouts.
  Require tagged station packages and independent semantic property inspection.
  Partial action structs belong in named bytecode fields, not native defaults.
- Overrides require the exact declaring UFunction owner and required interface
  membership. A compiled same-name method is not proof of runtime dispatch.
- Editor collision defaults do not prove cooked Interact=Block; inspect serialized
  responses. Place diagnostic markers before optional guards when investigating.
- GET_FUNCTION_NAME_CHECKED proves a C++ member exists, not that it is reflected
  or callable. GetTransform is the reflected actor transform getter;
  K2_GetComponentsByClass is the actual member behind the display label.
  Overloaded GetOwner needs an explicit signature and known reflected identity.
- Const BlueprintCallable functions can generate pure nodes: GetActorEyesViewPoint
  has no execution pin. Do not connect Exec merely from its Callable annotation.
- Array wildcard calls require UK2Node_CallArrayFunction where applicable.
  Actor-first array inference accommodates Pawn; Pawn-first can reject Actor.
  Class pins are not UObject pins. Optional serialized properties may be absent.
- By-reference FText needs a connected conversion when a literal is unsupported.
  TextRender's callable setter is K2_SetText, not merely reflected SetText.
- MinimalAPI declarations may lack exported symbols. EnhancedInput requires its
  own module dependency. Keep Engine includes outside anonymous namespaces.
- Changing module Editor/Runtime may leave stale UHT flags; inspect cooked flags.
  Disable unused plugins that mutate source config rather than bypassing hashes.
- Virtual/final bytecode forms differ. Audit semantics/owners, not one spelling.
  Fix an assertion against same-candidate evidence before repeating a cook.

## HUD, cameras and assets

- Input mapping, native action provider and HUD rendering are separate layers.
  Hiding HUD does not disable character actions; native handoff owns control.
- Identify the actual HUD producer/widget. Text in an overlay is not a native
  hint. World-space text is not a stable screen diagnostic.
- Continuous native property writers can overwrite one-shot changes. Preserve
  lifecycle ownership; do not reintroduce global HUD suppression as a workaround.
- FirstPersonCamera position is not interchangeable with GetActorEyesViewPoint
  during station control. Component-origin stability does not prove bone stability.
  Detailed accepted camera observations belong in RESEARCH.md.
- UImage.SetDesiredSizeOverride modifies live Slate, not necessarily serialized
  asset state. Optical mask required explicit FSlateBrush.ImageSize=2048x512;
  valid texture pixels alone did not prevent a cooked 0x0 brush.
- Telescope's InitialLockFOV5 is five degrees, not angular x5. Equipment animation
  notifies target the equipped slot; borrowing them is not a safe camera lifecycle.
- GLB hierarchy and its registry own model nodes and role bindings; gameplay
  roles, Q collision and attachment remain independent of visual bounds. The
  generated inventory drives shell packages; do not hard-code mesh counts.
- Read settings via the fingerprint-validated native LoadFileToArray contract in
  GAME_DERIVED_SOURCES.md. First-time user config provisioning was a one-off gap,
  not justification for a new general tool. Preserve preferences on cleanup.

## Evidence and tool failure interpretation

- Imported StaticMesh ray-tracing support must agree with the authoring project's
  cooked resources. On Steam25191271 / UE5.8 (editor5.8.2), importing supported
  meshes but cooking with r.RayTracing=0 caused a null-resource crash before the
  construction preview appeared. Enabling ray-tracing resource cooking resolved
  placement, construction and entry in the user's 2026-09-16 test. This changes
  the mod authoring project, not user game graphics settings. Container/semantic
  success alone did not catch the missing resource; revalidate on engine changes.

- Use exact absolute ModContainer for candidate inspection. Relative paths can
  resolve beneath game Paks. Do not use an accidentally mounted installed mod as
  proof of candidate ownership. Never manually manipulate the shared game store.
- Summary omissions are not proof of absence. Historical native Path/ResolvedOwner
  references and WidgetBlueprintGeneratedClass were omitted by some summaries;
  use public Get-VoyageAssetJson when the supported summary is insufficient.
- Complete JSON and optional pseudocode are separate outcomes. Historical
  FinalFunctionCleaner/WeakObjectProperty failures produced incomplete outputs.
  Check returned paths, errors and actual artifacts; do not consume failed staging.
- Historical enum/native-registration decoding was a bounded research fallback,
  not a maintained generic tool. Revalidate old observations against current tools.
- A failed encrypted config extraction does not establish configuration defaults.
  Missing shader-map data in an inspector is not by itself missing runtime shaders.
- Fingerprint output is JSON text: decode steam.buildId and executable.sha256.
  Keep display game version, Steam build, executable hash and engine identities
  distinct. Changed fingerprint invalidates reconstructed contracts.
- Preserve partial-failure truth: an orchestration wrapper can return normally
  while a nested command failed. Gate dependent operations on exit/status.
- After restoration, filesMatch=false because owned files are absent can be
  expected. Restore evidence and exact predecessor state determine success.

## Caller discipline

- Electrical shell repair: repeated compile/generate patches missed the native
  schema. Query mappings for ancestry, field owner and property type before
  authoring a new component. JSON value shape alone is insufficient. Soft
  references require no object stub; both UObject and UDataAsset are abstract.
  Partial schemas require tagged cook plus independent readback. Tagged enum
  JSON may omit the type prefix; validators may accept both exact spellings
  while retaining the same enum-value check.
- Repair used the public shell producer through build/generate/cook/header gate,
  packaging and semantic verification. First readback stopped on enum spelling;
  subsequent full run passed. No new tool family needed. Research failures were
  a missing process ExecutionPolicy Bypass, an existing mapping-output identity,
  and a relative ModContainer resolved against Paks; reuse returned evidence and
  pass absolute candidate-container paths.

Use quoted exact paths and directory -g searches; PowerShell does not implement
Bash brace expansion and -LiteralPath does not expand wildcard names. Use a true
PowerShell array for a string-array parameter, not comma text through -File.
Avoid full JSON/log/doc dumps and multiline Select-String projections. Read
restart sections and returned narrow summaries before implementations.
A failed guessed member/path is not evidence that the object does not exist.

For commits use the public exact-path semaphore wrapper; inspect untracked files
as well as tracked diffs. A formatting/index gate failure is not a build failure.
Keep model-owned and other agents' dirty changes separate.

## Recovery evidence retained outside routine context

Current exact releases and receipts: active backlog. Historical sources and
reports are recoverable from Git; raw game-derived data stays ignored.

Verified historical archive root: artifacts/archive/harpoon-hc33-20260913.
Each archive has a sibling inventory with original paths, lengths and SHA256.
Verify before extracting selected entries to a fresh ignored directory.

| Archive | SHA256 |
| --- | --- |
| hc33-source-and-methods.zip | 113FF3DA6283628558CFAA048C0023043C827E70EF7C739B4858C7EDD0B759E7 |
| research-history.zip | 64FB30198695BF85B627796F9C7CCB9F4EDF0A8EF04E5FE058A462C4FFA0FA93 |
| retired-experiments-and-logs.zip | 08FBB28E70C328DE826B9416EE7BCAEC0D91CA150DBF96600CBB6D028B156E35 |

Do not overwrite active sources or automatically install historical candidates.
Archive cleanup receipts, not prose totals, own historical file/byte counts.
