# Smart Outlet editor identities

The editor identities were reconstructed from Voyage Steam build 25982825,
UE 5.8, through reviewed mappings, asset summaries and native reflection
inspection. The native mirrors under `Project/EditorSupport/Source/Voyage`
describe only fields, functions and
delegates used by compiled Blueprint bytecode and editor fixtures. They are not
native ABI replacements and no native module ships. Revalidate affected owners,
signatures and enum values when the game contract changes or current inspection
disagrees. The producer's game pin and mapping resolver own build selection.

The following project assets are identity-only editor stubs, created through
Unreal factories rather than copied from cooked game content:

| Virtual package | Editor identity |
| --- | --- |
| `/Game/Game/Input/Character/IAV_InteractThree` | Boolean InputAction; stock third interaction action. |
| `/Game/Game/Input/Character/DA_Input_Context_Character` | VoyageInputContextAsset. |
| `/Game/UI/Game/Inputs/BP_ButtonInfoDetailed_Vertical` | Widget Blueprint with VoyageButtonInfoWidget parent. |
| `/Game/UI/Game/Inputs/BP_ButtonInfo_Vertical_Container` | Widget Blueprint with VoyageButtonInfoContainerWidget parent. |

These saved project assets are compilation dependencies, excluded from the
producer's explicit package allowlist. The game supplies their full
runtime content. The widget parents come from stock generated-class metadata;
the button-info container's class property and SetActions signature come from
native reflection. UI selection uses the native detector and character HUD
contracts described in [Architecture](ARCHITECTURE.md#input-and-hud).

`EditorBootstrap` is an editor-only Blueprint workspace stub and is also
excluded. All release assets under `/Game/Mods/SmartOutlet` are mod-authored;
the sectioned parser is a relocated copy of the shared BPT library. Builds cook
maintained assets, never regenerate their graphs. Changed input/HUD/resource
contracts need their corresponding real-game validation; editor fixtures alone
cannot establish compatibility.
