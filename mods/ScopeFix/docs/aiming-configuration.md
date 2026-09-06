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

The accepted values improve long-range alignment according to the user's
in-game observation. The same test found close-range impacts below the aim
point. Alignment can depend on distance when the aiming reference and firing
path are spatially separated, but the exact camera/weapon geometry and the
cause of this game's remaining offset have not been established. Do not
present that general explanation as a verified diagnosis.

The user prioritizes distant shots and accepts the close-range undershoot.
There is therefore no requirement to chase a single apparent alignment at
all distances. These values are an accepted configuration, not the result of
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

## Version-bound evidence

- Steam build: `25056839`; Unreal Engine: `5.8.1`, profile `UE5_8`.
- Executable SHA256:
  `CA84428CF4562C703BEDFF053DB727D14CC70C593451C09BE75A92828EFD9933`.
- Accepted feature checkpoint: `604fe6fdf3b202623c3126efb9c8f57de9617a6d`.

Method: fresh stock JSON and package extraction through
`Get-VoyageAssetJson.ps1` and `Extract-VoyagePackage.ps1`, guarded byte patch,
retoc packaging/verification, semantic reopen and export-payload roundtrip.
Both modified assets reopened with zero RawExports and the expected values;
re-extraction preserved both export payload hashes. The user accepted the
long-range/close-range tradeoff in the real game. Separate attachment
configurations and other weapons were not reported as tested.

Revalidate packages, property contracts and game behavior on any changed
fingerprint before building for that game version. Generated assets stay in
ignored `artifacts/` directories; the repository keeps the method.

The accepted installed artifact retains its immutable name `25056839-test1`.
The suffix records its original packaging identity, not pending user approval.

- ZIP: `artifacts/scope-fix/ScopeFix_25056839-test1.zip`.
- SHA256: `9233DFD8F0A247FD9E56BBE206F49E58D2C46B72A82055FB69B5830B67864601`.
- Build report: `artifacts/scope-fix/build/build-evidence.json`.
- Semantic checks: `artifacts/scope-fix/validation.json`.
- Release manifest: `artifacts/scope-fix/release-manifest.json`.
- Installation/rollback manifest:
  `artifacts/installations/ScopeFix/20260906-022853-25056839-test1-e7f26172/install-manifest.json`.

All artifact paths above are relative to the repository root. Installation
checked that the game was closed and read back all four installed hashes.
