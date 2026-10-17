# Generated output lifecycle

This policy applies to research, feature development, builds, releases, and
installation work across the repository. It governs ignored generated output;
Git source and documentation follow their owning rules.

An optional backlog for a large task is a local working note, not generated
output or an artifact. Keep it outside `Tmp/`, ignore it in Git, and retire it
as described in the [research workflow](research-workflow.md#working-backlogs-for-large-tasks).

## Temporary output and retained artifacts

Use repository-local ignored `Tmp/` for the complete output of a work cycle by
default. A cycle may be one investigation, feature, or release preparation.
Logs, intermediate files, one-off checks, full readbacks, disassembly, test
fixtures, and unsuccessful candidates belong there. A request to generate,
export, display, or hand off a file does not by itself make it worth retaining.

An **artifact** is generated output with a concrete reason to remain usable
beyond the current cycle. Put it under ignored `artifacts/` only when an
identified consumer needs it later, it materially avoids repeated work, it
supports rollback or release provenance, or it is explicitly selected research
for ongoing work. Record its owner, purpose, and condition for revalidation or
removal. This does not require a separate manifest for every file.

An output is not retained merely because it already sits under `artifacts/`,
was costly to produce, or came from a failed run. Keep exceptional failure
diagnostics only while an unresolved problem gives them continuing value. If
retention may matter but its continuing use is unclear, ask before creating a
new retained family. Do not register a family retroactively to justify an
output location.

Before creating or changing retained output, consult the family registry in
[`artifacts/README.md`](../artifacts/README.md) and its local
[`AGENTS.md`](../artifacts/AGENTS.md). A family needs an owner and continuing
consumer; individual run directories inherit that contract. Consult the owner
before changing or removing existing retained content, and do not delete
content whose value or dependencies are unknown.

Selected release candidates and published release packages must remain usable
without `Tmp/`. Working builds and transient installation
recovery state remain in `Tmp/`. Keep temporary state only through the active
install operation and any coupled settings change. The
[release workflow](release-and-installation.md) governs
this distinction for all mods. Promote reusable findings
to documentation and recurring methods to their owning tools; retained raw
output does not substitute for either.

## Game-derived output and Git

Do not commit data reproducible from the installed game: extracted assets,
JSON, `.jmap`, pseudocode, raw reports, disassembly, cooked packages, container
inventories, or installed-file backups. The reviewed `.usmap` registry under
`mappings/Voyage/` is the explicit exception. User-authored mod assets are
ordinary source inputs; that does not make extracted game meshes or materials
committable. Storage lifetime does not change these Git boundaries.

Bind retained game-derived inputs to the relevant Steam build ID and asset
contract. Revalidate them when that contract changes. Engine-only conclusions
follow their engine and serialization evidence rather than acquiring an
unnecessary game-build gate.
