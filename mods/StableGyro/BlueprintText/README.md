# StableGyro Blueprint text

These JSON files are readable text representations of the mod's three gameplay
Blueprints. They make changes visible in Git and let an editing tool work with
Blueprint graphs without manipulating binary Unreal assets directly.

The saved Unreal assets in `../Content/` are authoritative. BPT is a bidirectional
editing format: export reads an asset, and Apply writes proposed JSON changes
back to it. The JSON and its asset must be synchronized when committed. The game
does not read these files, and a release build does not recreate assets from them.

| File | Purpose | Unreal package |
| --- | --- | --- |
| [ModActor.json](ModActor.json) | Altitude and horizontal stabilization after physics. | `/Game/Mods/StableGyro/ModActor` |
| [PitchInputActor.json](PitchInputActor.json) | Pitch input handling before physics. | `/Game/Mods/StableGyro/PitchInputActor` |
| [BP_GyroCopter_StablePilot.json](BP_GyroCopter_StablePilot.json) | The mod's child of the stock Gyro, including its mooring-port and action integration. | `/Game/Mods/StableGyroCtrl/BP_GyroCopter_StablePilot` |

Each file's `asset` field identifies its Unreal package. This directory does not
need to repeat the package's `Mods/...` hierarchy; moving a JSON file does not
rename the corresponding asset. Meshes, materials, input assets and the item
DataAsset remain in the Unreal project's Content directory and are not described
by these three exports.

For editing and graphical previews, see the [shared BPT workflow](../../../tools/blueprint-workspace.md).
