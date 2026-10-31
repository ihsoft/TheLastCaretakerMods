# VoyageItem patching

Use the [public guide](../voyage-item-patch.md) and
`../Update-VoyageItemAsset.ps1` for normal project updates. Producers use
`../Test-VoyageItemPatch.ps1` for semantic validation. Native preparation belongs
to the common Unreal editor-support tool, not per-mod source-copy steps.

For maintenance, `item_patch.py` owns the bounded profile conversion/comparison;
`Profile/` owns reflected definitions compiled into `/Script/Voyage`;
`Source/VoyageItemPatchEditor/` owns editor reconstruction; `Update-Item.ps1`
owns current-game input resolution and the asset transaction. Mod-specific stubs
and patch recipes stay with their owners. Use the offline and ready-project
checks described in the guide; keep engine calls in the shared queue.
