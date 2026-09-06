HarpoonCannon HC04 - occupied Drone physics diagnostic, NOT the cannon mod.
Requires existing VoyageAutoLoader C8, Steam build 25056839.
One mod-authored entry package; no stock Blueprint, model or HUD overrides.

Use a DISPOSABLE save/session and start on foot on a stable deck. Do not save
manually or collect the Drone. Autosave/native persistence is not validated.
Like HC03, HC04 never requests destruction of the Drone, including when
the observer is destroyed. It remains available without a one-minute deadline.

Approach HC04 DRONE BELOW and use stock Enter (F in the validated HC03 test).
After entry, the observer requests root-body physics OFF exactly once, records
the original simulation state and measures world displacement from that point.
Try mouse look and briefly WASD/ascend. Report whether the view turns and whether
the Drone translates; send a screenshot of the board. Then use stock Exit
(E in HC03 HUD) and confirm normal character control. Screenshot after exit.
Exit restores the captured simulation state, not an assumed true default.
This is NOT attachment to a ship: keep the ship stopped and test only briefly.
World-relative drift on a moving ship is expected and is not this experiment.
If control or HUD breaks, close the game; do not save or continue experimenting.

The observer shows whether GetPlayerPawn equals the owned Drone and remembers
that comparison becoming true, then the original pawn being restored. These
are possession observations, not proof of complete native vehicle cleanup.
No DONE/deletion timer exists. Whole-Actor hidden/collision flags do not prove
all mesh/interaction behavior. Apart from the reversible root simulation change,
the probe does not force entry/exit, initialize items, recharge, activate,
attach or change input/camera/HUD. Native stock code
handles the actions you press.

If the root is not a PrimitiveComponent, the board reports FAILED and no
physics mutation is made. If physics becomes YES again while occupied, that
is evidence of native re-enable; the observer does not keep forcing it OFF.
Observer teardown/save/travel while occupied is unvalidated; always exit first.

The cannon is not required in the save. When testing ends, exit the Drone and
close the disposable session without manually saving. Remove/restore the probe
using its installation manifest with the game closed, not by copying files.
