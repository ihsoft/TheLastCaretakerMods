# INI parsers BPT library: integration and maintenance

The library provides two supported parsers. Choose by the configuration's shape:

| Source | Use case | Key lookup |
| --- | --- | --- |
| [BP_IniFileParser.json](BP_IniFileParser.json) | One flat set of settings; headers have no namespace meaning. | `Name` |
| [BP_SectionedIniFileParser.json](BP_SectionedIniFileParser.json) | Independent named groups or optional profiles, including empty section declarations. | `Section`, `Name`; `HasSection(Name)` checks presence. |

Both are schema-2 BlueprintText sources for Blueprints directly derived from
UObject. Parsing, validation, cache management and typed getters are implemented
in their graphs. Consumers import their own copy through the ordinary BPT
workspace. There is no parser asset generator, shared runtime package or additional
native parser to distribute. Both interfaces are maintained independently.

The interface and format below describe the flat source; the
[sectioned interface and grammar](#sectioned-parser) follow separately.

## Add to a mod project

The editor project needs the existing game API mirror
`/Script/Voyage.VoyageEditorBlueprintFunctionLibrary::LoadFileToArray(InPath)`.
This is the same file-reading dependency used by the existing
[runtime settings pattern](../../../../docs/runtime-configuration-patterns-knowledge.md).
Engine nodes handle the remaining operations. The editor mirror must retain the
game's reflected owner and signature; it is not a runtime DLL for the mod.

```powershell
$bpt = './tools/BlueprintText/Invoke-BlueprintWorkspace.ps1'
$work = './Tmp/BlueprintText/my-mod-settings'
& $bpt -Action AddSource -ProjectFile './mods/MyMod/Voyage.uproject' `
  -WorkspaceRoot $work `
  -SourcePath './tools/BlueprintText/Library/IniFileParser/BP_IniFileParser.json' `
  -Asset '/Game/Mods/MyMod/Settings/BP_IniFileParser'
& $bpt -Action Apply -WorkspaceRoot $work
```

`AddSource` is offline and copies the source with relocated internal references.
`Apply` prepares editor dependencies, compiles the asset, and verifies its export
in a fresh process. Warn before that engine action. Each mod uses a unique package
path. Existing destination assets are rejected by AddSource; update an existing
copy through the normal Export/edit/Apply workflow.

The library JSON is the distributable authoring input. Once imported, the mod's
Unreal project owns its copy; keep synchronized BPT with that asset and cook the
project through the mod's normal producer. The library does not install, merge or
package the player's INI. Its owner chooses the read policy and distributes the
file through the normal release/installation workflow.

## Blueprint interface

Construct the imported class with **Construct Object from Class**, supply an
Outer, and retain the resulting object reference in the owner. Call `Load` once
with the chosen file path, then use the getters. Call `Reload` only when the
owner's policy requires a new snapshot.

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

Import `BP_SectionedIniFileParser.json` with the same AddSource/Apply workflow,
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

The sectioned runner imports two copies of each variant through AddSource/Apply,
executes the flat regression unchanged, reuses its typed cases in a named
namespace, and checks section presence, repeated/empty/malformed headers, opaque
punctuation, address collisions, case/Unicode identity, global assignments,
invalid values, cache reset, read counts and independent snapshots. It also
checks reflected public pins/defaults and repeats Apply after execution. Use a
new output directory for each full run; game and installation checks remain with
the consumer.

The flat runner creates a disposable project, supplies test editor dependencies, imports two
relocated copies with AddSource/Apply, and executes the BPT functions. The suite
covers numeric storage and ranges, exact double precision and subnormal values,
default call pins, strings/escapes, comments, case-sensitive keys, duplicate keys, empty/missing files, reload, independent
objects, and file-read counts. `-RunOnly -ProjectFile <prepared test project>`
executes the same cases against existing test copies. On failure, use the
reported step and diagnostic directory to investigate; after source changes,
rerun the full test to import fresh copies before accepting the result.

`Tests/IniParser/EditorSupport` is test-only. Its native code supplies a file-reader
mirror and a reflective test driver; it does not implement parsing or generate
Blueprint graphs. Do not copy that plugin into a mod or ship it. Validation is
against the local UE 5.8.2 editor and the maintained Voyage API signature. It is
not cooked-game or installation validation. Runtime integration is established
for the flat mod-owned copy in [StableGyro](../../../../mods/StableGyro/ARCHITECTURE.md#settings-parser)
and the sectioned copy in [SmartOutlet](../../../../mods/SmartOutlet/AGENTS.md).
StableGyro uses `GetDouble` and `GetBool`; complete getter, numeric-edge and
section-grammar coverage belongs to the editor suites. Each new consumer still
needs game checks of its own settings lifecycle and value application.

Edit the BPT source through the normal workspace workflow and rerun the tests.
There is no regeneration step from a parser-specific C++ or Python builder.
