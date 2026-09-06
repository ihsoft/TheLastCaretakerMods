# HarpoonCannon rules

Repository rules in `../../AGENTS.md` also apply.

## Ownership and research mode

- This task owns gameplay research, `mods/HarpoonCannon/**` and
  `../../docs/harpoon-cannon-backlog.md`. Model geometry and its backlog belong
  to the separate model task; coordinate before any generator/model/build work.
- Do not modify DonkLift, another mod's assets or installed containers. Coordinate
  before editing shared architecture documents in an active shared worktree.
- Implementation scope is checkpoint-specific; see the active backlog. The
  renewed autoload experiment owns only `AutoloadProbe/`, not the historical
  UE5.7 shell or model generator. Installation requires separate authorization.
- Resume from the active section of the Harpoon backlog and `RESEARCH.md`.
  Chronological install claims in old experiments are historical, not gates or
  current state. Use the public installation-status tool for current state.
- Use the public tools selected through `../../tools/README.md`. Keep failed
  tool cases and any fallback method in the iteration report; common reusable
  gaps belong in the shared toolchain backlog, not in gameplay rules.

## Version and authoring gates

- The code is an editor-only asset generator plus experimental probes. Never
  ship its native binaries: the game supplies the actual `/Script/Voyage` types.
- Fingerprint first and resolve reviewed mappings through the public resolver.
  Before generation/cook/package, renew every affected entry in
  `GAME_DERIVED_SOURCES.md`; a new mapping alone does not renew native mirrors.
- Reproduce exact native declaring owners, default-subobject identities,
  inherited property ordering and referenced types. Never invent native members
  or infer successful runtime overrides from a compile/cook result.
- Generated Content, extracted snapshots, logs, cooked output, inventories and
  backups remain ignored artifacts. Follow the root reviewed-mapping exception.
- Future installation must use the current documented manifest/process/backup/
  hash-readback gates. A historical closed-game message is not a process gate.

## Preserved gameplay boundaries

- Placement/module attachment, possession, input, camera, optical trace,
  classification and HUD remain separate layers.
- The historical construction-safe owner is a `VoyageModuleActor` shell with
  exact named native default subobjects and custom-module ItemAsset alignment.
  Revalidate it on the installed build before claiming current compatibility.
- The disposable item adapter may target only the verified Cyclone leaf
  `/Game/Blueprints/Modules/Generators/BP_Module_WindTurbine_Medium_New`.
  Never restore the rejected small-turbine/Whisper parent-path override.
- Preserve the base mesh's simple collision. It supplied unfinished-fabricator
  acquisition for stock Q cancellation. The temporary fabricator clones meshes,
  not the shell's independent box. More mount-overlap changes were a no-op;
  collision is not a remedy for the separate preview-rendering defect.
- Two direct reconstructed InteractiveInterface variants crashed at completion.
  Do not iterate guessed interface signatures or combine new entry bytecode
  with the same unproven class contract.
- Historical shell-spawn tests produced no usable interaction; their latent
  continuation remains unresolved. Current standalone HC01/HC02 validate Drone
  survival and stock action hints, not entry/exit. Do not call the old
  missing-barrel observation proof of timed self-destruction.
- Do not inherit from another mod's replacement of the canonical Forklift path.
  Any stock relocation is version-bound and requires fresh isolated extraction.
- Full HUD selection through `GetHUDOverrideWidget` remains unbound until its
  exact declaring native identity and lifecycle are established for this mod.
- Keep 360-degree yaw, pitch -50/+10, first-person approximately 5x optics and
  distance from character to optical hit point. Re-inventory all shark identities
  after updates; use stable gameplay data, not localized/display/class-name text.
  Other wildlife can later extend the classifier without rewriting trace/HUD.

## Validation

- Compile, generated graph validation, cooked inspection and container verification
  are separate lower gates, never substitutes for real-game gameplay validation.
- Change one architectural variable per runtime experiment with recoverable
  evidence. After one or two no-ops revisit ownership or improve the discriminator.
- Preserve safe construction first; then validate entry/exit, camera, aim,
  HUD/trace and persistence in isolated checkpoints. No firing is implemented.
