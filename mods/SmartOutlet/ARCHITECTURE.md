# Smart Outlet architecture

## Behavior and identity

Smart Outlet adds selectable smart input/output modes to resource-storage sockets.
`SmartOutlet` is the technical identifier; **Smart Outlet** is the display name.
The generic controller is `BP_OutletController`.

The [commented INI](Assets/SmartOutlet.ini) owns settings and defaults. Each
optional section is named `<Blueprint>.<Class_C>:Input` or `:Output`, using the
owner's class object path without its folders. Lookup is case-sensitive and
uses the actual owner class; it does not inherit a base class's settings.
`BP_StorageProfile` caches the two independently validated mode rules by class,
including classes with no supported modes. Configuration is read once per session
by the mod-owned copy of the shared sectioned INI parser.

Every native socket on a supported storage can select its own mode. New ports
start normal. The mapped Interact 3 action (V by default) cycles smart input,
smart output, normal, skipping absent or invalid modes. If neither mode exists,
the mod offers no action. The hint reads `Set <next> (now: <current>)`; smart mode
labels include their actual Enable/Disable percentages. Normal releases control
back to the game. The supplied defaults offer battery output and fresh/salt water
tank input. Resource amount and capacity come from native getters without
resource-specific conversion.

Every mode requires valid `EnablePortAtPercent`, `DisablePortAtPercent` and
`GlowMaterialSlot` values. Input requires Enable < Disable; output requires
Enable > Disable. Missing, equal, reversed or out-of-range thresholds reject
that mode. Output opens at or above Enable and closes at or below Disable;
input opens at or below Enable and closes at or above Disable. Between them,
the previous enabled state is retained.

The selected mode is stored in the owner's tags as
`SmartOutlet.Mode.v1.<socket>.<input|output|normal>`. Explicit normal is saved.
A saved mode no longer supported by the configuration falls back to normal.
Each port has its own choice; shared mode rules remain immutable.

Each controller stores its latch on the storage actor in namespaced `Tags`:
`SmartOutlet.Latch.v1.<socket>.<mode>` means open;
`SmartOutlet.LatchKnown.v1.<socket>.<mode>` distinguishes an explicit closed
state from a missing record. Missing state starts open, then normal boundary
checks apply immediately. Cleanup retains the tags. An older open-only encoding
cannot distinguish closed from missing: an unmarked midrange port starts open.
Tags are per socket/mode; switching mode retires the previous mode's markers.

`UnlockSaltWaterTank` makes the existing tank technology available at Tier 1
at runtime. Learning it remains the player's action with the stock research
cost and native recipe/component grants. Disabling the setting does not revoke
learned recipes. See the shared
[research contract](../../docs/voyage-scanning-and-recipe-unlocks-knowledge.md).

## Runtime ownership

`ModActor` subscribes to `VoyagePersistentSubsystem.OnActorAttached` and makes
one initial pass over existing `VoyageModuleActor` instances. For supported
classes it owns one transient controller per native socket view component.
Actor ticks are disabled; discovery does not run periodically.

After a next-tick delay, a controller resolves its storage module and named
socket, then restores the selected mode. Entering smart control captures the
original port type, bandwidth, automatic initialization flag and connection
direction. It restores the mode's latch, disables automatic initialization,
opens the type to `ST_Any`, requests the selected direction and applies limits.
See the shared [socket control contract](../../docs/module-socket-runtime-control-knowledge.md)
for native direction restrictions and network notifications.

Switching smart modes preserves the original bandwidth snapshot and restores
the previous indicator before capturing the new rule's material slot. Returning
to normal restores captured fields and material. Normal makes no subsequent
resource writes; re-entering smart mode captures the then-current stock state.

`OnModuleValueChanged` drives threshold evaluation. Limit changes are written
through the live `Port` reference. The comparison is captured before the write;
`BroadcastConnectionsChanged` runs only after an actual input/output limit
change. `OnModuleSocketConnectionChanged` schedules a coalesced next-tick
reconciliation. Unchanged limits do not trigger another notification.

Storage EndPlay destroys its controller. Cleanup unbinds module delegates and
restores captured socket fields, direction and material when their objects
remain valid. ModActor EndPlay removes its attachment listener and destroys
its controllers. Missing visuals do not block resource control. A negative
indicator slot disables visuals; out-of-range slots are ignored.

## Input and HUD

`WBP_PortModeAction` owns the presentation lifecycle. The manager listens to
the native detector's `OnHoverChanged` and reads `GetHoverReference`. Selection
matches the socket view component, its native collision box or its mesh by
object identity. A connected cable plug is deliberately not a socket target;
the user must aim at the storage socket. No extra collision shape is created.

The stock `IAV_InteractThree` action drives one change per press, with a held
latch reset on completion/cancel. Input and presentation are gated by the
active game HUD. The mod creates a stock button-info container, assigns its
stock card class before `SetActions`, and attaches its own hint host to
`CentralInputActionContainer`. It leaves stock cards and mappings intact.

The transparent owner widget's UI Tick refreshes selection, visibility and
attachment because the native HUD can clear/rebuild its row. Hint text is
cached to avoid recreating the card each frame. This is UI maintenance, not
resource polling. Cleanup unbinds listeners and removes only owned widgets.

## Indicator material

The controller assigns a per-socket dynamic instance of `M_PortGlow` to the
selected mode's mesh material slot (the rim in the supplied profiles). `ColorTint` is red
while disabled and green while enabled. The material is lit and opaque:
base color is `ColorTint * 0.12`,
metallic is 0.25 and roughness is 0.5. Emissive is
`ColorTint * GlowStrength * (0.525 + 0.475 * sin(2*pi*Time/period))`.
The default glow strength is 2.0; the pulse ranges from 5% to 100% of that
intensity. `GreenPulseSeconds` is 6 and `RedPulseSeconds` is 2. The material
compares the green and red channels of `ColorTint` to select the period.
Material Time follows game pause; animation needs no actor tick or timer.
This is a simple tinted surface without stock trim textures; geometry is
unchanged. Bloom and exposure affect the halo and perceived pulse contrast.

The maintained material lives in the project. The focused
[creation script](Editor/CreatePortGlowMaterial.py) refuses to overwrite it and
is not part of normal builds. The focused
[pulse update](Editor/ApplyPortGlowPulse.py) adds the pulse to the original
steady material and refuses an already modified graph. Both scripts are
one-time authoring steps; edit the saved material to tune its appearance.
See [self-contained materials](../../docs/voyage-cooked-asset-toolchain-knowledge.md#self-contained-materials-in-additive-mods)
for the shader packaging contract.

## Source and package boundaries

The Unreal project owns the assets; synchronized files in `BlueprintText/`
provide the text editing/review interface through the shared BPT workflow.
`EditorSupport/Source/Voyage/` supplies minimal editor-only reflection mirrors
for Steam build 25982825 and UE 5.8. Required stock input/widget identities are
compilation stubs; their [provenance](SOURCE-PROVENANCE.md) owns reconstruction
and revalidation boundaries.
Their test getters and
notification counter do not enter the game. `ModuleSocketIOData` is deliberately
partial: use member writes by reference, never serialize a complete native
socket or replace its full Port value from this mirror.

The public producer cooks exactly these owned packages:

- `/Game/Mods/SmartOutlet/ModActor`, DML entry class `ModActor.ModActor_C`;
- `/Game/Mods/SmartOutlet/BP_OutletController`;
- `/Game/Mods/SmartOutlet/M_PortGlow`;
- `/Game/Mods/SmartOutlet/WBP_PortModeAction`;
- `/Game/Mods/SmartOutlet/Settings/BP_SectionedIniFileParser`;
- `/Game/Mods/SmartOutlet/Settings/BP_StorageProfile`;
- `/Game/Mods/SmartOutlet/Settings/BP_OutletRule`.

The container triplet installs directly into `Voyage/Content/Paks/LogicMods`.
The package contains no replacement stock Blueprint, native DLL, game INI,
stock mesh, or stock material. `EditorBootstrap` is an editor-only BPT stub
excluded from the release inventory. Project configuration enables SM6 and
inline material shader code; it is not installed as game configuration.

The installer preserves existing sectioned INI text, including omitted modes,
comments and unknown settings. Legacy numbered port rules migrate to class/mode
profiles without adding modes; conflicting rules for one class/mode stop the
installation. Socket names are no longer configuration selectors. The original
INI and container recovery evidence are retained for rollback. `-ResetSettings`
explicitly replaces settings with the candidate's defaults.

## Validation boundary

Editor tests execute the compiled Blueprints against native mirrors: input and
output hysteresis, per-port independence, configuration and discovery, latch
snapshots, bandwidth preservation, notifications, reconciliation and visual
cleanup. They also check optional mode profiles, class lookup, mode selection,
hint text, HUD attachment/gating, input debouncing, research and restoration.
Offline checks cover settings migration and preservation. These tests do not
simulate native network transfer or the game save pipeline.

Runtime port control, pulsing glow, mode switching, the central HUD hint and
section-based settings are accepted for the user's tested setups. Attachment/restoration coverage across every placement context,
multiplayer and a complete save/reload matrix are not established by that
acceptance. The user accepted latch persistence and missing-record recovery in
his tested game saves; this does not establish every streaming/save context.
Recheck native identities and notification behavior when
the game contract changes; the producer's game pin and reviewed mappings own
build selection. Exact accepted candidate identity and the user report remain
with local release evidence, separate from the original build manifest.
Technical identity changes also require a fresh DML discovery/load check.

Run all maintained editor checks through the shared queue:

```powershell
$shell = if ($PSEdition -eq 'Core') { 'pwsh.exe' } else { 'powershell.exe' }
./tools/Invoke-UnrealQueued.ps1 -Operation 'Validate SmartOutlet' `
  -FilePath (Join-Path $PSHOME $shell) `
  -ArgumentList @('-NoProfile','-ExecutionPolicy','Bypass','-File',
    'mods/SmartOutlet/Validate-SmartOutlet.ps1')
```

The validator prepares declared editor support and runs the six suites in one
editor session. Reports and logs live under `Tmp/SmartOutlet/`; missing or failed
reports fail the command. Build/install failures remain visible in their owning
tools. Development-only `PrintString` is not a production diagnostic channel.
