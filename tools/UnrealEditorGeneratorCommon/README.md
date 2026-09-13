# Unreal editor generator common

Header-only helpers shared by UnrealEditor commandlet modules that construct
Blueprint graphs. These files are hand-written build tools, not game-derived
sources and not runtime mod payloads.

Each consuming module adds `Public` to its private include paths. Add only
engine-level reflected names or helpers here; game paths, reconstructed Voyage
contracts, and mod-specific tuning belong to their owning mod.

[`CharacterObservationGraphNames.h`](Public/CharacterObservationGraphNames.h)
owns the semantic Engine string-conversion input pins used by read-only
character observation and reporting graphs. It contains no Voyage or mod
identity.

`ActorScanGraphNames.h` owns Engine actor-enumeration macro pins and string
suffix comparison pins; consuming generators keep their game identities local.

Style contract:

- expose semantic node roles such as binary operands and select branches;
  never make consuming generators address raw `A`, `B`, or `bPickA` pins;
- share reflected identities and stable graph contracts, not incidental log or
  assertion wording;
- keep one-off diagnostics inline at their call sites;
- extend an existing semantic group before introducing another generic name.
