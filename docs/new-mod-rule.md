# Starting a new mod

Use this workflow when a feature request has no owning mod directory yet. The
request describes a goal, not an established game contract or implementation.

## Find the starting facts

Read the [repository index](../AGENTS.md). Follow only the shared knowledge
relevant to the requested behavior, then search existing mod indexes and
architecture for related objects or mechanics. A mod-local finding can identify
a useful stock object, but its implementation is an example, not a requirement
for the new mod. Record the exact owner and applicability of any reused fact.
Investigate how to achieve the requested behavior only after identifying the
object that owns it. Do not invent a game-build pin or choose an Unreal project,
loader, override, or producer from the feature description alone.

For example, a request to recolor all large containers can start from the
[ContainerLidLabel architecture](../mods/ContainerLidLabel/ARCHITECTURE.md),
which identifies the stock Large container Blueprint. That identity does not
yet establish which material or component to change for the requested color.

## Create the minimal directory

Once the mod has a name and one-sentence goal, call the public
[`New-ModSkeleton.ps1`](../tools/mod-setup.md#new-mod-skeleton) entry point. It
creates `mods/<Mod>/` with only:

- `AGENTS.md`: a short index to the repository index and local architecture.
  Add links when real mod-specific documents appear.
- `ARCHITECTURE.md`: the goal and an explicit statement that no implementation
  or game contract has been established yet. Replace that statement with the
  current design and its known boundaries as evidence arrives.
- `README.md`: a short folder label, marked planned until the mod works. Add
  the command-line candidate build command when a public producer exists.
- `version.txt`: `0`, because no numbered `V1` release has been published.

The scaffold does not create `MOD-RULES.md`, `game-build.json`, provenance
files, build scripts, an Unreal project, or a generic backlog. Add each only
when the chosen implementation needs it. `MOD-RULES.md` holds durable local
constraints; `game-build.json` belongs to a game-bound contract; source
provenance belongs with actual game-derived inputs. A large active
investigation may use an ignored backlog under the mod, following the
[research workflow](research-workflow-rule.md#working-backlogs-for-large-tasks).

## Advance the mod

Keep the architecture and README honest about what exists. A planned skeleton
does not claim a working mod, a supported game Update, or a release. When a
public producer exists, follow the [release workflow](release-and-installation-rule.md):
build first, diagnose only an actual failure, and seek a proportional game test
before claiming runtime behavior. Commit a skeleton or later research checkpoint
only when its recorded facts and status are accurate and useful; never commit an
active backlog or raw game-derived output.
