# Mooring cable length: bounded research

## Current state and next action

**Final release: 1.0.0, user accepted manual deployment and towing.** User chose to retain
automatic payout with the original 20 m ceiling while allowing manual 60 m
deployment. Build/independent checks passed in
`artifacts/releases/MooringCable60m-60m-test5-r2/`. Installed with game closed;
four file hashes matched. Installation/rollback manifest:
`artifacts/installations/MooringCable60m/20260911-044057-60m-test5-f724172e/install-manifest.json`.
User loaded the save; all three connected cables read MaxLength=2000 and
MaxLengthBase=6000, zero read errors. Evidence:
`artifacts/mooring60/live-test5-loaded-a.json`. Native inheritance and refresh
callback execute in the loaded game. The user then physically deployed a free
end to 59 m. `live-test5-transition-a.json` independently captured that cable
with one socket, MaxLength=6000 and currentLength=5732.5 cm, while all connected
cables retained MaxLength=2000. Manual deployment and switching are game-validated.
The user subsequently reported less payout during towing, accepted the remaining
movement and requested the final version. Final 1.0.0 promotes the exact tested
triplet into `artifacts/releases/MooringCable60m-1.0.0/`; no further physics
changes or recook. The 20 m value is the stock automatic payout parameter, not
a hard geometric distance under load. Existing-save loading passed; a new-save/
reload roundtrip was not separately reported and is not claimed as validated.
No further gameplay changes are pending. The final promotion report and common
release/install manifests own artifact identity, byte equality and rollback.

Test5 preserves freshly extracted stock mooring as BP_ModuleCable_Orig060,
with only MaxLengthBase=6000, and installs a generated child at the original
package/class identity. On ReceiveBeginPlay the child starts a 0.05 s looping
object-bound timer. Its refresh sets native MaxLength to 2000 if both cached
sockets are valid, else 6000. The male socket still supplies 6000. It does not
change strength, tightness, snap settings or other cable types. This keeps the
original class identity for saved cables; the runtime checks above passed.
The minimal editor mirror preserves ModulesConnectCable -> UtilityCable ->
Actor and ModuleSocketComponent -> BoxComponent, established by current mapping
schema inspection. Only the generated child is cooked with tagged properties;
this avoids inherited-property index drift from the minimal mirror. Both stock
packages retain their original property serialization. Child CDO readback has
only UberGraphFrame and the inherited named RootComponent; its SCS has no active
root nodes. No editor placeholder, native DLL or autoload actor is shipped.

Tool report: current fingerprint, mappings resolver, asset JSON, compact
Blueprint summary and schema inspection succeeded. Native member reader was
extended for the current exact hash and three zero-error live reads proved
payout. Graph producer required two local fixes (include path, explicit self
targets); an initial package gate caught an overly broad inactive-root check
and unversioned CDO index drift. Tagged child cook resolved the latter; native
semantics were not relaxed. Stock unchanged roundtrips, exact three-package
container verification, independent stock/relocated-parent semantic comparison,
child superclass/event/root/CDO checks and install hash readback passed.
Producer source reads were necessary for new mod logic and those local failures;
no common pipeline internals were changed. Mechanical work uses documented
tools plus the new owning Build-LimitGraph.ps1; no uncovered reusable operation
requires a new cross-cutting tool. Purely mechanical coverage exceeds 80%.
Detailed ignored build/cook evidence: `artifacts/mooring60/limitgraph-test5-r3/`
(compiled graph) and `limitgraph-test5-r4/` (tagged cook).

Finalization tool report: promoted the accepted containers without recooking;
the final source cleanup compiled successfully in `final-source-build-r2.log`.
PowerShell parsing, Test-VoyageContainer exact three-package verification,
New-VoyageReleaseManifest validation and common installation/hash readback all
passed. Source checkpoint: `4c8c99826fa72cccf2dd0e23e8c620fb057fda82`.
Final installation with the game closed:
`artifacts/installations/MooringCable60m/20260911-051113-1.0.0-2e0c5bb9/install-manifest.json`.
Independent Get-VoyageInstallationStatus readback: four matching files, zero
mismatches, matching game fingerprint. The archive and all three containers
match test5 bytes; `artifacts/releases/MooringCable60m-1.0.0/promotion-report.json`
records their relationship and the exact validation scope.
No common tool implementation changes or new pipeline wrapper were needed.
The new TimerGraphNames header contains shared engine event/timer identities;
the generator uses only committed shared Blueprint pin contracts. Detailed
final evidence belongs to the ignored final release and installation directories.
The shared tool index contained unrelated edits: only the mooring routing and
diagnostic contracts were committed, with the full working file preserved.
Coordination helper parameters were inspected because the documented normal
exact-file workflow did not cover this mixed-file preservation; the normal
commit wrapper still owned staging and commit under the same Git semaphore.

**Prior validated deployment baseline.** Test4 combines mooring
`MaxLengthBase = 6000` with male-socket `IntegratedCableLength = 6000`, and
removes marker3's hidden plate. Candidate output:
`artifacts/releases/MooringCable60m-60m-test4/`. Test4 installed after the user
closed the game at 2026-09-10 08:41 UTC. Installer and independent status
readback confirmed all four hashes and game fingerprint. Current installation
and rollback manifest:
`artifacts/installations/MooringCable60m/20260910-084122-60m-test4-920a3f5d/install-manifest.json`.
User confirmed success with test4: screenshot shows `55 m / 60 m` while
swimming with the free end. The displayed 60 m maximum and physical deployment
beyond the former 20 m cap are game-validated. The exact endpoint and save/load
remain pending. The user subsequently reports
that attached cables seem to hold and the boat reached the dock, but sharks now
trail behind the boat instead of staying beside the socket and the boat rocks
more than expected at the dock. This is qualified attachment evidence, not
validation of unchanged tension/holding behavior. Increased slack versus changed
effective stiffness remains unresolved; do not compensate strength blindly.

Follow-up fingerprint check found installed Steam build `25191271`, executable
SHA-256 `747DC2553F7E68D8EA7ED0B2E0CAC6D08943EA3F50DD6ED822E9293E0B45F58B`.
The earlier native addresses and extracted contracts are historical evidence
only until revalidated. `Get-VoyageMappings.ps1` rejected this new fingerprint:
no reviewed mapping matches. The user started the game; generation failed first
in Windows PowerShell 5.1 (`ProcessStartInfo.ArgumentList` missing), then in
PowerShell 7 inside jmap (`unexpected null ptr`, out-of-range conversion).
Those attempts produced no validated mapping. A subsequent main-menu run
succeeded without changing the dumper. The validated mapping is now registered
under `mappings/Voyage/steam-25191271-ue5.8/`; see
`docs/voyage-mapping-25191271-tool-report.md`. Current stock mooring cable and
male socket parsed successfully with it. Live read-only samples proved
automatic payout during towing, not merely a stretched fixed-length cable.
User chose stock 20 m payout with 60 m manual deployment. Test5 above implements
that distinction without changing spring strength; runtime validation is recorded above.

### Live payout diagnosis (25191271)

The user loaded the save with one dock cable and two shark cables, and reported
that the sharks returned close to the boat after loading. After detaching from
the dock and towing, two same-address integrated cables changed as follows
(all values in cm; native component width 20 and actor tightness 0.2 throughout):

| Sample | Cable A rest/current/segments | Cable B rest/current/segments |
| --- | --- | --- |
| a, loaded | 420 / 518.05 / 21 | 540 / 649.98 / 27 |
| b, towing | 840 / 1019.11 / 42 | 960 / 1163.67 / 48 |
| c, towing | 2440 / 3085.08 / 122 | 2840 / 3590.33 / 142 |

Dock cable in sample a: rest 220, current 312.94, segments 11. It was detached
in b and absent in c. All three reads reported zero read errors. Strength 10,
strengthMax 1e9, stretchMultiplier 1, stretchOffset 5 and both length caps 6000
were observed. Current stock base/mooring CDO tuning matches these strength
and tightness defaults. No threefold reduction in spring coefficient was found.
The 20% threshold depends on issued rest length, not directly on the 60 m cap.

At `0x1453afd5f` the tick loads rest length as NumSegments * CableWidth and
multiplies by CableTightness. Its mooring/unheld branch (`0x1453afdd2` onward)
compares stored stretch against this allowance +/- one width, and requests
segment changes via vtable slot 0x780. For the eligible endpoint-owner case it
uses actor MaxLength as the ceiling (`0x1453afe51`); the positive segment change
is gated by `(1+tightness)*rest < MaxLength-width` (`0x1453afe7c` onward).
Thus the deployed-length cap is shared with automatic payout. Endpoint-owner
virtual predicate 0x4e0 still needs exact semantic identification before using
it as a public generalized contract. Observed sharks do take the payout path.
The native force calculation itself remains proportional to absolute excess
beyond the threshold, with damping/mass/clamping terms.

Evidence: `artifacts/mooring60/live-tension-25191271-{a,b,c}.json`,
`extension-25191271.json`, `extension-inputs-25191271.json`,
`classes-25191271-{cable,socket}.json`, `member-registry-25191271.json`.
Class singleton identities and used member offsets were revalidated against
the current executable before adding its hash to Read-VoyageCableState.py.
The runtime reader's new fields are enabled only for 25191271; older diagnostic
support is preserved. Three runtime calls each completed in about 1-1.5 seconds.
Get-VoyageAssetSummary confirmed no Blueprint functions in the stock base;
attached payout belongs to native logic. No physics candidate is yet built.

### New-build tension evidence (25191271)

Read-only executable inspection and decoded native instructions found the
connected-force path at VA `0x14538cb00`. At `0x14538ccec` through `0x14538cd16`,
it computes `D = CableLengthCurrent - (1 + CableTightness) * N * W` and exits
without applying this force when D is nonpositive. N is component NumSegments
at 0x628, W is component CableWidth at 0x68c, and CableLengthCurrent is 0x624.
Do not rename W to segment length without tracing its lifecycle. Actor
CableTightness is 0x410. The additive constant was decoded as float 1.0.
At `0x14538cf71`, the positive branch uses
`(D * CableStretchForceMultiplier + CableStretchForceOffset) * CableStrength`
plus a velocity-dependent term, then mass-related scaling, CableStrengthMax
clamping and timestep/scaling factors. This supports a length-related onset
threshold rather than proving an inverse-length spring constant. Actual N/W,
their relationship to the two 6000 limits, and the current live tightness still
need measurement. A threefold behavioral change is not yet established.

Evidence: ignored `artifacts/mooring60/tension-25191271.json`,
`tension-prologue-25191271.json`, `tension-fields-25191271.json`; registration
reports `artifacts/inspection/executable-7d2975834bc1419bbcd32448ed5fa169.txt`
and `executable-4c952127181544eba41e8ca9f0792aac.txt`.
Tool report: documented mapping generator failed as above; source read was
limited to its failed process launch. Two compact executable queries succeeded;
decoded member analysis was necessary because correlations alone cannot show
the formula. Narrow PE descriptor inspection resolved component field names.
No game-memory writes, package mutation, build, installation or runtime
validation of a tension correction occurred.
Marker3 has been replaced and its plate visibility override removed.
Installation tools succeeded without fallback. No new build or installation
was performed when recording this game result.
No further reel-in prompt is required to establish the two-cap diagnosis.

### Prior test and installation identities

2026-09-10: user selected **mooring only, 60 m**. Candidate `60m-test1`
is built and structurally validated under
`artifacts/releases/MooringCable60m-60m-test1-r2/`; installed with explicit
user authorization at 2026-09-10 07:40 UTC. Runtime test FAILED: user reports
the same 20 m maximum; screenshot displays `MOORING CABLE`, `18 m / 20 m`.
Its only semantic change is mooring CDO `MaxLengthBase = 6000`.
Installation manifest for hash checks and rollback:
`artifacts/installations/MooringCable60m/20260910-074031-60m-test1-11570668/install-manifest.json`.
Test2 prepared and validated at
`artifacts/releases/MooringCable60m-60m-test2-r2/release-manifest.json`.
It changes only the male socket's IntegratedCableLength 2000 -> 6000.
Installation attempt stopped because the game had started; installer reported
rollback. Transaction:
`artifacts/installations/MooringCable60m/20260910-075220-60m-test2-c5cea7e9/install-transaction.json`.
After the user closed the game, test2 installed successfully at 2026-09-10
07:59 UTC through `Install-VoyageRelease.ps1 -AllowDirtySource`. All four
installed file hashes matched. Current installation/rollback manifest:
`artifacts/installations/MooringCable60m/20260910-075915-60m-test2-88e7e42d/install-manifest.json`.
Test2 runtime FAILED: user screenshot shows `MOORING CABLE`, `1 m / 20 m`.
Two class/template-default experiments have now produced no visible change.
Marker3 installed after the user closed the game at 2026-09-10 08:21 UTC.
Installer and independent readback confirmed all four hashes and game identity.
Current installation/rollback manifest:
`artifacts/installations/MooringCable60m/20260910-082129-60m-marker3-d02ef408/install-manifest.json`.
Marker3 user result: plate DISAPPEARED, while display and physical limit both
remain 20 m. Asset override/template propagation is now demonstrated for the
plate; stop treating global failure to load the package as the leading cause.
Next action: trace writes to effective MaxLength and restore the plate in the
next package. Source release manifest:
`artifacts/releases/MooringCable60m-60m-marker3/release-manifest.json`.
This preserves test2 length and adds only ship-plate `bHiddenInGame=true`;
it is a temporary diagnostic, not an accepted feature candidate. Restore the
plate after the result. Do not ship a third guessed length field.
Installation and independent status readback succeeded; package presence alone
does not prove the live instance inherited the modified default.
Producer and usage:
`mods/MooringCable60m/README.md`.

## Verified serialized contract

### Effective length contract (current-build native and runtime evidence)

The mooring deployment path enforces BOTH caps. Native fragment 0x1453b2250
loads MaxLength at +0x3fc and MaxLengthBase at +0x400, initializes the candidate
limit from MaxLengthBase, then applies `minss` with MaxLength when positive
(0x1453b22cf..0x1453b22d4; repeated at 0x1453b22e8..0x1453b22ed).
The physical extension path independently rejects length beyond either positive
cap at 0x1453affa4..0x1453affe5. Creation copies socket IntegratedCableLength
into MaxLength as described below. Thus changing only one cap cannot give 60 m.

Read-only runtime evidence `artifacts/mooring60/live-sockets-marker3.json`
found four placed mooring socket components with length 6000, plus their
template, and mooring cable CDO MaxLengthBase 2000. No read errors occurred.
After the user confirmed holding the free end, a second direct snapshot
`artifacts/mooring60/live-held-marker3.json` captured one non-CDO integrated
cable: MaxLength=6000, MaxLengthBase=2000, integrated=1, linked to a socket
with IntegratedCableLength=6000. It returned 87 cables, 10 integrated sockets
and zero read errors. This directly confirms the remaining 20 m base cap on
the user's held cable and justifies the combined change.
Detailed decoded consumer instructions: `artifacts/mooring60/dual-limit-evidence.json`.

The runtime reader uses only query/read rights and current fingerprint-pinned
class singleton/member identities. Its first attempt used the wrong FUObjectItem
pointer position, producing zero matches and many read errors; that result is
invalid. The documented scanner's layout confirmed 24-byte items with object
pointer at +8. Corrected reads returned 86 cables and 10 integrated-socket
records with zero errors in about 1.6 seconds. An intervening observation saw
only 10 CDOs during save unload; it was not treated as a live gameplay result.
No process mutation, debugger attachment, or injection was used.

Test4 preparation completed successfully through Windows PowerShell 5.1:
both unchanged roundtrips, exact two-package container verification,
independent semantic readback of only the two length changes, and common
schema-2 manifest validation passed. Marker visibility override is absent.
Evidence: `artifacts/releases/MooringCable60m-60m-test4/build-report.json`.
Installation and real-game validation are the remaining gates. The new narrow
runtime read method was required to resolve this actual failed workflow; no
generic process editor or unrelated pipeline work was introduced.

Fresh stock-game inspection used Steam build `25056839`, executable SHA-256
`CA84428CF4562C703BEDFF053DB727D14CC70C593451C09BE75A92828EFD9933`.
Revalidate these contracts after a fingerprint change.

- `/Game/Blueprints/Cables/BP_ModuleCable_Base` derives from native
  `/Script/Voyage.ModulesConnectCable`; its default object explicitly sets
  `MaxLengthBase = 2000`, `CableSnapLength = 1000`, and `MinSegments = 8`.
- `/Game/Blueprints/Cables/BP_ModuleCable_Mooring` derives from that base;
  its default object does not override `MaxLengthBase`. It explicitly sets
  `bMooringCable = true`, `CableSnapLength = 2000`, `CableStrengthMax = 1e9`,
  `CableStrength = 10`, and `CableStretchForceOffset = 5`.
- `/Game/Data/Assets/Cable/DA_Cable_Mooring` selects
  `BP_ModuleCable_Mooring_C` as `CableClass`; no explicit length override
  appears in this item asset.
- `/Game/Blueprints/Cables/BP_MooringCable_Socket_Male` is an Actor with
  `VoyageModuleSocketView_GEN_VARIABLE` (VoyageModuleSocketViewComponent).
  That component explicitly sets `IntegratedCable = DA_Cable_Mooring` and
  `IntegratedCableLength = 2000`. The initial research incorrectly claimed
  absence after inspecting a truncated search result; the complete relevant
  component confirms both fields. Never treat a truncated search as absence.
- The native `ModuleSocketComponent` mapping owns `IntegratedCable` and
  `IntegratedCableLength`. Female and craftable mooring socket assets contain
  length 1000 but no explicit IntegratedCable item; test2 leaves them unchanged.
- Reviewed mappings identify `MaxLength`, `MaxLengthBase`,
  `bCanSnapFromStretching`, and `CableSnapLength` on `ModulesConnectCable`.

## Interpretation and bounded experiment

The actor-default `MaxLengthBase` hypothesis failed the user's game test.
The socket's explicit `IntegratedCableLength = 2000` is the next, much more
direct candidate for the built-in reel. Test2 changes that existing property
to 6000 on the male mooring socket, and removes the actor-default override by
replacing the old container with a one-package socket-only container. The
native assignment/initialization trace is not proven; a real-game result is
still required. This experiment is supported by fresh socket producer data,
not an arbitrary second length-field adjustment.

## Test2 diagnostics

### Native evidence and marker3

The user confirms a physical stop at 20 m matching the display. Do not frame
this as merely a UI error or assert old-save causality without evidence.
The optional full-reel experiment was not confirmed and must not block
independent investigation or be repeatedly requested.

Current-fingerprint executable inspector resolved registration records for
IntegratedCableLength (native member offset 0x778) and MaxLengthBase (0x400);
the preceding MaxLength field is 0x3fc. Decoded native sequence at
0x14548d0bc reads `[rax+0x778]`, then at 0x14548d0c2 writes the same 32-bit value
to `[rdi+0x3fc]`. Nearby code copies the source's IntegratedCable item at 0x770
to the cable's item at 0x500 and sets its integrated flag at 0x414. This supports
the socket-to-cable assignment, rather than changing more defaults. It does not
prove which live socket instance supplies the value. Detailed fingerprinted
evidence is in `artifacts/mooring60/socket-to-cable-length-evidence.json` and
the inspector/native-members reports.

Marker3 changes only the existing test2 length and the ship plate component's
render visibility. It does not alter collision, transform, socket identity,
or cable interaction. The stock level's four male plate instances inherit
this template and do not serialize bHiddenInGame. Expected discriminator:
plate absent + 20 m means the template applies but a later value source remains;
plate present means loading/template propagation or a later visibility override
needs investigation before further length editing. Do not treat either result
alone as absolute proof of all lifecycle behavior.

Tool report: compact executable inspector found fields but only byte
correlations, motivating `Inspect-VoyageNativeMemberAccess.py` for actual
decoded operands. Python/capstone/pefile were isolated under ignored artifacts;
no process writes or injection. The initial general PE parse consumed excessive
memory; the method now reads only headers and the exception range table.
Decoded reports remain ignored. Marker3 Windows PowerShell build, unchanged
roundtrip, exact one-package verification, independent two-change semantic
comparison and schema-2 validation passed. Game testing remains pending.

### After the second no-op

Fresh fingerprint and installation readback match build 25056839 and test2;
the game is running, so no installed files were changed. Exact stock
`/Game/Maps/Boats/TestBoat` JSON (SHA-256
`2069EE88E89821C92EDBE8DA7F541D8F4C80B0FE60F8BDEECBA7D76DEF6683C1`)
contains four male socket actors ending `_0`, `_1`, `_2`, `_7`, whose socket
components reference `BP_MooringCable_Socket_Male.8`. None of those component
instances serializes IntegratedCableLength or IntegratedCable. Two other
actors are actually female despite their Male-looking object names. Use class
and template identity, not instance name. The male Blueprint summary has zero
serialized functions, excluding a Blueprint BeginPlay assignment there.

The next lightweight discriminator requested from the user is full reel-in
with Q until the free end disappears, then a fresh deployment. A pre-existing
cable may retain runtime/saved length; this remains a hypothesis, and reel-in
is not yet proven to destroy/recreate the actor. If unchanged, require a strong
visible marker or direct runtime/native producer evidence before another length
candidate. A level-wide override or third guessed field is not justified.

Tool report for this pass: public installation status/fingerprint/inventory,
one exact map JSON retrieval, and narrow Blueprint summary succeeded. No
build, installation, native-code investigation, or tool-source inspection.
Installed-directory log path was absent; local Saved/Logs inventory did not
establish a useful game log. No reusable mechanical gap established. Main
remaining uncertainty is runtime initialization/persistence and actual override
use; structural package readback alone does not settle it.

Public installation status confirmed test1 installed hashes and game fingerprint
still match. Inventory, narrow Cables reference search, exact asset JSON, and
ModuleSocket mappings resolved the socket-owned length. Boat base/barge and
item JSON did not own the relevant value. No native binary scan was needed.
The first test2 build passed container verification but its semantic comparator
stopped because PS5.1 serialized a script-assigned numeric value differently
from the JSON-parsed float. A focused diff contained only that reset value;
the comparator now restores the actual stock typed value before exact comparison.
The assertion remains strict; no additional differences are allowed.

Do not equate `CableSnapLength` with the deployment limit: the base already
has different values for these two properties. Its exact stretch/snap formula
is unverified. First change only `MaxLengthBase`, with a recoverable baseline.
If deployment passes but connected stretching fails, record the measured
failure before investigating the separate snap threshold. Do not blindly
scale strength, segment count, or visual cable length.

No supported INI or console setting was established. ClassConfigName=Engine
alone does not prove that a particular native property is configurable.

## Tool-use report

### 60 m candidate preparation

- Authorized installation: `Install-VoyageRelease.ps1 -AllowDirtySource`
  succeeded with matching hashes for all four installed files (triplet and
  provenance ZIP). Game-process guard passed. Installation evidence and
  rollback identity are recorded in the restart section above.

- `Build-Candidate.ps1` passed through Windows PowerShell 5.1 after fixing
  its nested JSON-array handling. No game files or Git state were changed.
- Documented fingerprint, mapping, extraction, GUI roundtrip, bounded GUI/
  retoc, container verification, independent Mod JSON inspection, and common
  manifest producer entry points passed. Source format lookup was limited to
  the existing GUI regression's CLI argument order and public parameter blocks;
  no shared tool implementation was changed.
- Unchanged roundtrip: zero RawExports and unchanged export payload; whole
  header byte equality is false due to the documented JSON name-hash behavior.
- Container: exactly one expected mooring package. Independent CUE4Parse JSON
  equals stock after removing only the new `MaxLengthBase = 6000` property.
- ZIP and immutable schema-2 manifest passed installer `-ValidateOnly`.
  Manifest SHA-256: `346E75BCC055592F4BBD223475E4199B5ABA969152B266423C8419CA95205DBC`.
  Full evidence: candidate directory `build-report.json`.
- Diagnostic retry with relative `-ModContainer` failed because that interface
  resolves relative names under Paks; absolute path succeeded. The producer
  already uses an absolute path. A status check mistakenly used `-Compact`;
  documented `-Summary` is the supported flag. These were caller errors,
  not recurring tool defects. No reusable tool gap established.
- Runtime behavior, including snap semantics and saved-cable initialization,
  remains unvalidated. No commits before the required game test.

### Initial research

- Intent: identify the smallest length-setting change using stock defaults.
- Entry points: `Get-VoyageBuildFingerprint.ps1`, `Get-VoyageAssetJson.ps1`
  for the four exact assets above, and `Inspect-VoyageAsset.ps1` for mappings.
- All four JSON retrievals and exact native mapping lookup succeeded using
  the documented published tools and reviewed mappings. Evidence remains in
  ignored tool-returned JSON paths and inspection manifests.
- Initial guessed mapping query `mappings:VoyageModuleCable` exited 1;
  its log explicitly reported zero mapped types. Reading the base asset
  established the real owner `ModulesConnectCable`; that query succeeded.
- No tool implementation inspection, binary analysis, build, or installation.
  Validation is serialized data and mapping inspection only.
- No new recurring mechanical tool gap established; native formula tracing
  and real-game testing remain pending if the one-property experiment needs them.
