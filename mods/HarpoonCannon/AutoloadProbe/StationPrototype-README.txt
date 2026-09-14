HarpoonCannon operator/optics checkpoint

Requires the paired Harpoon shell and existing mod autoload support.
Aim at the built cannon base and use Enter Harpoon.
Entry defaults to 1x: character-eye position, hollow center circle, normal mouse.
RMB / Toggle scope switches 1x and 5x. In 5x the barrel sight, optical mask,
crosshair and first-hit name/range are shown. A miss clears target data.
E exits; F8 and the 20-second timeout are exit safeguards, not entry methods.

Runtime tuning: ProjectSavedDir/Config/HarpoonCannon.ini, read on each entry.
OpticsMousePercent=35
YawLimitDegrees=80
MinimumPitchDegrees=-50
MaximumPitchDegrees=10
Normal mouse scalar is1.28; optics35% is0.448. Legacy MousePercent is ignored.
Use key=value with decimal points, no inline comments. Unknown/invalid lines
are ignored; last valid duplicate wins. Exit before editing and re-enter.
Settings reload has a separate validation boundary from default gameplay.

No diagnostic overlays. No firing or ammunition-state support yet.
Do not save, dismantle, loot or enter another vehicle while occupying the station.
After exit ordinary character actions should resume.
Install/remove only through public manifest tools with the game closed:
install shell then operator; restore operator then shell. Preserve preferences.
