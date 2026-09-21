# The Last Caretaker: gameplay observations

This document contains compact, reproducible gameplay values that are useful
for mod balancing. It is not an experiment log.

## Provenance

The values below were re-read from Steam build `25191271`, executable SHA-256
`747DC2553F7E68D8EA7ED0B2E0CAC6D08943EA3F50DD6ED822E9293E0B45F58B`.
They are version-bound and require revalidation after the executable
fingerprint changes.

## Electricity units

- Voyage module electricity amounts behave as kilojoules. Module power demand
  is expressed in watts, so charging `E` kJ over `t` seconds requires
  `E * 1000 / t` watts, excluding any standby demand.
- The stock HUD convention divides the stored amount by `1000` and labels the
  result `kWh`. That presentation must not be used as an SI conversion: one
  physical kWh is `3600` kJ.
- Railgun therefore correctly treats `FullChargeEnergyKJ=500` as `500` native
  electricity units and requests `500 * 1000 / FullChargeTimeSeconds` watts.
  The user validated that `500` kJ with a `5.5` second setting takes about
  `5.5` seconds in the game.

## Electric Rifle

The stock Electric Rifle has a `250` kJ electricity capacity. A fully held
shot consumes ten charge steps of `25` kJ, or `250` kJ total when the rifle is
not connected to an electric cable.

Its base damage is `10`. The full-charge modifier is `5`, producing `50` raw
damage. While an electric cable is connected, the weapon applies a `1.5`
damage multiplier and a `2` consumption multiplier:

| State | Full-charge energy | Full-charge raw damage |
| --- | ---: | ---: |
| Disconnected | 250 kJ | 50 |
| Connected | 500 kJ | 75 |

These are raw attack values before target-specific resistance.

## Hostile health

| Display name | Internal identity | Maximum health |
| --- | --- | ---: |
| Talon Shark | `DA_NPC_Shark` | 600 HP |
| Ram Shark | `DA_NPC_RamShark` | 250 HP |
| Talon Shark - Dorsal Model | `DA_NPC_LaserShark` | 600 HP |
| Angel | `DA_NPC_Squid` / `BP_NPC_Angel` | 300 HP |
| Arch Angel | `DA_NPC_ArchAngel` | 400 HP |

The current package inventory contains two live Angel actor classes:
`BP_NPC_Angel` and `BP_NPC_ArchAngel`. `Angel_Rusty` exists as a loot-table
identity, not as a separate combat actor with independent health.

All three sharks and the ordinary Angel declare electric resistance `-0.05`.
Arch Angel has no electric resistance entry in its actor template.

Railgun currently submits a directional `PhysicalForce` attack with zero
damage variance. The shark data declares no `PhysicalForce` resistance, so a
direct Railgun hit of `600` removes all `600` HP from a full-health Talon or
Dorsal Shark, provided the target is not temporarily invulnerable.
