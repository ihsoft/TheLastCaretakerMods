# BlueprintText

For new and existing mods, start with the [workspace workflow](../blueprint-workspace.md).
The [format guide](../blueprint-text.md) owns lower-level interfaces.
BlueprintText is the recommended Blueprint authoring workflow. Packaging and
installation remain the mod producer's responsibility. Keep generated projects
and results under `Tmp/BlueprintText/`; keep the catalogue cache local and ignored.

The agent edits desired source directly. Extend shared factories and reflection
when needed; do not introduce per-mod graph-building code. Unsupported state
must be diagnosed rather than silently discarded. Do not imply universal
Blueprint preservation from the bounded tests.

## Routine agent work

- Use the shared workflow entry point and its compact result. Read the workspace
  manifest to find editable Blueprint JSON; inspect only source needed for the
  requested change. Edit complete desired source, not generated native text.
- Generated previews are for the user. Supply review annotations to the renderer
  and return its index link. Do not open or analyze generated HTML, SVG, PNGs or
  screenshots to reconfirm a successful render or infer changes already known
  from the source. Inspect preview output only when the user asks to investigate
  the preview itself, or a concrete rendering failure requires it.
- Baselines, native intermediates, resource manifests, comparison dumps and
  full logs are tool-owned. Do not read them during a successful routine cycle.
  On failure, start with the compact diagnostic and read only the indicated
  evidence relevant to that failure. Do not repeat a successful mechanical check
  by manually comparing exports or running a second verification script.
- Do not create per-mod orchestration scripts for a documented workflow. If the
  shared interface cannot perform the requested operation, report the missing
  capability or fix the shared tool. Focused tool-development test fixtures are
  separate from ordinary mod work.
- Warn before engine actions. Let the tool own backups, conflict checks and
  recovery; do not bypass them or manually overwrite an asset changed since
  export. Existing resources and native mirrors remain project dependencies.
- Give independent tasks separate workspace directories. One task owns edits to
  a workspace, including its JSON and review annotations; command locks cannot
  protect files edited directly by another agent. Coordinate ownership before
  sharing sources or the same selected assets. Use the public workflow for engine
  operations; do not bypass a busy lock with low-level scripts, direct Unreal
  launches, or deleting lock files. Other engine tools and manual editor sessions
  must be coordinated separately; they do not participate in this tool's locks.
