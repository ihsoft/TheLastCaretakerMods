# Release and installation workflow

This policy applies to every mod in `mods/`, including mods still in research.
The mod's root `version.txt` contains one non-negative decimal integer: the
latest numbered release published on Nexus. `0` is a special value meaning
there has been no numbered `V1` release yet. The mod may already have been
published on Nexus without such a release number. `V0` is never a release.
It is not a build number, a game version, or the version of the currently
installed local package. Present positive values as `V<N>`; never present `0`
as `V0`.
Change it only for an explicitly requested release, after that exact version
has been published. A local build, game test, or successful installation does
not advance it.

Work after `V<N>` consists of candidates for `V<N+1>`; with `0`, the work is
on candidates for the first numbered release, `V1`.
Candidate names may start with that next version and a distinguishing suffix;
they must never imply that publication has occurred. A candidate promoted to
a release keeps one exact ZIP identity. Publishing a different ZIP under the
same version is a conflict to resolve explicitly, not a routine overwrite.

## Output lifetime

Builds and installations used for experiments or feature development belong
under ignored `Tmp/`. Keep the location of the next usable build in the active
task context or a large task's ignored backlog. A sequence of temporary
installations does not need a durable record of each replaced build, its hashes,
or its version. The installer may use hashes and backups while an installation
is in progress to detect a bad copy and recover a partial mutation. Once that
operation and any coupled settings edit have succeeded, those records are
disposable. Temporary outputs do not become release artifacts merely because
they were installed or tested in game.

Only explicitly selected release candidates and published releases belong in
ignored `artifacts/`. Put a selected candidate's exact ZIP and release manifest under
`artifacts/installations/<Mod>-V<N>/`, for example
`artifacts/installations/Railgun-V5/`. The directory identifies the proposed
release version, not a timestamp or installation run. Refuse to reuse an
existing version directory for a different ZIP. Keep the selected ZIP there
through publication and for later identification or recovery. Historical
timestamped directories may be read for existing restore dependencies; migrate
or remove them only after their ownership and recovery value are established.

`Voyage/Content/Paks` and `LogicMods` contain only files consumed by the game.
Never copy a ZIP into either directory, including for a temporary build. A
release archive is validated as an input and retained in repository artifacts;
its presence in the game directory provides no runtime value.

## Installation and rollback

The operator keeps track of two ready packages: **current** and **previous**.
If the current package fails a game test, reinstall the previous package as it
is. The path to each working package can stay in the active task context or an
ignored backlog when the task is large. There is no release history of
`replaces` relationships, installed predecessor hashes, or backup chains.

An installer may create a transaction record and copy existing files under
`Tmp/` while an installation or coupled settings edit is in progress. These
are short-lived recovery data for an interrupted operation, not a package to
choose for rollback after testing. Never put them in the versioned release
directory. Railgun's settings backup has the same limited purpose: preserve
the user's INI through the settings mutation, then discard the operation
record once it has completed. Keep failure evidence in `Tmp/` only while it is
needed to resolve that failure.

Check that the game is closed immediately before changing installed files,
backup existing targets, install from a validated manifest, read back every
installed hash, and restore or stop with explicit recovery evidence on failure.
Follow the [shared installer contract](../tools/README.md#release-installation)
and any owning mod's additional layout or settings contract.
