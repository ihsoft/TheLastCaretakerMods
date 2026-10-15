# CAP-01: process observation

## Scope and measurement contract

User-requested observation of this task only; not a new repository-wide rule or
telemetry framework. Starts 2026-10-15 04:44:12 UTC (first clock sample; opening
message/read preceded it by roughly seconds). Earlier feasibility research is
excluded. User chose extending ContainerLidLabel and a default of 200 kg.

Researcher /root owns contracts, brief, review and conclusions. Coder
/root/container_capacity_coder owns implementation/build and its local debugging.
Clean fork: no inherited conversation. Spawn requested/accepted gpt-5.6-sol,
medium. Local turn_context records subsequently confirmed coder gpt-5.6-sol /
medium and parent gpt-6-astra / high. Child's generic initial model
self-description was not runtime identity and was corrected from these records.

Measure phase wall time, tool/command counts, retries, files/excerpts read,
build waits, messages and transferred characters. Local session token_count
events expose cumulative input, cached input, output and reasoning counters;
task usage is the delta from the pre-task baseline (child baseline zero).
Counts of top-level calls and serialized tool-output characters are also read
from these two sessions only. Nested shell commands/files and retry reasons
remain manual. These are not billed cost or active compute duration.
Concurrent wall intervals must not be added as elapsed task duration. Separate
user/agent/build waits, research, implementation, review and observation overhead.

Each parent recheck must record its reason, evidence, duration estimate and
whether it found a defect/changed a decision, merely confirmed a claim, or was
redundant. Do not repeat a build or asset investigation merely for assurance.
Review a compact report, relevant diff and static acceptance evidence; open raw
compiler output only for a concrete unresolved failure. Logs stay in Tmp.

## Phase ledger

| Phase | Owner | UTC interval | Observations |
|---|---|---|---|
| Scope/current contracts | Researcher | 04:44:12 onward | New reviewed mappings now exist for 25929985; old blocker removed. User chose existing patch and 200 kg default. Current Large default is 200. Current stock base calls native setter. BeginPlay and PostLoad contracts inspected. |
| Read-only implementation preparation | Coder | 04:45:48–04:46:28 | Self-count: 6 exec batches, 24 underlying read commands, 15 full files + 3 excerpts, 1 memory registry search, 0 retries/builds. One noisy generated-file search, then narrower scope. Report about 2.5k characters; no compiler log forwarded. |
| Coordination | Both | overlapping | READY/ACK/receipt succeeded. First brief supplied scope/rules/metrics; implementation brief follows researched contracts. |
| Measurement setup | Researcher | approximately 04:50:30–04:54:01 | Read only own session plus verified child's metadata/counters. Created a disposable aggregate reader under Tmp. Initial live-file tail read stalled and was interrupted; rg-prefiltered records worked. Fixed timestamp comparison with explicit DateTimeOffset and included custom-tool records. Roughly 3.5 minutes of observation overhead, overlapping implementation. |
| Cache scope refinement | Researcher then coder | approximately 05:05 onward | User rejected per-container file reads. First corrected release050337 already existed and passed coder's13 assertions; held as superseded, never installed. Parent verified native GetClassDefaultObject owner/signature and CDO execution, selected cache on owned CDO with two packages. This is a user-driven scope change, not another compile retry. |

## Review/rework ledger

Parent did not reread the coder's 15 preparation files. The source-surface report
established where edits belong; it did not establish game contracts and was not
treated as proof of them.

First selective review, approximately 05:01:52–05:03:17 UTC, read roughly 440
lines/excerpts of the one generator (including some unchanged helpers), the
INI, and a README excerpt. A preliminary full diff command was poorly bounded
and truncated; subsequent reads were targeted. No compiler logs or build rerun
were consumed by the parent.

| Recheck | Reason and evidence | Result/value |
|---|---|---|
| Parent lifecycle calls | Compare exact current-game owner with generated placeholder/override source, then one public Mod summary of built candidate 045714 | Found a substantive defect: PostLoad called BP_Container_Base directly; cooked BeginPlay called Actor directly. Both bypass stock PlayerBox work. Build success and presence of parent-call nodes were insufficient. Coder accepted correction to PlayerBox-owned placeholders and exact cooked target assertions. |
| INI numeric validation | Read positive-double/range check and conversion | Found representability edge: tiny positive double can become zero float. Coder added positivity check after conversion. Lower-priority than lifecycle defect; fixed in the same necessary rebuild. |
| Setter/meter target and optional default | Read shared apply function and INI | Confirmed intended inventory target, native setter, explicit bar refresh and 200 default; no defect found. |

The independent Mod-summary check needed one retry because relative
ModContainer resolved under game Paks; absolute release path succeeded. This is
parent review overhead. The first cooked report is
artifacts/asset-summaries/mod/20261015T050305973Z-cd93f261/summary.json.
No old-candidate inspection was requested again from the coder; the report was
handed over for the fix. Only the rebuilt candidate needs fresh validation.

Research overhead already observed: a missing mod AGENTS file was requested;
one inspector query used /Game syntax accepted by the summary wrapper but not
the lower inspector, yielding zero packages. Correct full virtual file path
succeeded. One rg context output repeated long paths and truncated; a bounded
pseudocode read replaced it. These are researcher costs, not coder rework.
One unquoted engine path caused a failed rg and was corrected. One metadata
read printed full base instructions unnecessarily; later reads select fields.

## Interim measured snapshot (04:54 UTC, work incomplete)

| Metric | Researcher | Coder |
|---|---:|---:|
| Summed request input tokens | 4,851,821 | 4,570,077 |
| Of those, cached | 4,783,872 | 4,429,952 |
| Input minus cached | 67,949 | 140,125 |
| Output tokens | 14,619 | 24,039 |
| Of output, reasoning tokens | 4,022 | 6,978 |
| Last nonzero request input/context | 133,109 | 158,800 |
| Top-level tool calls | 45 | 41 |
| Serialized tool-output characters | 181,774 | 333,650 |
| Sent inter-agent message characters | 12,232 | 4,696 |

Request inputs count repeated context on every request; they are not millions
of unique tokens read. Cached input is a subset of input, reasoning is a subset
of output. Character counts are not tokens. Output counters include generated
code and tool arguments. The parent has not read the coder's source edits or
compiler logs at this point. The child started with 43,988 input tokens despite
the clean fork: shared instructions/tools still have substantial fixed cost.
The latest events in this snapshot are 04:53:46, so it excludes subsequent work.
Raw aggregate is Tmp/container-capacity/process-snapshot.json; the disposable
reader is Measure-Process.ps1 in the same directory. Durable conclusions belong
here; no general-purpose telemetry tool is being added.

## Outcome and limits

Follow-up: user personally installed the delivered candidate and reported that
it works. No agent installation was performed; no independent installed-hash
readback or enumerated edge-case coverage is implied. Closing measurements below
remain scoped to preparation/review and exclude this follow-up.

Final candidate: Tmp/ContainerLidLabel/releases/ContainerLidLabel-v4-test-20261015-050916.
Static review accepted; not installed or game-validated. UToc SHA-256
A029008A9796401BF2F509AD4334CE67FD2DA79BB57C965F1C80C76208F3F9E2;
ZIP SHA-256 15C15F032C07B4D556FD1BED91D86C8A62043F9344D61EBD2E9A2B3BEFB58F7B.
Coder's final semantic report passes 17 assertions. Parent inspected the supplied
cooked summary, about 130 additional generator lines covering cache branching,
CDO ownership and the converted-float guard, then independently matched triplet,
ZIP, summary and INI hashes. No new functional discrepancy. No parent rebuild,
compiler-log read, or repeated native-contract investigation by coder.
Minor literal-naming cleanup can be folded into post-game source finalization;
it was not used to force another build of this test candidate.

Approximate wall span: 04:44–05:15 UTC, about 31 minutes. Coder preparation began
04:45:48 and final semantic/finalize work ended around 05:11, with reporting
through 05:13. This includes waits and overlaps parent work; it is not active
model time. Parent initial contract work/brief occupied roughly 6 minutes,
cache refinement roughly 3, selective review roughly 4, initial measurement
setup roughly 3.5, with later reporting/measurement and waiting additional.
These phase durations are approximate, overlapping and not additive CPU time.

Coder reports seven producer directories, five local producer retries, about
327 seconds of captured producer wait (282 earlier + 45 final cached version),
plus validation-only retries for relative container path and JSON escaping.
This is not seven full successful builds or a count of seven independent bugs.
About 18 distinct authored/reference files were read by coder (estimate).
Parent reviewed roughly 570 generator lines in two passes, plus small INI/doc
excerpts and compact artifact evidence, not all coder-read files. Initial
review occurred before the coder's first final BUILD_READY; it found real
defects but cannot prove that coder's remaining own checks would miss them.

### Closing measured snapshot

Sample taken 05:14:48 UTC; latest usage records are parent05:14:31 and
coder05:13:11. Excludes this final documentation and final response. Absolute
counts below come from session events, not estimates.

| Metric | Researcher (gpt-6-astra/high) | Coder (gpt-5.6-sol/medium) |
|---|---:|---:|
| Summed request input tokens | 10,850,192 | 17,525,282 |
| Cached input subset | 10,619,136 | 17,233,280 |
| Input minus cached | 231,056 | 292,002 |
| Output tokens | 31,797 | 60,517 |
| Reasoning subset of output | 13,229 | 15,528 |
| Initial request input | 70,394 | 43,988 |
| Peak observed request input | 180,666 | 211,254 |
| Last request input | 180,666 | 131,638 |
| Top-level tool calls | 84 | 140 |
| Serialized tool-output characters | 283,588 | 722,949 |
| Sent inter-agent message characters | 23,648 | 11,840 |

The researcher has 11 explicit agent waits among those calls. The coder's lower
last input than peak must not be represented as less total work. Uncached input
is not necessarily unique/new information; cache misses can reread old context.
Prices, billing and exact active inference time were not measured. No monetary
estimate is inferred from account-wide limits.

Conclusion: implementation/build noise was substantially isolated: roughly12k
characters of coder messages reached the parent against roughly723k characters
of that agent's tool output, with selective source/artifact review added by the
parent. Review produced one important lifecycle fix and one numeric edge fix.
But total researcher context was not kept small: request input grew70k to181k,
and instrumentation, verbose research reads, retries and coordination cost real
work. This observation supports partial context isolation, not proven total
token/cost savings. A single task cannot demonstrate causal savings versus a
single-agent baseline. Do not repeat the first-time measurement setup for each
small task or promote this disposable reader to a general tool without need.

## CAP-02: remove unnecessary INI section

User requested the flat Railgun/Gyro convention with comments preserved.
Scope start 2026-10-15 05:38:50 UTC; coder completion 05:49:44, parent acceptance
about 05:52. Roughly 13 minutes elapsed, overlapping roles, not active compute.
The unnecessary section was our initial choice; cleanup is additional cost.
Reused the same child and disposable measurement reader with a separate cutoff.
No new native investigation, installation, or Git mutation.

Accepted candidate: Tmp/ContainerLidLabel/releases/ContainerLidLabel-v5-test-20261015-054306.
UToc SHA256 2C4A90165BB33CF402CA2B5B1AADD74E5661DE27256EC3E0E9498DFF8260A221;
ZIP SHA256 58C0467474DAF2F41284109C49F01D0D987C49A6AB9AF116140DAF071278DCC0.
Build, container validation and 19 cooked assertions passed; runtime untested.
Parent reviewed roughly 106 parser lines, 52 build-entry lines and small config/doc
excerpts, then independently matched payload/ZIP/INI hashes and all 22 source
hashes. No code defect found; one stale installed-version documentation claim
corrected. No parent rebuild or compiler-log inspection. Coder reported one
PowerShell interpolation retry, one successful producer (~49 seconds), and two
manifest-only finalizations. Four interrupted temporary validation-report
serialization attempts cost ~180 seconds before compact evidence worked.
This was reporting friction, not gameplay failure; no new shared tool is justified.

Snapshot at about 05:50:07 UTC excludes final parent review/doc edits:

| Metric | Researcher | Coder |
|---|---:|---:|
| Summed request input tokens | 1,900,082 | 7,404,615 |
| Cached input subset | 1,853,056 | 7,362,048 |
| Input minus cached | 47,026 | 42,567 |
| Output tokens | 3,558 | 12,501 |
| Reasoning subset | 468 | 2,893 |
| Top-level tool calls, including waits | 31 | 48 |
| Serialized tool-output characters | 81,064 | 75,882 |
| Sent agent-message characters | 4,812 | 5,040 |

Metadata includes recovery overhead; characters are not unique evidence/tokens.
Costs and active inference time remain unmeasured. Implementation debugging
stayed with coder; only its summary reached parent. But coordination,
documentation and observation remain substantial for a tiny change, and parent
review found only a documentation discrepancy. Net delegation savings are not
established. One final parent document patch needed a context-line retry.

Installation follow-up: at the user's explicit request the parent installed v5
directly, without delegating or rebuilding. Public validate-only and installation
entry points passed on Steam25929985; game was closed and installed hashes matched.
INI migration preserved 200 kg and removed only the old section header, with a
separate exact backup/readback record alongside the common installation manifest:
artifacts/installations/ContainerLidLabel/20261015-060929-v5-test-0f93978d/.
No new telemetry run for this small operational follow-up; installation completed
at 06:09 UTC. The existing installer does not own INI, so its bounded manual
migration remains the known uncovered operation. No runtime success claimed.

## CAP-03: container-specific key

User confirmed installed v5 works, then requested a container-specific INI key.
Parent changed only the external key to LargeContainerMaxWeightKg in authored
INI, generator literal, producer validator and documentation. Known bounded
work was performed directly; no coder handoff or new game-contract research.
The public v6-test producer passed once, with two runtime packages. Reused cooked
validation passed 19 assertions after correcting one stale v5 ZIP filename in
the disposable validation script. Candidate v6-test-20261015-065220 is not installed.
Its ZIP SHA256 is DB8CA6D6D3F7BFFF7840DC058C4AFB5E163FD4618C052F69DD967AF5715211AF.
User authorized committing documentation only; code/config/build-script changes
remain in the working tree. No new token measurement was run for this follow-up.

Finalization follow-up: v6 was installed with hash readback, then the user
confirmed it works with a temporary 1000 kg value and authorized the code commit.
Authored INI and build-manifest default were independently checked as 200 kg;
all implementation source hashes still matched the validated v6 build. Only
documentation had changed. No rebuild or new runtime experiment was needed.
The personal installed INI was not imported into source. Original build/release
manifests and the 19-assertion report were retained with installation evidence;
the versioned installed ZIP preserves the payload independently of Tmp.
