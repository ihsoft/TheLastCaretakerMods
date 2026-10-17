# ScopeFix compatibility backlog

## Restart

- Current installed game: Steam build `25902891`.
  The two newest crash contexts report engine `5.8.2`.
- The builder remains gated to `25880952`; no new build is required. The user
  installed the correct archive on `25902891` and reported that it works.
- `Get-VoyageMappings.ps1` currently reports no reviewed mapping for the new
  fingerprint. Do not generate mappings merely because the fingerprint changed.
- The recursive Paks inventory taken during crash triage found ScopeFix ZIP
  archive files but no active ScopeFix container triplet. The user suspects he unpacked the wrong
  version; the exact archive used before the crashes is unknown. After installing
  from the correct archive, he reported that the game works. Do not attribute
  the crashes to the `25880952` candidate.
- Next action: none unless the user reports an aiming problem or requests a new
  build. The successful report establishes startup with the correct archive on
  `25902891`; it does not separately confirm the aiming behavior. If a future
  build is needed, first obtain reviewed mappings and fresh stock assets for the
  then-current fingerprint.

## Startup crash observation

The user reported a crash before the main menu. The two newest crash contexts
are:

- `UECC-Windows-20CD08EE4554DB9AD14A429F68933C03_0000`
- `UECC-Windows-F7003DC84AD92E78EA54AB99607C05FF_0000`

Both report an `ObjectSerializationError` while loading
`DA_Weapon_ElectricRifle`: `VoyageItemHandEquipment` encountered bad name index
`511/16`. This identifies a serialized-package failure involving the rifle
asset, but it does not identify which archive supplied that package. The lack
of a ScopeFix triplet in the read-only inventory taken before the user
reinstalled, together with his wrong-version suspicion, leaves causality
unresolved. The subsequent successful launch with the correct archive supports
the user's wrong-archive explanation, but the original archive remains unknown.

## Candidate built for 25880952

The `v1-steam-25880952-test` candidate targeted Steam build `25880952`.
Its fresh stock JSON hashes were:

- electric rifle:
  `4C267A2BD8F63782D5CCBD03E8962EDB33FA59F98E7F8DB911DAEF4BBB3FFC3E`;
- RedDot:
  `D87541EA6D7C0693C35CF8EEE61023F9F6B8328D50E71AEDDF981775C0391AB1`.

Static validation passed the exact two-package set, semantic reopen of both
modified vectors, whole-JSON equality after restoring each X, byte-identical
`.uasset` files and `.uexp` differences confined to each eight-byte X field.
The schema-2 release manifest SHA256 was
`83C3D61556C32C86F0455284E8A5C0F367CD61CA4B143366D6EB5325D71C19F4`;
the release ZIP SHA256 was
`C138C526AFC62C520F6DD521FAB1CD5FFC4FBD3F8B306C129CA5A40CD76A50B8`.
The user later reported that the correct archive works on Steam build
`25902891`. This establishes successful startup on that build; it does not
separately confirm the aiming behavior or turn the older stock inputs into fresh
`25902891` validation.
