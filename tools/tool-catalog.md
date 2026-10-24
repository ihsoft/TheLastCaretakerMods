# Tool catalog

Find a public entry point by intent. Its guide has the arguments, results, and
failure handling.

| Intent | Public entry point | Guide |
| --- | --- | --- |
| Create a mod skeleton | `New-ModSkeleton.ps1` | [Mod setup](mod-setup.md) |
| Generate a settings header and INI | `New-VoyageModSettings.ps1` | [Mod setup](mod-setup.md) |
| Merge an existing user INI | `Merge-VoyageModIni.ps1` | [Mod setup](mod-setup.md) |
| Get the installed Steam build ID | `Get-VoyageBuildFingerprint.ps1` | [Mappings](mappings.md) |
| Check a mod's game-build pin | `Assert-VoyageModGameBuild.ps1` | [Release build](release-build.md) |
| Find or inspect cooked assets | `Inspect-VoyageAsset.ps1` | [Asset inspection](asset-inspection.md) |
| Summarize a Blueprint | `Get-VoyageAssetSummary.ps1` | [Asset inspection](asset-inspection.md) |
| Retrieve one asset JSON or package inventory | `Get-VoyageAssetJson.ps1` | [Asset inspection](asset-inspection.md) |
| Extract one cooked package | `Extract-VoyagePackage.ps1` | [Asset inspection](asset-inspection.md) |
| Export a stock model | `Export-VoyageModelGlb.ps1` | [Model export](model-export.md) |
| Cook authored GLB meshes as StaticMesh assets | `New-VoyageStaticMeshAssets.ps1` | [StaticMesh tools](static-mesh-assets.md) |
| Inspect or validate cooked StaticMesh assets | `Test-VoyageStaticMesh.ps1` | [StaticMesh tools](static-mesh-assets.md) |
| Export stock material samples | `Export-VoyageMaterialsGlb.ps1` | [Material export](material-export.md) |
| Render material previews | `Export-VoyageMaterialPreview.ps1` | [Material export](material-export.md) |
| Package one material | `Export-VoyageMaterialPack.ps1` | [Material export](material-export.md) |
| Package a material batch | `Export-VoyageMaterialBundle.ps1` | [Material export](material-export.md) |
| Inspect a GLB or owning model registry | `glb/inspect_glb.py` | [GLB tools](glb-tools.md) |
| Preview a GLB | `glb/preview_glb.py` | [GLB tools](glb-tools.md) |
| Preview imported material swatches | `glb/preview_material_glb.py` | [GLB tools](glb-tools.md) |
| Convert an authored Blender hierarchy to GLB | `glb/export_blender_glb.py` | [GLB tools](glb-tools.md) |
| Verify a material-library GLB | `glb/verify_material_library.py` | [GLB tools](glb-tools.md) |
| Resolve reviewed mappings | `Get-VoyageMappings.ps1` | [Mappings](mappings.md) |
| Generate mappings for a new game build | `New-VoyageMappings.ps1` | [Mappings](mappings.md) |
| Validate a mapping candidate | `Test-VoyageMappings.ps1` | [Mappings](mappings.md) |
| Diagnose mapping address discovery | `Find-VoyageUObjectArray.ps1` | [Toolchain maintenance](toolchain-maintenance.md) |
| Build a primary-asset registry | `New-VoyageAssetRegistry.ps1` | [Asset registry](asset-registry.md) |
| Build and package a mod candidate | [Mod producers](release-build.md#release-producers) | [Release build](release-build.md) |
| Verify an IoStore container | `Test-VoyageContainer.ps1` | [Release build](release-build.md) |
| Create a Vortex archive | `New-VoyageVortexArchive.ps1` | [Release installation](release-installation.md) |
| Create a release manifest | `New-VoyageReleaseManifest.ps1` | [Release installation](release-installation.md) |
| Inspect the installed game and mods | `Get-VoyageInstallationStatus.ps1` | [Release installation](release-installation.md) |
| Validate, install, or retain a release | `Install-VoyageRelease.ps1` | [Release installation](release-installation.md) |
| Restore an interrupted installation | `Restore-VoyageReleaseInstallation.ps1` | [Release installation](release-installation.md) |
| Inspect the shipping executable | `Invoke-VoyageExecutableInspector.ps1` | [Executable inspection](executable-inspection.md) |
| Diagnose one native instruction window | `Inspect-VoyageNativeMemberAccess.py` | [Executable inspection](executable-inspection.md) |
| Bound a failing native process | `Invoke-VoyageBoundedTool.ps1` | [Toolchain validation](toolchain-validation.md) |
| Publish or repair canonical binaries | `Publish-*.ps1` | [Toolchain maintenance](toolchain-maintenance.md) |
| Prepare UAssetAPI source for development | `Prepare-UAssetApiVoyageUe58.ps1` | [Toolchain maintenance](toolchain-maintenance.md) |
| Stress-inspect GUI asset parsing | `UAssetGUI.exe stress-open` | [Toolchain maintenance](toolchain-maintenance.md) |
| Run toolchain regressions | `Test-*.ps1` | [Toolchain validation](toolchain-validation.md) |
| Coordinate Git mutations | `commit-repository-changes.ps1` | [Git coordination](repository-git-coordination.md) |
