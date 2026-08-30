# DonkLift editor generators

This is our hand-written, editor-only build-tools module. The commandlets are
invoked through `UnrealEditor-Cmd.exe`, so Unreal Header Tool must discover and
compile their `UCommandlet` classes inside an Unreal `Source` module. Moving
these files to the repository-level `tools/` directory would make the
commandlets unavailable to UnrealEditor.

Engine-level header-only names shared with future generator modules live in
`../../../../tools/UnrealEditorGeneratorCommon/Public`. The module adds that
directory as a private include path; game- and DonkLift-specific identities
remain beside these commandlets.

The module provides two commands:

- `GenerateDonkLiftMod` creates the helper actor, X/C input actions, and the
  replacement forklift keyboard mapping context.
- `GenerateDonkLiftInheritance` creates the temporary forklift parent/child,
  installs the helper, and implements localized standard X/C actions.

Only the generated cooked Blueprint assets are staged into the final IoStore
container. This module's DLL, C++ files, UHT output, and editor dependencies are
never shipped.

The implementation is hand-written, but it consumes version-bound contracts
reconstructed from the game: package paths, mirrored `/Script/Voyage` types,
function ownership, action identities, mappings, and enum values. After a game
fingerprint change, review the checklist in
`../../GAME_DERIVED_SOURCES.md` before regenerating assets.
