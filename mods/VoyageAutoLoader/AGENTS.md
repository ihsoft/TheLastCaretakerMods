# Voyage Auto Loader

- Empty sidecars default to gameplay-only activation. entryClass and activateIn
  are independently optional key:value fields; their absence preserves each
  default independently. Do not reintroduce JSON or a required version field.
  Never treat every non-playing/transitional world as the main menu.
- Entry mods own their Pawn/control readiness checks; the loader must not
  require a Pawn. Menu/phase changes still require their runtime test.
- Physical container stem and virtual entry class are separate identities.
- No DML payload redistribution, native runtime DLL, UE4SS, external runtime
  preprocessing, or changes to the user's DML enable-list/save.
- Our bootstrap container has priority; DML is optional and never a parent dependency.
  Preserve DML menu availability; gameplay autoload does not wait for DML.
  Use existing-actor guards, not a claimed strict activation order. Validate
  repeated travel separately: DML can recreate itself independently. Do not
  claim arbitrary async mod initialization has completed when spawning returns.
- Preserve the exact game-validated C1 no-DML checkpoint and its test14 predecessor.
  Never launch Voyage without permission or install while it is running.
- Generated Content, build/cook output and package evidence remain ignored.
