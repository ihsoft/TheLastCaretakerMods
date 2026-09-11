# Voyage mapping refresh: Steam 25191271

## Result

Registered `mappings/Voyage/steam-25191271-ue5.8/Voyage-25191271.usmap`
with an immutable sibling provenance manifest. SHA-256:
`E25620FB2D0674B26A3E4EF059AC2C5AF58E28CE1B4F07307860F7FADB9A0F68`;
2,376,216 bytes. Previous build registry remains intact. No mod installation
or game-memory write was performed.

## Tool use and validation

- Intent: regenerate mappings after a confirmed executable/build change,
  validate parser use and make the mapping available through the normal resolver.
- `New-VoyageMappings.ps1`: initial Windows PowerShell 5.1 launch failed at
  `ProcessStartInfo.ArgumentList`; PowerShell 7 with a loaded save reached jmap
  but failed with an unexpected null pointer. Source inspection was triggered
  by these failures, confined to launch and error propagation.
- Same documented generator at the main menu succeeded with the unchanged
  reviewed jmap `4f88d8af758712839529f9eeeb02b82c9469e271`. Menu recovery supports
  a session-state sensitivity, but does not prove the exact failing object.
- `Test-VoyageMappings.ps1`, called by the generator and resolver, passed header,
  payload, fingerprint, file hash/length and required reflected-name gates.
- `Get-VoyageAssetJson.ps1` parsed current stock BP_ModuleCable_Mooring and
  BP_MooringCable_Socket_Male with explicit new mapping provenance, without
  pseudocode failures. No broad changed-save or runtime claim is made.
- Registry promotion used an exact copy and authored portable manifest;
  `Get-VoyageMappings.ps1` selected and verified the new registry entry.
- PowerShell compatibility fix uses EnvironmentVariables and quoted Arguments
  instead of ArgumentList, plus the supported Kill overload. PS5.1 AST/launch
  smoke exercised real jmap with an invalid PID and output path containing
  spaces; native error propagated correctly, with no argument parsing failure.

Expected return of the small launch fix: remove the deterministic first-call
failure and host-switch retry for every ordinary Windows PowerShell refresh.
It required no dependency change or dumper rebuild. The full successful capture
used PowerShell 7; the compatibility fix was subsequently tested separately,
without repeating the costly full capture just for a shell change.

Ignored evidence: `artifacts/mappings/steam-25191271-747DC2553F7E-jmap-4f88d8a-20260911T032148Z/`
and `artifacts/mappings/test-ps51-launch.ps1` with `ps51 launch smoke/` logs.
Uncovered operation: registry promotion still requires exact copy plus manifest
authoring; one occurrence does not justify a new tool. No other reusable gap
was introduced. Canonical dumper internals were not modified.
