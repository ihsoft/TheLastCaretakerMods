# DonkLift-320 keyboard control research

## Environment

- Game install: `P:\SteamLibrary\steamapps\common\Voyage`
- Game executable: `VoyageSteam-Win64-Shipping.exe`
- Engine: Unreal Engine 5.7
- Mod loader: UE4SS Compact 3.0.1 beta

## Verified runtime objects

- Forklift actor class:
  `/Game/Blueprints/Vehicles/BP_Forklift_Possesable.BP_Forklift_Possesable_C`
- Native base input functions:
  - `/Script/Voyage.VoyageVehicleForkliftPawn:GetThrottleInput`
  - `/Script/Voyage.VoyageVehicleForkliftPawn:GetSteeringInput`
- Forklift HUD class:
  `/Game/UI/Game/HUD/BP_VoyageIngameForklift.BP_VoyageIngameForklift_C`
- Candidate live widgets:
  - `ThrottleSlider`
  - `SteeringSlider`
  - `ThrottleTB`
  - `SteeringTB`

## Verified behavior

- Keyboard axes are digital: throttle `-1/0/1`, steering `-1/0/1`.
- Pre-hooks on `GetThrottleInput` and `GetSteeringInput` are the reliable input
  interception points. The production control mod integrates its own values on
  a fixed clock and writes them before the native getters read the fields.
- Mod-produced active values are limited to `+/-0.9999`; exact `-1/0/1` remain
  unambiguous game commands. The active neutral sentinel is `0.0001`, while
  exact zero is reserved for release and vehicle exit.
- The same getter path drives both vehicle behavior and the native percentage
  indicators. No separate slider writes are required.
- `X` resets throttle and `C` resets steering immediately. Throttle resets on
  exit; visually turned wheels may remain turned until the next acquisition.

## Verified hotkey HUD

- Visible lower-left vehicle hints live under
  `BP_DynamicPlayerInputHorizontalWidget_Bottom.ContextInputActionsRoot`.
- `KeybindRoot` is not the visible `E/H` host and is empty at runtime.
- The HUD section of `DonkLiftKeyboardControl` creates two native
  `WBP_InteractIndicator_C` copies and configures them after their widget trees
  become available. It runs independently from the 50 ms control loop and
  never writes vehicle input.
- The custom `FText_Constructor.lua` resolves `FText(FString&&)` to a unique
  match in the validated Voyage executable. Without it, marshaling a new
  `FText` fails and may crash. Revalidate the signature after game updates.
- Voyage's active menu language comes from
  `VoyageGameUserSettings.CustomSettings.LanguageType`, not from
  `KismetInternationalizationLibrary.GetCurrentLanguage`.
- English and Russian labels are validated. Language is read once per forklift
  HUD lifecycle, allowing a menu language change to take effect after the next
  game load without continuous polling.

## Research rule

Ship physical control and HUD hints as one mod, but keep their code paths and
state isolated. Revalidate both the runtime widget hierarchy and the
version-pinned `FText` signature after a game update before changing or
redistributing the mod.
