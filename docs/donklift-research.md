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
- Writing smoothed values to Blueprint properties `Acceleration` and `Steering`
  produces the desired persistent physical control.
- The visible HUD continues to show the original digital values.
- Direct `USlider:SetValue`, direct `Value` writes, and clearing
  `ValueDelegate` did not change the visible indicators.
- Any Lua path that marshals a new `FText` crashes this UE4SS Compact build with
  `FText(FString&&) not found. FText::construct_with_string failed.`

## Research rule

Do not make further HUD changes from widget names alone. First extract and
decompile the HUD Blueprint and trace the exact data flow that renders the two
visible indicators.

