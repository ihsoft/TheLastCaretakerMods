# Optional sidecar-based autoloading for DML

Hi! Thank you for DML and for making asset-based mod loading accessible.

We've been experimenting with an opt-in autoload mechanism for Voyage and found
a path that may be useful for DML. Rather than discovering entries through the
Asset Registry, it uses the game's existing Blueprint-callable
`VoyageEditorBlueprintFunctionLibrary.GetAllFilesInDirectory` to find
`Content/Paks/*.autoload`, and `LoadFileToArray` to read optional settings.

A mod author can place an empty sidecar next to the mod's container files. The
filename supplies the default mod identity. Two independently optional settings
allow a different entry class and a choice of menu/gameplay activation:

```text
entryClass: /Game/Mods/MyMod/ModActor.ModActor_C
activateIn: gameplay
```

Our prototype runs entirely through cooked assets and functions already present
in the game; it requires no UE4SS or additional native runtime. It assumes the
game has mounted the containers at startup, so this is not hot-loading newly
copied containers. After discovery, it loads the entry Actor class and uses an
existing-instance check before spawning.

We have tested discovery, custom entry paths, phase selection and repeated
menu/gameplay transitions on Steam build 25056839 (UE5.8.1). Our separate
bootstrap also preserves the compatible DML menu console when DML is installed.
This is still a prototype, not a claim of universal compatibility: one HUD test
mod caused UI/exit problems while its autoload was enabled, and broader failure
handling and lifecycle coverage remain open.

Would you be interested in considering sidecar discovery as an optional DML
feature? Integrating it into DML's existing loading flow could be simpler than
maintaining a separate bootstrap. We would be happy to share the generator,
descriptor tests and the limitations we found. This is intended as a possible
contribution, not a request for you to support another loader or change DML's
existing manual workflow.
