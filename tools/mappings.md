# Mapping resolution and generation

## `Get-VoyageMappings.ps1`

This is the only normal entry point for obtaining mappings:

```powershell
.\tools\Get-VoyageMappings.ps1
```

It fingerprints the installed game, searches only the reviewed tracked
registry under `mappings/Voyage/`, runs `Test-VoyageMappings.ps1`, and returns
the exact mapping and manifest paths plus their identity. Consumers such as
`Get-VoyageAssetJson.ps1` and non-list `Inspect-VoyageAsset.ps1` call it
automatically when no explicit mapping is supplied. A new agent should neither
walk mapping directories nor inspect generation scripts on this successful
path.

If no reviewed entry matches, the resolver exits non-zero and explicitly says
not to regenerate automatically. Confirm that the fingerprint is genuinely
new, ask the user to start Voyage, and only then use the generator below.

## `New-VoyageMappings.ps1`

This is the exceptional refresh path after a confirmed game update:

```powershell
.\tools\New-VoyageMappings.ps1
```

Run this command outside the restricted sandbox from its first invocation.
The sandbox does not expose the running Voyage process to this tool: an
in-sandbox attempt reports that Voyage is not running even when it is open.
Check the process from the same outside-sandbox context used for generation.

Start Voyage first; exactly one running shipping process is a precondition.
The script never starts or stops the game. It returns a new candidate below
`Tmp/mappings/` only after validation. If it reports a missing or invalid
canonical dumper, run `Publish-JmapBinary.ps1` and retry; do not rebuild the
dumper source as a prerequisite.

Use `-InstallForUAssetGUI` to copy the validated result to
`%LOCALAPPDATA%\UAssetGUI\Mappings\Voyage-<build>.usmap`. Partial runs and logs
stay in `Tmp/` while needed for diagnosis.

If the dumper fails after loading a save, return to the main menu and retry
once before investigating its implementation. Retain the failure log while
diagnosing it. Promote only output that passes the normal mapping checks.

If the standalone mapping producer itself fails, use the
[toolchain-maintenance guide](toolchain-maintenance.md#mapping-dumper-fallback)
for dumper repair and the exceptional UE4SS fallback.

For a standalone check of an old saved input, use
`Get-VoyageBuildFingerprint.ps1` and compare its Steam build ID to the input's
provenance. Do not add this call before a producer or resolver that already
identifies the game during its run.
