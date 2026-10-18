# Game updates and executable versions (observed mapping)

The Last Caretaker names substantial game changes as Updates. A new Update can
spend time in beta while the previous Update remains the main branch. Players
can therefore use both at once, and a mod may need separate downloadable files
for each supported Update. A later Steam build within one Update is not by
itself evidence that the game's relevant behavior changed.

| Game Update | Observed channel | `Voyage.exe` numeric file version |
| --- | --- | --- |
| Update 5.5 | Main while Update 6 is in beta | `5.8.1.0` |
| Update 6 | Beta during the observed overlap | `5.8.2.0` |

This is an observed association, not a promise that every later build or
channel will retain the same version. Check the installed executable when the
Update identity matters. On Windows, the numeric `FileVersionRaw` of
`Voyage.exe` exposes this value; its text `FileVersion` may instead say
`UE5-CL-0`:

```powershell
(Get-Item -LiteralPath '<GameRoot>\Voyage.exe').VersionInfo.FileVersionRaw.ToString()
```

Do not infer an Update number merely from the Unreal Engine editor or parser
version.

Use the Update label and numeric executable version to describe player-facing
compatibility. Steam build ID remains an exact technical identity for reviewed
mappings, saved game-derived inputs, source provenance, and build evidence.
Those identities answer different questions: a build change can require new
mappings or fresh inputs without requiring a new compatibility claim for the
same Update. A change of Update deserves review of the mod's affected game
contracts and an in-game test before claiming support.
