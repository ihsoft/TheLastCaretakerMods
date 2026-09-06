# ScopeFix current-game candidate

Status: container-verified, installed `25056839-test1` on 2026-09-06 UTC;
user accepted the real-game aiming compromise described below.

The downloaded `ScopeFix_P Pak Form-57-V1-0-1763799042.zip` contains two
overrides: `/Game/Data/Assets/Weapon/DA_Weapon_ElectricRifle` and
`/Game/Data/Assets/Attachments/DA_Weapon_Attachment_Sight_RedDot`.
Recovered ADSCameraOffset vectors are `(-2.74, 0, 13.5)` and
`(1.13, 0, 0.275)`. Current stock vectors are `(-2, 0, 13.5)` and
`(1, 0, 0.275)`. This candidate transfers only the X values to fresh stock
assets; it does not transfer old item metadata or weapon logic.

Provenance: Steam build `25056839`, UE 5.8.1, executable SHA256
`CA84428CF4562C703BEDFF053DB727D14CC70C593451C09BE75A92828EFD9933`.
The values above were reconstructed with canonical retoc UE5_7 extraction,
CUE4Parse partial property decoding and exact vector bytes in both original
export payloads. The original mod's game fingerprint and stock baseline are
unknown. Thus this is a recovered configuration, not a complete historical
stock-versus-mod diff. Revalidate on any game fingerprint change.

Old-package pitfall: its directory index has no filenames. retoc recovered
both package identities, but absent old dependency packages produced unknown
imports. UAssetAPI JSON returned RawExport with both available mappings.
CUE4Parse decoded the early ADS vectors but logged later property errors;
do not treat its remaining old JSON as a fully validated schema. A diagnostic
mixed installed-container mount was used only for this recovery, never as
stock provenance. Old extraction is retained under ignored
`artifacts/scope-fix/legacy` and need not be repeated.

Build with Windows PowerShell 5.1:

```powershell
powershell.exe -NoProfile -ExecutionPolicy Bypass -File mods/ScopeFix/Build-ScopeFix.ps1 -OutputRoot artifacts/scope-fix/build-new
```

Requires canonical published retoc, Inspector and reviewed mappings. Output
must be new. The builder checks the fingerprint and semantic stock vectors,
extracts stock packages through the public tools, locates each unique double
vector, changes only its X bytes, packages UE5_8 and verifies the container.
This byte patch preserves every other export byte and the current headers.
It does not install or produce the common installer's schema-2 release manifest.

Current evidence: `artifacts/scope-fix/build/build-evidence.json` and
`artifacts/scope-fix/validation.json`. Both patched assets reopen with zero
RawExports and the intended semantic vectors; re-extraction of the produced
container preserves both export payload hashes. Test ZIP:
`artifacts/scope-fix/ScopeFix_25056839-test1.zip`, SHA256
`9233DFD8F0A247FD9E56BBE206F49E58D2C46B72A82055FB69B5830B67864601`.

User game-test result for installed `25056839-test1`: subjectively, aiming is
corrected at long distances; at short distances shots now land below the aim
point. The user explicitly accepted this compromise because accurate aiming
matters more at long range, and requested no further refinement. The weapon's
animation makes a reliable precision assessment difficult. Treat this as
qualitative user acceptance, not measured accuracy across distances.
The user did not distinguish attachment configurations or report testing
RedDot on other weapons; do not infer those results. A full historical mod
diff and the native camera axis convention remain unestablished.
Keep the installed candidate unchanged; no further aiming work is pending.

Installation: the existing verified ZIP was adopted into schema-2
`artifacts/scope-fix/release-manifest.json`, explicitly declaring dirty source
and recording the builder hash. `Install-VoyageRelease.ps1 -AllowDirtySource`
passed ValidateOnly, then installed the triplet and versioned ZIP with game
closed and all four installed hashes matching. Evidence and restore entry:
`artifacts/installations/ScopeFix/20260906-022853-25056839-test1-e7f26172/install-manifest.json`.
Use `Restore-VoyageReleaseInstallation.ps1 -InstallManifest <that path>` for
an authorized rollback. No new extraction or rebuild was needed for this
installation. Installation tool-use report: manifest packaging prepared from
the verified candidate; public installer validation and installation passed;
no tool failures, runtime test was pending at installation and is now accepted
with the limitations above. Manifest preparation was manual for this initial
candidate. Future release preparation uses `tools/New-VoyageReleaseManifest.ps1`
with exact ScopeFix SourcePath scope; do not reproduce manual schema-2 JSON.

Tool report: fingerprint/mappings, stock inventory/JSON, extraction,
retoc packaging/verify and payload roundtrip succeeded. Legacy full parsing
failed as described above. Fallbacks: canonical retoc CLI for the unnamed old
container; GUI Program.cs to establish CLI syntax after --help started a GUI;
partial CUE4Parse plus raw vector confirmation for legacy schema mismatch.
The Inspector does not index loose legacy inputs on the attempted path.
The unrelated GUI started with --help could not be identified through CIM
(access denied); no existing user GUI was terminated. Installation-status
source was unnecessarily opened once; its public command later succeeded.
Historical full-iteration coverage R9: 7/9 recurring families, including
installation; initial modification and manual release packaging were uncovered.
See `docs/voyage-toolchain-coverage.md` for the authoritative sample. Later
pipeline improvements do not rewrite R9. Legacy diagnosis is excluded.
Reusable gap: historical containers without index paths/complete
matching dependencies have no intent-level validated property-recovery path.
