# Agent work measurement

Use this opt-in tool only for an explicitly requested measurement. It reads one
named Codex JSONL session and writes its complete measurement evidence below a
new repository-local `Tmp/` directory. It does not search for sessions, message
markers, encrypted payloads, or fixed dates.

Use the bundled Python runtime:

```powershell
$python = 'C:\Users\IH\.cache\codex-runtimes\codex-primary-runtime\dependencies\python\python.exe'
& $python .\tools\measure-agent-work.py start `
  --session-path <absolute-session.jsonl> --session-id <exact-session-id> `
  --output-directory .\Tmp\agent-work\measure-<purpose>
```

For a newly assigned agent whose complete session is the intended boundary,
add `--from-session-start`. This explicit mode verifies the supplied session ID,
uses the `session_meta` timestamp, and sets the cumulative baseline to zero so
the initial prompt, READY exchange, and first reads are included. Normal start
behavior is unchanged.

The start command records the timestamp, file size, session metadata, latest
pre-start model/effort context, and latest available cumulative counter. Finish with the returned
`measurement_path`:

```powershell
& $python .\tools\measure-agent-work.py finish `
  --measurement-path .\Tmp\agent-work\measure-<purpose>\measurement-start.json
```

Finish prints a compact result and writes `measurement-report.txt` plus
`measurement-result.json`. The JSON contains the timestamp cutoff, cached and
uncached input, output and reasoning subset, request samples, first/peak request
input, separate top-level and statically nested tool-call counts, elapsed time,
inter-agent call count, and bounded evidence timelines. Retry count remains
unknown without an explicit structured source. A partial final JSONL record is ignored.
The first report total is `used_tokens = input_tokens + output_tokens`; cached
input is counted fully and the reasoning subset is not added again. This is a
cold-proxy total, not proof that provider caching was disabled and not a quota
or pricing calculation.
Missing counters remain `null`; no usage samples is reported separately from a
measured zero, and any reset between consecutive cumulative counters makes
deltas unknown. Corrupt complete lines, a partial live tail, missing baseline,
or session truncation are explicit coverage states. Finish rechecks that its
state path is still below repository `Tmp/`. Tool
argument size is not treated as plaintext inter-agent communication.

If start or finish rejects the explicit identity, incomplete state, counter
sequence, or output location, keep the small error and correct the supplied
paths. Do not fall back to transcript-wide discovery.
