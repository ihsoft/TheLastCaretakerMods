# Tool use and coding-agent reports

Coding-agent handoffs follow the [shared subagent workflow](../docs/subagent-workflow-rule.md).
Include a compact tool-use report:

```text
Operations: <intent -> public tool -> result/evidence>
Unexpected: <failure and diagnostic path, or none>
Fallback: <manual/source inspection and its trigger, or none>
Validation: <checks completed and game-test status>
Coverage: <public-tool operations>/<eligible recurring operations> = <percent or N/A>
Routine cost: <public calls; internal files opened; repeated or oversized output>
Reusable gap: <missing interface or recurring friction, or none>
```

Count recurring mechanical game-asset and release operations for the shared
tool-coverage target. Count each requested operation once; do not count
reasoning, source editing, documentation, or novel diagnosis after a tool
failure. Report an empty denominator as `N/A`. A gap is evidence for later
triage, not automatic permission to build another wrapper.
