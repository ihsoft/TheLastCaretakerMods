HarpoonCannon HC02 - standalone interaction-observation diagnostic, NOT the cannon mod.
Requires the existing VoyageAutoLoader C8 protocol, Steam build 25056839.
Adds only a new entry actor package; no stock Blueprint or HUD overrides.

Load gameplay on a stable deck. Do not save manually or enter/collect the test
Drone during this disposable test. Normal autosave/streaming behavior for this
diagnostic has not been validated; use a disposable save/session.

A cyan world-space board should appear near the upper-left view. It reports
initial spawn validity, independently sampled validity, seconds and distance.
The observation window is sixty seconds. A separate HC02 DRONE BELOW label
marks the spawned actor's position. Approach and point the crosshair at the
Drone; take a screenshot showing any normal Enter/Loot/No Power hint (or its
absence). DO NOT press Enter/Loot: the probe will request removal at the end.
Actor hidden/collision diagnostics are whole-actor flags, not mesh visibility
or proof that an interaction trace hits it.
At the end it says HC02 DONE, preserving the last sample, and requests removal
of only the Drone created by this probe. Send that screenshot too if possible.
If no board appears, or it stays WAITING/OBSERVING, report that instead.

The existing cannon/model is NOT installed or changed. The test Drone receives
no activation, attachment, possession or input changes; this tests standalone
creation/lifetime only. It does not claim working cannon entry or optics.
The observer does not create widgets, change camera/HUD/control or write saves.
Its reference-scoped cleanup never searches for/deletes other Drone instances.

Install/remove through the accompanying release/installation manifest tools.
Never copy or replace mod files while Voyage is running.
