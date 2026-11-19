# Native INI parser library: integration and maintenance

The library provides two supported parsers. Choose by the configuration's shape:

| Source | Use case | Key lookup |
| --- | --- | --- |
| [BP_IniFileParser](Project/Content/BlueprintTextLibrary/BP_IniFileParser.uasset) | One flat set of settings; headers have no namespace meaning. | `Name` |
| [BP_SectionedIniFileParser](Project/Content/BlueprintTextLibrary/BP_SectionedIniFileParser.uasset) | Independent named groups or optional profiles, including empty section declarations. | `Section`, `Name`; `HasSection(Name)` checks presence. |

Both are uncooked Blueprint assets in the [source project](Project/IniParsers.uproject),
derived directly from UObject. Parsing, validation, cache management and typed
getters are implemented in their graphs. Each consumer receives its own native
copy through Unreal duplication. There is no shared runtime package or additional
native parser implementation to distribute. Both interfaces are maintained independently.

The interface and format below describe the flat source; the
[sectioned interface and grammar](#sectioned-parser) follow separately.

## Add to a mod project

The editor project needs the existing game API mirror
`/Script/Voyage.VoyageEditorBlueprintFunctionLibrary::LoadFileToArray(InPath)`.
This is the same file-reading dependency used by the existing
[runtime settings pattern](../../../../docs/runtime-configuration-patterns-knowledge.md).
Engine nodes handle the remaining operations. The editor mirror must retain the
game's reflected owner and signature; it is not a runtime DLL for the mod.

Use a prepared working project copy under `Tmp/<Mod>/`, following the
[workspace ownership contract](../../../blueprint-workspace.md#source-of-truth-and-git).
CopyAsset does not create that project copy. After review, accept only the
intended saved assets back into the maintained project.

```powershell
python -B tools/run_powershell.py tools/BlueprintText/Invoke-BlueprintWorkspace.ps1 `
  -Action CopyAsset -ProjectFile './Tmp/MyMod/authoring/Project/Voyage.uproject' `
  -WorkspaceRoot './Tmp/BlueprintText/my-mod-settings' `
  -SourcePath './tools/BlueprintText/Library/IniFileParser/Project/Content/BlueprintTextLibrary/BP_IniFileParser.uasset' `
  -Asset '/Game/Mods/MyMod/Settings/BP_IniFileParser'
```

`CopyAsset` launches Unreal, duplicates the saved asset, and exports the result in
a fresh process after removing its temporary source package. Warn before running
it. Each mod uses a unique package path; no original library package is required
at runtime. Existing destinations are rejected. Update a mod-owned copy through
normal Export/edit/Apply, or explicitly review replacement with a new library
version. Library fixes do not automatically overwrite consumer adaptations.

The native `.uasset` is the distributable authoring source. The source project
contains the minimal Voyage file-reader declaration needed for editing; its
EditorSupport plugin is not a runtime dependency to ship. The consumer supplies
its own matching game declarations. BPT JSON is exported into temporary workspaces
for editing and review, never retained as a second library source.

The library does not install, merge or package the player's INI. The mod owner
chooses reload policy and distributes the file through the usual release workflow.

## Blueprint interface

Construct the imported class with **Construct Object from Class**, supply an
Outer, and keep the resulting reference alive through its last use. Call `Load` once
with the chosen file path, then use the getters. Call `Reload` only when the
owner's policy requires a new snapshot.

For one synchronous function that constructs the parser, loads the file, reads
the getters and stores only effective values, wire the construction result
directly to those consumers. No persistent parser member or loaded flag is
needed solely for that sequence. Retain the parser in an owner reference when
later calls need its cache or `Reload`. `Outer` alone does not keep it alive;
latent or deferred uses require a reference that survives until those uses.
Parser lifetime and the owner's settings read policy are separate decisions.

| Function | Inputs and defaults | Result |
| --- | --- | --- |
| `Load` | `Path: String` | Stores the path and calls Reload. |
| `Reload` | None | Clears the cache, then reads the stored path. |
| `GetFloat` | `Name`, `Default = 0`, `Min = -FLT_MAX`, `Max = FLT_MAX` | Float32 value or Default. |
| `GetDouble` | `Name`, `Default = 0`, `Min = -DBL_MAX`, `Max = DBL_MAX` | Float64 value or Default. |
| `GetInt` | `Name`, `Default = 0`, `Min = -2147483648`, `Max = 2147483647` | Int32 value or Default. |
| `GetBool` | `Name`, `Default = false` | Boolean value or Default. |
| `GetString` | `Name`, `Default = ""` | Decoded string or Default. |

The getters are pure and do not read files or mutate the cache. Unconnected
optional input pins use the defaults above. With the [BPT argument contract](../../../blueprint-text.md#optional-call-arguments),
omit unneeded overrides at calls: for a positive capacity with fallback 200,
set only `Name`, `Default=200`, and `Min=1`; omit `Max` when the caller has no
upper-bound requirement. A required fixed bound must be passed even if it
equals the parser's default; do not copy defaults merely for completeness.
The public entry declarations now use `parameterDefaults`, including the
explicitly empty GetString fallback; Name and Section remain required.
Transition old literal calls
explicitly before removing their keys. Numeric bounds are inclusive and
apply to the represented result. Invalid/reversed bounds, overflow, nonfinite
input, fractional integers, and nonzero numbers that underflow to zero return
Default. Values are never clamped; Default itself is returned unchanged.
`FLT_MAX` is approximately `3.402823466e38`; `DBL_MAX` is approximately
`1.7976931348623157e308`. `GetDouble` uses double inputs, bounds and output,
without an intermediate float32 conversion. Both getters read the same raw
cached value; choose the required numeric type at the call site.

The path is used as supplied. The parser does not search for mods or infer their
installation directories. A caller can build an absolute path from
`ProjectContentDir()` and its known relative location, for example
`Paks/MyMod.ini`.

`Load` and `Reload` have no success/error output. A missing or unreadable file
leaves the cache empty, so getters return their supplied defaults. Old values
are not retained after a failed reload. Each instance owns its path and cache;
create separate instances for independent files or snapshots.

## Format

```ini
# Comments start with # outside a quoted string.
Speed = 12.5
Count = 4
Enabled = true
Label = "Example #1"
Escapes = "quote: \"; backslash: \\"
```

- Flat, case-sensitive keys; `Case` and `case` are distinct, including Unicode
  case variants. Leading/trailing whitespace around keys and values is trimmed.
- The first unquoted `=` separates key and value. Blank lines and lines without
  an assignment are ignored. Section headers have no namespace semantics:
  assignments following them still enter the same flat key space.
- `#` starts an unquoted comment. Semicolon comments are not part of this format.
- Numbers accept signs, decimals and exponents. `GetInt` accepts integral forms
  such as `3.0` and `5e1`, but rejects `1.5`. Hexadecimal and locale comma decimals
  are not accepted.
- Booleans accept only `true` and `false`, without regard to case.
- Strings require double quotes, may be empty, and escape only `\"` and `\\`.
  `#` and `=` inside a quoted string remain ordinary characters.
- The last assignment to a key wins. An invalid last value returns the getter's
  Default; a malformed quoted assignment with an identifiable key removes its
  earlier cached value. The cache stores raw strings without error-status fields.

The object holds only `FilePath` and a String-to-String map of raw values. Its
private key encoder preserves case-sensitive identity despite Unreal's reflected
String-map comparison. Helpers are private; consumers need only the seven methods
above. The lexical helper graphs preserve the existing shared settings grammar.

## Sectioned parser

Duplicate `BP_SectionedIniFileParser.uasset` with the same CopyAsset workflow,
using a distinct destination package such as
`/Game/Mods/MyMod/Settings/BP_SectionedIniFileParser`. The reader dependency,
object lifetime and snapshot ownership are the same as for the flat source.

| Function | Input pin names | Result |
| --- | --- | --- |
| `Load` | `Path` | Stores the path and calls Reload. |
| `Reload` | None | Clears both caches, then reads the stored path. |
| `HasSection` | `Name` | Pure Boolean presence query. |
| `GetFloat`, `GetDouble`, `GetInt` | `Section`, `Name`, `Default`, `Min`, `Max` | Same types, defaults, bounds and validation as the flat getters above. |
| `GetBool`, `GetString` | `Section`, `Name`, `Default` | Same types, defaults and validation as the flat getters above. |

All getters and HasSection are pure and perform no file I/O. `Section` and `Name`
are exact lookup strings; trimming happens when the file is parsed. Getter
defaults remain 0 for numbers, false for Boolean and empty string for String;
numeric bounds retain the full ranges listed above.

```ini
GlobalValue = 3
[BP_Module_Battery.BP_Module_Battery_C:Input] # optional comment
Enabled = true
Threshold = 20
[EmptySection]
```

- Sections are opaque names. Dots, colons, slashes, `#`, `=`, and Unicode inside
  brackets are ordinary characters. Trim whitespace around the enclosed name;
  preserve case and Unicode identity without normalization. Brackets and line
  breaks are reserved; an empty enclosed name is invalid.
- A trimmed line beginning with `[` is a header candidate, parsed before the
  ordinary comment/assignment scanner. After its closing `]`, only whitespace
  and an optional `#` comment are allowed. A malformed candidate disables the
  current assignment destination until the next valid header; its following
  assignments cannot leak into the preceding or global section. An ordinary
  malformed value does not switch sections.
- Every valid named header establishes presence, including an empty section.
  Repeating a header reopens that section. The last assignment wins only for
  the same section/key pair; malformed identifiable values remove the preceding
  value in that pair, following the flat lexical contract.
- Before the first header, assignments use `Section=""`. `HasSection("")`
  becomes true on the first lexically valid global assignment and stays true
  until Reload, even if a later malformed assignment removes its value. A raw
  value that fails typed conversion still counts as a lexical assignment.
- Presence is separate from value validity. Consumers validate their own mode
  or profile after HasSection succeeds. The parser does not interpret class,
  socket, mode or other consumer-specific syntax in a section name.
- Values retain the flat grammar, including quoted strings, `#` comments,
  escapes and numeric checks. Section/key addressing is collision-free and
  case-sensitive; punctuation does not act as an internal separator.
- Reload clears both section presence and raw values, including when the file
  is missing, unreadable or empty. Each instance retains an independent snapshot.

## Validation and maintenance

Run the focused integration test in a new temporary directory:

```powershell
./tools/BlueprintText/Test-IniParser.ps1 `
  -OutputRoot './Tmp/BlueprintText/test-ini-parser'
```

For the sectioned variant, run:

```powershell
./tools/BlueprintText/Test-SectionedIniParser.ps1 `
  -OutputRoot './Tmp/BlueprintText/test-sectioned-ini-parser'
```

The sectioned runner duplicates two native copies of each variant through CopyAsset,
executes the flat regression unchanged, reuses its typed cases in a named
namespace, and checks section presence, repeated/empty/malformed headers, opaque
punctuation, address collisions, case/Unicode identity, global assignments,
invalid values, cache reset, read counts and independent snapshots. It also
checks reflected public pins/defaults and repeats Apply after execution. Use a
new output directory for each full run; game and installation checks remain with
the consumer.

The flat runner creates a disposable project, supplies test editor dependencies, imports two
native copies with CopyAsset, and executes the saved Blueprint functions. The suite
covers numeric storage and ranges, exact double precision and subnormal values,
default call pins, strings/escapes, comments, case-sensitive keys, duplicate keys, empty/missing files, reload, independent
objects, and file-read counts. `-RunOnly -ProjectFile <prepared test project>`
executes the same cases against existing test copies. On failure, use the
reported step and diagnostic directory to investigate; after source changes,
rerun the full test to duplicate fresh source assets before accepting the result.

`Tests/IniParser/EditorSupport` is test-only. Its native code supplies a file-reader
mirror and a reflective test driver; it does not implement parsing or generate
Blueprint graphs. Do not copy that plugin into a mod or ship it. Validation is
against the local UE 5.8.2 editor and the maintained Voyage API signature. It is
not cooked-game or installation validation. Complete getter, numeric-edge and
section-grammar coverage belongs to the editor suites. Each consumer needs
game checks of its own settings lifecycle and value application.

Edit a disposable copy of the library project through the normal BPT workspace
workflow or Unreal Editor. Review and accept the changed `.uasset`, then rerun
the tests. BPT is temporary interchange; there is no regeneration step from a
parser-specific C++ or Python builder. The maintained library project must remain
free of a required BPT plugin and BPT node classes.
