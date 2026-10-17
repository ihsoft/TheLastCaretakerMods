# Shared Voyage vehicle and HUD principles

- Prefer lifecycle events and direct owned references over periodic object
  discovery. Repeated scans or rediscovery are a last-resort runtime design:
  explain why no event or ownership path is sufficient and notify the user
  before implementing one. This restriction does not apply to ordinary
  simulation ticks, line traces, or other gameplay updates that are not
  searching for ownership or lifecycle state.
- Treat input mapping, vehicle state, physical movement, provided actions, and
  HUD rendering as separate layers until evidence proves a direct connection.
- Enhanced Input mappings or action events alone do not imply that Voyage's
  standard hint UI will display an action. Locate the live action provider,
  registration path, filter, and rendered widget identity.
- Blueprint overrides bind to the exact declaring UFunction owner. A same-name
  function compiled against the wrong stand-in class is a different function,
  even when the graph compiles successfully.
- Editor-only mirrors must reproduce exact `/Script` module/class/function,
  property, struct, and enum identities needed by cooked assets. Convenient
  invented members are unsafe.
- A relocated original Blueprint is a snapshot of one game version. Never keep
  it in Git or assume a previously built replacement inherits later game
  changes automatically.
- Identify UI elements by stable UObject/action identity where possible, not
  child index, rendered key text, or localized label.
- Treat the shared-pattern document as a starting model for another vehicle,
  not proof that every Voyage vehicle uses the same fields or lifecycle.
