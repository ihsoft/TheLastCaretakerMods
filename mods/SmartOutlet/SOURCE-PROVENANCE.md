# Smart Outlet editor identities

The maintained project targets Voyage Steam build 25982825, UE 5.8. The native
mirrors under `EditorSupport/Source/Voyage` describe only fields, functions and
delegates used by compiled Blueprint bytecode and editor fixtures. They are not
native ABI replacements and no native module ships. Recheck reflected owners,
signatures and enum values when changing the supported game build.

The following project assets are identity-only editor stubs, created through
Unreal factories rather than copied from cooked game content:

| Virtual package | Editor identity |
| --- | --- |
| `/Game/Game/Input/Character/IAV_InteractThree` | Boolean InputAction; stock third interaction action. |
| `/Game/Game/Input/Character/DA_Input_Context_Character` | VoyageInputContextAsset. |
| `/Game/UI/Game/Inputs/BP_ButtonInfoDetailed_Vertical` | Widget Blueprint with VoyageButtonInfoWidget parent. |
| `/Game/UI/Game/Inputs/BP_ButtonInfo_Vertical_Container` | Widget Blueprint with VoyageButtonInfoContainerWidget parent. |

[CreateRuntimeHintResources.py](Editor/CreateRuntimeHintResources.py) creates
missing identities only. These are compilation dependencies, excluded from the
producer's explicit seven-package inventory. The game supplies their full
runtime content. The widget parents come from stock generated-class metadata;
the button-info container's class property and SetActions signature come from
native reflection. UI selection uses the native detector and character HUD
contracts described in [Architecture](ARCHITECTURE.md#input-and-hud).

`EditorBootstrap` is an editor-only Blueprint workspace stub and is also
excluded. All release assets under `/Game/Mods/SmartOutlet` are mod-authored;
the sectioned parser is a relocated copy of the shared BPT library. Builds cook
maintained assets, never regenerate their graphs. A new game build requires
reviewing these identities and real-game input/HUD/resource validation before
claiming compatibility; editor fixtures alone cannot establish it.
