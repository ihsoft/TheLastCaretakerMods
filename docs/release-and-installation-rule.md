# Release and installation workflow

This policy applies to every mod in `mods/`, including mods still in research.
The mod's root `version.txt` contains one non-negative decimal integer: the
latest numbered release published on Nexus. `0` is a special value meaning
there has been no numbered `V1` release yet. The mod may already have been
published on Nexus without such a release number. `V0` is never a release.
The value does not identify whether a Nexus mod page already exists; establish
the page identity separately when preparing publication.
It is not a build number, a game version, or the version of the currently
installed local package. Present positive values as `V<N>`; never present `0`
as `V0`.
Change it only for an explicitly requested release, after that exact version
has been published. A local build, game test, or successful installation does
not advance it.

Work after `V<N>` consists of candidates for `V<N+1>`; with `0`, the work is
on candidates for the first numbered release, `V1`.
For a requested publication, read `version.txt` directly and compare the
proposed release number with the numbered files already published on the mod's
Nexus page before selecting that number. If they disagree, resolve the target
release number explicitly; do not infer a missing or unread value from the
usual `N+1` path. This publication check does not precede an ordinary candidate
build that has no publication request, and it is not a game-version preflight.
Before publication, identify the supported game Update and whether an older
Update remains in use on the main or beta channel. A new release for one Update
does not automatically replace a downloadable file for another still-supported
Update. Decide whether to add a parallel file or replace an existing file
before preparing the Nexus upload. Use the
[Update/version mapping](game-update-versions-knowledge.md) for player-facing
compatibility; keep Steam build IDs in technical build evidence and mapping
provenance.
For a Nexus file belonging to that target release, enter the integer `N+1`
as its version, without a `.0` suffix. Until publication, `version.txt` still
contains `N` (or `0` before V1); it is not the candidate's Nexus file version.
The release ZIP name is exactly `<Mod>-V<N+1>.zip`, with the mod directory name
and an uppercase `V`. Feature names, fix names, candidate labels, timestamps,
and attempt numbers never belong in that filename. Distinguish temporary runs
with their directory below `Tmp/<Mod>/`; multiple runs may contain ZIPs with
the same canonical leaf name. Candidate status follows from `version.txt`, not
from a filename suffix: while `version.txt` contains `N`, `<Mod>-V<N+1>.zip` is
still a candidate. A candidate promoted to a release keeps one exact ZIP
identity. Publishing a different ZIP under the same version is a conflict to
resolve explicitly, not a routine overwrite.
Do not rename or rebuild an already retained historical package merely to make
its old name conform; the canonical rule applies when creating new candidates.
If the user explicitly changes the target release number after a game test,
keep the tested ZIP bytes and original build manifest. Rename the ZIP file to
the canonical name for the actual target without recompressing it. Name the
retained release directory for that target
and check whether any version-bearing content inside the ZIP makes the change
unsafe. Do not leave a local selection under the old release number or rebuild
solely to change a filename.

## Release archive

Every selected release candidate has one ZIP intended for Vortex. Its entries
must deliver every game-consumed payload file to the mod's required destination.
Do not wrap the payload in a release/version folder. Use the shared archive
producer for a standard IoStore triplet, including INI, content-plugin, and
LogicMods cases. It selects the standard layout and verifies the entry list,
payload readback, and any required override JSON. Its successful result
satisfies the structural ZIP check on the normal path. Consult
[Vortex layouts](vortex-release-archive-knowledge.md) when defining a
nonstandard destination or investigating an unexpected result; do not unpack
a successful routine archive merely to repeat the producer's checks.
Archive validation is structural. Claim Vortex
compatibility only after installing, enabling, and deploying that exact ZIP
in Vortex, then checking both required destinations and the absence of
unexpected deployed files; claim gameplay compatibility only after the
real-game test. A published ZIP remains immutable; packaging changes reach
users through a new release.

## Changelog

Every mod owns `mods/<Mod>/CHANGELOG.md`. It is repository source for
player-facing release notes, not payload: never put it inside the Vortex ZIP.
`## Next` describes the player-visible difference between the latest published
release and the version that would be published now. It is not a history of
commits, candidates, experiments, reversals, or implementation steps. Do not
use build logs, Git history, architecture documents, or a descriptive ZIP
filename as a substitute.

Before committing a change to a mod, determine whether the resulting mod changes
what a player receives or observes relative to the latest published release. If
it does, update `Next` in the same proposed commit. Keep its text aligned with
the current cumulative result: rewrite or remove an earlier entry when later
work changes, supersedes, or reverts that result. A reverted intermediate
change leaves no changelog entry. Experimental work that is not being accepted
into the mod does not enter the changelog. Internal refactors, build-tool
maintenance, and validation details belong there only when their final result
affects players.

When preparing publication, read `Next`, make it accurate and concise, and use
that text for the Nexus file changelog. Do not reconstruct a missing release
story from commit history or preserve notes about work that is absent from the
selected ZIP. An empty or materially incomplete `Next` is a publication
blocker, not permission for the publishing agent to invent one. After the exact
ZIP is publicly released and `version.txt` advances to `N`, rename that section
to `## V<N>` and create a fresh empty `## Next` section. Do not invent history
for releases that predate the changelog.

## Game update and candidate builds

The normal path is: run the public builder, install its candidate through the
owning documented entry point, test that exact ZIP in the game, keep that
selected candidate under `Tmp/<Mod>/`, then publish that same ZIP. Use the
documented producer and installer commands; do not add
a separate game-version or installation-status preflight. The producer and
installer own their game identification and installed-file checks. A running
game is a reported blocker for a phase that needs it closed, not a reason to
inspect game contracts manually.

Run the mod's public builder before manually inspecting game contracts. If its
only blocker is the pinned Steam build ID in `game-build.json`, rerun with
`-IgnorePinnedGameVersion`. The flag never changes the actual build ID in the
candidate's evidence or waives fresh-input and package checks. Missing current
mappings are a separate prerequisite; resolve them and retry. Analyze the
specific failing contract only when these inexpensive paths fail.

Offer an in-game test for a successful candidate. A passing build alone does
not establish gameplay compatibility or authorize committing implementation
and current-state documentation that still await that test. Keep such changes
uncommitted and record proposed documentation in the ignored owning backlog.
After the game test passes, prepare the exact tested implementation and its
durable documentation for review; commit them only with explicit user
authorization. Update the pin for future default builds and record the exact
tested ZIP's existing `Tmp/` path in the active context or ignored backlog when
it is selected for publication. Prefer a meaningful
`release-candidate-V<N>` output name when the producer supports choosing it,
but do not move or rebuild an existing candidate merely to rename it. Changing
the pin is metadata work; it must not silently
replace the tested package. Record the game-test result separately from build
provenance: the candidate manifest must keep the game build ID and the one
allowed final-ZIP digest from the original build under the shared
[hash rule](hash-usage-rule.md). Do not rebuild or regenerate its manifest
merely to update a tested-game version or other post-test metadata. Selection
does not promote the candidate into `artifacts/`: keep the exact ZIP and its
original manifest in `Tmp/` until that ZIP is either published or rejected.

## Output lifetime

Builds and installations used for experiments or feature development belong
under ignored `Tmp/<Mod>/`, with names such as `build-<identity>` or
`research-<subject>`. Keep the location of the next usable build in the active
task context or a large task's ignored backlog. A sequence of temporary
installations does not need a durable record of each replaced build, its hashes,
or its version. The installer may use hashes and backups while an installation
is in progress to detect a bad copy and recover a partial mutation. Once that
operation and any coupled settings edit have succeeded, those records are
disposable. Temporary outputs do not become release artifacts merely because
they were installed or tested in game.

Only published releases belong in `artifacts/installations/`. After the exact
ZIP is confirmed public on Nexus and `version.txt` has advanced to `N`, retain
that ZIP and its original release manifest under
`artifacts/installations/<Mod>-V<N>/`, for example
`artifacts/installations/Railgun-V5/`. The directory identifies the published
release version, not a timestamp or installation run. Refuse to reuse an
existing version directory for a different ZIP. Keep the selected ZIP there
for later identification or rollback. The ZIP is the retained payload; loose
`package/` copies are build intermediates and do not belong beside a complete
release archive. The installer validates the
manifest records against the ZIP and reads the container files from it when
the loose build intermediates have been removed. Historical
timestamped directories may be read for existing restore dependencies; migrate
or remove them only after their ownership and recovery value are established.

`artifacts/installations/` is the published rollback collection. Canonical
`<Mod>-V<N>` directories created under this rule are published and must be
treated as known-good rollback versions without searching for a separate chat,
log, or gameplay-test record. Historical noncanonical directories may predate
this contract; audit their owner and consumers rather than inferring publication
from location alone. Every unpublished build stays in `Tmp/<Mod>/`, including a
game-tested candidate selected for the next release. Promote the exact ZIP only
after publication is confirmed. Normal archive-identity, installation-integrity,
and supported-game version checks still apply when reinstalling a retained package.

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
`Tmp/<Mod>/installations/` while an installation or coupled settings edit is in
progress. These are short-lived recovery data for an interrupted operation, not a package to
choose for rollback after testing. Never put them in the versioned release
directory. Railgun's settings backup has the same limited purpose: preserve
the user's INI through the settings mutation, then discard the operation
record once it has completed. Keep failure evidence in `Tmp/` only while it is
needed to resolve that failure.

Check that the game is closed immediately before changing installed files,
backup existing targets, install from a validated manifest, read back every
installed file, and restore or stop with explicit recovery evidence on failure.
Follow the [shared installer contract](../tools/release-installation.md#release-installation)
and any owning mod's additional layout or settings contract.
