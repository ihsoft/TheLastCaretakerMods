HarpoonCannon HC03 - stock Drone enter/exit diagnostic, NOT the cannon mod.
Requires existing VoyageAutoLoader C8, Steam build 25056839.
One mod-authored entry package; no stock Blueprint, model or HUD overrides.

Use a DISPOSABLE save/session and start on foot on a stable deck. Do not save
manually or collect the Drone. Autosave/native persistence is not validated.
Unlike HC02, HC03 never requests destruction of the Drone, including when
the observer is destroyed. It remains available without a one-minute deadline.

Approach HC03 DRONE BELOW and use the stock Enter action (F in the prior test).
Use the stock exit action shown by the Drone HUD. Do not assume an exit key
from this README. Stay close; no long flight, grab/loot or charging experiment.
Report whether you saw through the Drone and could return to the character.
Send a screenshot from inside and after exit, or report any failed entry/hint.
If control or HUD breaks, close the game; do not save or continue experimenting.

The observer shows whether GetPlayerPawn equals the owned Drone and remembers
that comparison becoming true, then the original pawn being restored. These
are possession observations, not proof of complete native vehicle cleanup.
No DONE/deletion timer exists. Whole-Actor hidden/collision flags do not prove
all mesh/interaction behavior. The probe does not force entry/exit, initialize
items, recharge, activate, attach or change input/camera/HUD. Native stock code
handles the actions you press.

The cannon is not required in the save. When testing ends, exit the Drone and
close the disposable session without manually saving. Remove/restore the probe
using its installation manifest with the game closed, not by copying files.
