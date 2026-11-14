# Project images and audio

Use [Invoke-UnrealResource.ps1](Invoke-UnrealResource.ps1) to inspect, export or
explicitly import project `Texture2D` and `SoundWave` assets. This is an authoring
tool, not a build stage. The saved Unreal asset owns the resource; an exported
PNG/WAV is a preview or editing copy.

## Prepare and select resources

Create a disposable project with the owning mod's project-copy command and
prepare its required plugins using its documented authoring entry point.
The tool expects a prepared `.uproject` below repository `Tmp/`; it enables the
engine Python and editor scripting plugins for its own process. It owns the
[Unreal queue](unreal-coordination.md). Launch outside the restricted sandbox.
It does not create a project, prepare game dependencies or write maintained
source. Shared [source acceptance](mod-setup.md#explicit-source-update-acceptance)
owns transferring reviewed assets back after checking concurrent source edits.

Create a temporary JSON request with exact package paths, without object suffixes:

```json
{
  "schemaVersion": 1,
  "resources": [
    {"asset": "/Game/Mods/MyMod/T_Icon", "type": "Texture2D"},
    {"asset": "/Game/Mods/MyMod/S_Shot", "type": "SoundWave"}
  ]
}
```

Run from the repository root through the shared PowerShell launcher:

```powershell
python -B tools/run_powershell.py tools/Invoke-UnrealResource.ps1 `
  -Action Export -ProjectFile Tmp/MyMod/authoring/Project/Voyage.uproject `
  -RequestPath Tmp/MyMod/resources.json -OutputRoot Tmp/MyMod/resource-preview
```

`Inspect` reports identities, types, image dimensions or sound duration/channel
count, and the settings listed below. `Export` additionally writes PNG/WAV files
below `OutputRoot/exports/`, mirroring package paths after `/Game/`. View or play
those files to review the current project content. Exported source media does
not reproduce Unreal material effects, UI layout, sound-class mixing or gameplay.
Both operations require existing resources of the requested types and leave
project content unchanged.

## Explicit import

For `-Action Import`, each entry additionally requires `source` and `mode`:

```json
{
  "schemaVersion": 1,
  "resources": [
    {
      "asset": "/Game/Mods/MyMod/T_Icon",
      "type": "Texture2D",
      "source": "chosen-icon.png",
      "mode": "replace",
      "settings": {"srgb": true, "compression_settings": "TC_EDITOR_ICON"}
    }
  ]
}
```

Source paths resolve relative to the request file. The supported inputs are PNG
images and nonempty PCM WAV audio. `create` requires an absent destination;
`replace` requires an existing asset of the exact type. Replacement retains the
package/object identity, so existing references keep their targets. It uses
Unreal's import factories with existing settings retained and explicitly
preserves the settings below; supplied overrides then apply. Creation uses
engine defaults for omitted settings. Review consumer-specific settings in the
editor when they fall outside this interface.

| Type | Optional `settings` fields |
| --- | --- |
| `Texture2D` | Boolean `srgb`, `never_stream`; enum member names for `compression_settings`, `lod_group`, `mip_gen_settings`, `filter`. |
| `SoundWave` | Nonnegative `volume`, positive `pitch`, Boolean `looping`, `sound_class_object` identifying an existing project SoundClass. |

For example, UI textures can select `TC_EDITOR_ICON`, `TEXTUREGROUP_UI`,
`TMGS_NO_MIPMAPS` and `TF_DEFAULT` where those choices suit their consumer.
The tool does not impose a universal UI or audio policy. Use Unreal Details
for settings not exposed by this bounded interface. Mesh imports have their own
[StaticMesh tool](static-mesh-assets.md); this tool does not import model scenes,
create materials, wire Blueprints or cook a release.

All destinations, types and settings are checked before the first import.
Unknown fields, duplicate destinations, redirects, incorrect create/replace
modes and linked working paths are rejected. Import checks the selected input's
dimensions or duration/channels, saves the requested assets, then reopens them
in a fresh Unreal process and compares the recorded state. The shared content
scope check rejects changes outside the requested packages. Review visuals or
audio and consumer behavior before accepting source changes; structural success
is not a game test.

## Results and failures

`OutputRoot` must be a fresh directory below repository `Tmp/`, separate from the
working project. Success returns `status: passed`, `action`, `reportPath` and
`outputRoot`. The report contains one record per requested resource, including
export paths when applicable. Logs and the normalized request remain in that
run for review. An import failure can leave partial changes in the disposable
project; preserve diagnostics and repair or recreate the copy before retrying.
No changes are automatically transferred to maintained source or installed.

## Tool maintenance checks

Run `python -B tools/UnrealResources/test_resources.py` for offline request
validation. Add `--with-unreal` outside the sandbox to check requested settings,
their preservation during replacement, and rejection of a mixed request before
any resource changes; export reads back the unchanged resources after rejection.
Input dimensions/duration and fresh-process readback remain checks of the import
operation rather than additional test assertions. These synthetic cases
protect parameter-dependent authoring behavior; they are not mod build checks
or snapshots of a particular mod's art.
