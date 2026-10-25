# Hash usage

Cryptographic hashes are not general-purpose provenance, compatibility, or
validation metadata. Do not add a hash merely because bytes are available to
hash or because an earlier report contained one. Before introducing a digest,
name the concrete failure it prevents and why a cheaper stable identity, direct
validation, or ordinary path/version check cannot prevent that failure.

## Allowed uses

A hash is allowed only when it directly implements one of these contracts:

- **One operation's copy integrity and rollback safety.** A mutating operation
  may compare source, staging, destination, backup, and restored bytes so that
  it never installs a bad copy, restores a damaged backup, or deletes a file it
  no longer owns. Keep these digests inside the operation and its temporary
  recovery evidence; discard them after successful cleanup.
- **Exact published-archive identity.** A release manifest may retain one
  digest for the final ZIP that was tested and published. This distinguishes
  that immutable player-facing file from a rebuilt archive with the same name.
  Do not also retain hashes for every file already contained in that ZIP.
- **A real time-of-check/time-of-use gate.** A producer may hash an input when
  it must prove that the same bytes cross a process or phase boundary after
  validation. The Railgun cook list's Core SHA-1 is such an explicit producer
  contract. Do not generalize one bounded gate into hashes for every input.
- **Content-addressed storage or deduplication.** A digest may be an internal
  cache key when byte identity actually determines lookup, reuse, or
  invalidation. Keep it internal unless a caller must supply that key.
- **A required external format field.** Preserve or calculate hashes that are
  part of Unreal, IoStore, GLB, package-manager lock files, or another consumed
  format. The format owns those fields; they are not repository provenance.
- **Exact-byte regression fixtures.** A test may use a digest when exact byte
  equality or intentional corruption is the behavior under test.
- **Bounded identifier encoding.** A digest may turn an arbitrary value into a
  fixed-length safe mutex, cache, or similar internal identifier when the
  consumer imposes a real length or character-set constraint. It must not be
  presented as validation evidence.

If a proposed use does not fit one of these cases, do not add it without first
raising the new need with the user and updating this rule.

## Forbidden uses

Do not use hashes to:

- identify or gate a game version; use the Steam build ID where an exact build
  matters, and the documented `Voyage.exe` file version or game Update where
  that broader compatibility level is sufficient;
- pin a public tool binary, tool source tree, generator commit, mapping file,
  cooked package, authored model, or game executable "just in case";
- require callers to preflight a tool that can validate its own actual inputs
  and fail with a useful error;
- decorate manifests, build reports, documentation, handoffs, or logs with
  per-file digests that no later decision consumes;
- treat identical hashes as gameplay, serialization, compatibility, or release
  validation;
- freeze replaceable source structure such as mesh topology, bounds, material
  counts, generated JSON, or extracted game data.

A Git commit ID identifies repository history; it is not a file-integrity
check. Mention one only when a Git operation or explicit handoff needs that
exact revision. Do not embed commit IDs in release manifests or ordinary build
reports as generic provenance.

Legacy names such as `Get-VoyageBuildFingerprint.ps1` use "fingerprint" for a
semantic installed-game identity: Steam build ID, `Voyage.exe` file version,
and the documented game Update. They do not authorize hashing the executable
or adding digests to compatibility metadata.

## Prefer direct contracts

Prefer semantic checks: required files and paths, archive entry layout, schema
and field validation, package reopen, container verification, tool exit status,
Steam build ID, game file version, and real-game behavior. Size can be a cheap
structural check but must not be dressed up as byte identity.

When removing an unjustified hash, remove the complete contract: producer
field, consumer check, documentation promise, report output, and regression
expectation. Retain legacy reading only when existing artifacts still need it;
new output must use the simpler schema.
