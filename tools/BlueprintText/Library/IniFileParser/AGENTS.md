# INI parser library index

The library supplies mod-owned UObject parsers through BPT. Choose the source
by the configuration's shape:

- [Flat parser](BP_IniFileParser.json): one case-sensitive key space; section
  headers do not create namespaces.
- [Sectioned parser](BP_SectionedIniFileParser.json): section/key pairs and
  presence checks for optional or empty sections.
- [Integration](integration.md#add-to-a-mod-project): import the chosen source,
  prepare its game API dependency and retain the consumer's saved asset.
- [Interface and flat grammar](integration.md#blueprint-interface): object
  lifetime, getters, fallbacks, bounds and value syntax.
- [Sectioned grammar](integration.md#sectioned-parser): section identity,
  presence and malformed-header behavior.
- [Maintenance checks](integration.md#validation-and-maintenance): validate a
  parser change and distinguish editor coverage from consumer game checks.
- [Settings lifecycle](../../../../docs/runtime-configuration-patterns-knowledge.md):
  choose reload policy and preserve player INI values during installation.
