# Package additional files in a PAK

`New-VoyagePak.ps1` packages an explicit file-to-mount-path mapping with UnrealPak,
then checks the exact archive paths and extracted bytes. Use it for extra files
such as a content-plugin registry. It does not generate registries, cook assets,
assemble IoStore containers or install files.

Create a JSON manifest; relative sources resolve from its directory:

```json
{
  "schemaVersion": 1,
  "files": [
    {
      "source": "AssetRegistry.bin",
      "mountPath": "../../../Voyage/Mods/ExampleCatalogue/AssetRegistry.bin"
    }
  ]
}
```

```powershell
python -B tools/run_powershell.py tools/New-VoyagePak.ps1 `
  -Manifest Tmp/Example/build/extra-files.json `
  -OutputRoot Tmp/Example/build/extra-pak -EngineRoot '<Unreal installation>'
```

`EngineRoot` is the installation directory containing `Engine/`. Start outside
the restricted sandbox so UnrealPak can access its engine directory.
`OutputRoot` must be fresh, unlinked and below repository `Tmp/`; selected output
for a mod belongs under its `Tmp/<Mod>/` directory. Inputs must be existing files.
This bounded operation uses the shared process runner, with no editor/cook stage.

Every mount path starts with `../../../` and uses canonical forward-slash
segments. Traversal after that prefix, empty segments, response-file injection,
Windows device names, duplicate destinations (case-insensitive) and file/directory
collisions are rejected before output creation. Different directories may contain
the same basename. The tool determines the archive's common mount point through
UnrealPak and verifies every resulting full path against the request before
extracting anything. Callers supply paths, not a guessed common mount point or
raw UnrealPak arguments.

Success returns `status: passed`, `pakPath`, `fileCount`, `mountPoint` and
`reportPath`. The output contains `files.pak`, the normalized request, response
file, native logs and `result.json`. Successful readback scratch is removed;
failures retain diagnostics and must not be treated as completed archives.
Keep source files stable during the operation; a byte mismatch fails validation.

Pass `pakPath` as `CompanionPak` to the
[shared container operation](mod-setup.md#cook-and-container-stages) when that
container owns the extra files. This replaces its companion PAK, not merges
with another archive. The consumer owns all required extra-file entries and
their mount destinations.

For changes to request or verification logic, run
`python -B tools/AssetPackaging/test_packaging.py`. For native packaging changes,
also exercise the public command with files in different target directories and
check a consumer's final package. Existing archive inspection and byte readback
are part of the operation, not an additional mod-specific test.
