# DonkLiftKeyboardControl architecture

This loader-free mod replaces the stock forklift package with a generated
child of a freshly relocated stock original. An owned helper attached to each
forklift integrates the native digital throttle and steering commands into
gradual control values, keeping the native movement and percentage display.
It also supplies X/C reset actions through the game's standard provided-action
HUD path. The shipped payload is one IoStore container; editor mirrors and
generator binaries are build inputs, not runtime dependencies.

The control, HUD, package, and validation contracts are in [mod rules](MOD-RULES.md).
Version-bound game identities are in [GAME_DERIVED_SOURCES.md](GAME_DERIVED_SOURCES.md),
while [README.md](README.md) owns the public build and installation interface.
