# Aiming configuration

## Why adjust the camera

The problem being addressed is the alignment between the displayed aiming
reference and observed impacts. `ADSCameraOffset` controls the camera offset
used for aiming. Adjusting it changes the view relative to the weapon/sight.
The implementation changes these offsets only; projectile behavior, damage
and weapon animations retain their stock data.

The electric rifle and RedDot have separate offset settings, so the mod
configures both owners. RedDot is a shared attachment asset: its adjustment
is not restricted to the electric rifle. This is not a fix for every sight.

The configured values improve long-range alignment while close-range impacts
can fall below the aim point. Alignment can depend on distance when the aiming
reference and firing path are spatially separated, but the exact camera/weapon
geometry and the cause of this game's remaining offset have not been
established. Do not present that general explanation as a verified diagnosis.

The configuration prioritizes distant shots and accepts close-range undershoot.
There is therefore no requirement to chase a single apparent alignment at
all distances. These values define the mod configuration, not the result of
an exact calibration or a universally correct geometric formula.

## Property contract

| Package below `/Game/` | Property | Stock | ScopeFix |
| --- | --- | --- | --- |
| `Data/Assets/Weapon/DA_Weapon_ElectricRifle` | `ADSCameraOffset` | `(-2, 0, 13.5)` | `(-2.74, 0, 13.5)` |
| `Data/Assets/Attachments/DA_Weapon_Attachment_Sight_RedDot` | `ADSCameraOffset` | `(1, 0, 0.275)` | `(1.13, 0, 0.275)` |

Only X changes. The builder validates each stock vector semantically, locates
its unique serialized double-vector representation and replaces the X bytes.
Current headers and all other export bytes are preserved. X's exact physical
axis in the native aiming implementation has not been verified; it should not
be described simply as world-space height.

## Build and compatibility boundary

The builder is gated to Steam build `25880952`. It extracts both stock assets,
checks their camera vectors, changes only the two intended X values and
verifies the resulting package structure. A release built for that version
also starts on Steam build `25902891`; aiming behavior on that later build has
no separate compatibility claim.

A build for another Steam build needs reviewed mappings and fresh stock assets
for that build, followed by the relevant structural and gameplay checks.
Generated assets remain ignored output; the repository keeps the method.
