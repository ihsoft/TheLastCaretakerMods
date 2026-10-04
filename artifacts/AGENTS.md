# Artifact registry rules

Root `../AGENTS.md` applies.

- Register every new top-level directory or distinct-purpose subtree in
  `README.md` before creating it. Do not create anonymous `misc`, `temp` or
  catch-all families here.
- Record only the owner, producer, consumer, retention reason and owning
  contract link. Keep schemas, commands and private layout details with the
  owning tool or mod.
- Generated hash, date, run and `<mod>` child directories inherit the owner role
  from their declared path pattern and manifest. Do not add a README for each
  cache key, installation, run or file. Add a row for a distinct purpose or
  contract, not for every new mod instance.
- Consult the declared owner before modifying, relocating, merging or cleaning
  a family, especially the private shared game-asset store.
- Logs, scratch data and build output belong in repository `Tmp/`. Do not infer
  retention value from generated status, age or size, and do not treat registry
  documentation as approval for perpetual retention.
- If an existing directory has no known owner or consumer, flag it for review.
  Do not invent a contract, delete it blindly, add automatic cleanup or build a
  new retention framework.
