Breech sight and file settings test, NOT a release.
Aim at the cannon base. If Enter Harpoon is labeled, enter once and exit with E.
HarpoonEntryInteractBlocks records the effective response after spawn (read only).
If the hint is empty/absent, do not press it; take a screenshot and stop the test.
Requires paired current-build Harpoon shell and mod autoload support.
Wait a few seconds, approach the built cannon base and aim at it.
Enter Harpoon should appear on the ordinary native interaction hint. Press its key.
Expected: stationary first-person x5 optics, own reticle and native Exit Harpoon key card,
game item name (or technical actor name if unavailable) and rounded sight-to-hit
distance for ANY solid target. Try two targets, then empty sky: both fields clear.
Defaults: MousePercent=40, YawLimitDegrees=80, pitch -50/+10.
Optional ProjectSavedDir/Config/HarpoonCannon.ini is read on every entry.
Keys: MousePercent, YawLimitDegrees, MinimumPitchDegrees, MaximumPitchDegrees.
Use key=value with decimal point. Missing/invalid/unknown lines are ignored;
last valid duplicate wins. Values clamp to mouse1..100, yaw1..170,
minimum pitch-89..0, maximum pitch0..89. Keep this local text file small.
Camera follows the model sight above the breech; distance starts at the camera.
Native entry and E exit unchanged.
F8 and 20-second station timeout are EXIT fallbacks; F8 does not enter.
After exit check ordinary walking and actions. Do not save while testing.
Do not dismantle, loot, drive or enter another vehicle while inside.
No firing in this checkpoint. Check native Exit Harpoon key card bottom-left.
Marker reports prepared station count and sticky ANY entry provider/callback.
These flags aggregate all prepared stations, not the currently aimed gun.
If entry is absent, take one screenshot: do not try F8 preparation/entry.
Occupied HUD marker separates widget READY from action provider OBSERVED. If absent, take
one screenshot then E exit; do not repeat blind tests. No Forklift inheritance/HUD.
After result close game and restore probe then paired shell by install manifests.
RMB action toggles x1 / x5 while controlling the cannon. Entry resets to x1.
x1 uses captured on-foot FOV and100% mouse, no reticle/target data/mask.
x5 uses angular magnification and35% mouse by default, with circular soft-edge
mask, reticle and hit data. OpticsMousePercent config overrides the35% setting;
legacy MousePercent is ignored. Sight position/yaw limits/native E exit unchanged.
F9 diagnostic was removed. No saving; safety timeout remains20 seconds.
