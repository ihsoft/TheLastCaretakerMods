# Release archives and installation

Use the owning mod's public producer for a new candidate. This guide covers an
already-built IoStore triplet. The [shared release rule](../docs/release-and-installation-rule.md)
owns version identity, candidate retention, installation, and rollback.

## Read-only installation status

```powershell
.\tools\Get-VoyageInstallationStatus.ps1 -Summary
```

The compact result identifies the installed game build, observed game
processes, and top-level Paks files. Rerun without `-Summary` only when those
details are needed. `-InstallManifest <path>` checks exact installed targets
from a completed common installation manifest. Exit zero means the snapshot
was obtained; inspect the returned match states before drawing conclusions.
This read-only snapshot does not replace the installer's immediate process and
hash checks.

## Release installation

For a triplet built without a Vortex ZIP, call the shared archive producer:

```powershell
.\tools\New-VoyageVortexArchive.ps1 `
  -Container '.\Tmp\Example\build\Example_P.utoc' `
  -Archive '.\Tmp\Example\build\Example-candidate.zip'
```

Add `-AdditionalFile` for an INI or another payload file. Use
`-ContentPluginDescriptor` for a content plugin, or `-LogicMods` for a DML
actor. The producer chooses and verifies the matching ZIP layout; consult
[Vortex layouts](../docs/vortex-release-archive-knowledge.md) only to choose
the intended destination. The archive path must be new. No ZIP goes in Paks.

Create the release manifest for an already-built triplet and ZIP:

```powershell
.\tools\New-VoyageReleaseManifest.ps1 `
  -ReleaseRoot '.\Tmp\Example\V1-candidate' `
  -Mod 'Example' -Version 'V1-candidate' `
  -Container '.\Tmp\Example\V1-candidate\package\Example_P.utoc' `
  -Archive '.\Tmp\Example\V1-candidate\Example-V1-candidate.zip' `
  -SourcePath '.\mods\Example'
```

The producer writes `release-manifest.json` only after validating the archive
and payload. It refuses to replace an existing manifest. Use
`-ContentPluginDescriptor` when that exact file is part of the release; the
producer and installer support one content-only plugin descriptor. A dirty
development candidate needs explicit `-AllowDirtySource`.

Validate an already-built release without installing it:

```powershell
.\tools\Install-VoyageRelease.ps1 `
  -ReleaseManifest '.\Tmp\Example\V1-candidate\release-manifest.json' `
  -ValidateOnly
```

Inspect the planned destinations and any failure before installation. Remove
`-ValidateOnly` only for an authorized installation. The installer checks that
the game is closed immediately before mutation, uses temporary recovery data,
and reads back installed hashes. If it reports `recovery-failed`, stop and
preserve the reported files under `Tmp/` for diagnosis. `-ReleaseCandidate`
retains the selected exact ZIP and manifest under
`artifacts/installations/<Mod>-V<N>/`; an existing version directory is a
conflict, not an overwrite. Installed game directories contain only files the
game consumes.

For a failed gameplay test, reinstall the previous ready package. Use
`Restore-VoyageReleaseInstallation.ps1` only for an interrupted or coupled
installation whose temporary recovery files are still available:

```powershell
.\tools\Restore-VoyageReleaseInstallation.ps1 `
  -InstallManifest '<completed install-manifest.json>' `
  -ValidateOnly
```

Inspect the restore plan; remove `-ValidateOnly` only when restoration is
authorized. An unfinished transaction needs diagnosis before another mutation.
