# Runtime INI configuration knowledge

This recipe separates file parsing, settings lifetime and application to game
objects. The recommended authoring path is the maintained Unreal project and
BPT. A parser's supported grammar does not establish a consumer's lifecycle or
the effect of writing a game field.

## Choose the read policy before writing a loader

Keep **reading the file**, **storing effective values**, and **applying them to
an instance** separate. Instance creation and save restoration can apply an
existing snapshot without reading the file again.

| Policy | Use when | Read and apply contract |
| --- | --- | --- |
| Session snapshot | Many passive instances share settings and live edits are not promised. | A session-lived owner reads once on first need. Existing, newly created and restored instances apply that snapshot. Document that file edits require restarting the game. |
| Entry reload | An explicit, infrequent entry or activation is the intended reload trigger. | One owner reads a fresh snapshot at entry and supplies all consumers before use. Read earlier only for behavior needed before entry. File edits take effect on the next entry. |

Choose an owner whose actual lifetime supports the promised policy. A CDO cache
can avoid one read per instance, but class unload/reload can cause another read;
it does not by itself establish a session-long snapshot. Neither policy needs
Tick-based file reads or independent reads by several helpers on the same event.
Changing a parser must not silently change the mod's read policy.

## Author one settings contract

Use the [BPT INI library](../tools/BlueprintText/Library/IniFileParser/integration.md)
for import, object lifetime, getter signatures and grammar. It supplies two
UObject sources:

- The flat parser addresses one case-sensitive key space.
- The sectioned parser addresses section/key pairs and distinguishes section
  presence from valid values, including empty sections and global assignments.

Import the selected source into the mod's own package path through BPT. The
saved asset and synchronized BPT then belong to that project. Both parsers use
the existing Voyage file-reading API; no separately installed runtime parser
package is needed. Their editor tests establish parser behavior, not another
consumer's game lifecycle.

For flat, schema-driven settings:

1. Keep the distributable default INI under the mod. It owns shipped values,
   player comments, order and formatting. Choose the installed path from the
   runtime reader's contract.
2. Keep the key, type, required runtime fallback, optional inclusive bounds,
   final storage and binding in the settings schema. Distributed values and
   runtime fallbacks have distinct roles.
3. Use the [INI preparation guide](../tools/mod-setup.md#runtime-ini-preparation)
   to validate changes to the schema or canonical INI. Ordinary builds copy the
   INI unchanged without repeating that validation. The validator does not
   generate the maintained Blueprint loader; consuming BPT graphs own getters,
   members and application.
4. On reload, assign every owned setting from the new snapshot or its defined
   fallback. BPT getters return that fallback for missing or invalid values;
   a separate reset-before-read chain is unnecessary.

Sectioned parser support does not by itself establish section support in a
flat schema validator. Define the consumer's profile and validation contract
through their supported interfaces before promising that behavior.

The library owns the full lexical and numeric contract. In particular, it
handles `#` comments outside quoted strings, typed fallbacks, inclusive bounds
and invalid last duplicate values. Passing an unprocessed INI line directly to
`IsNumeric` is not equivalent to using that parser.

Apply effective values through the actual owning setter or consumer path.
A field write may not update derived state, modifiers or presentation. For a
Blueprint override, establish its declaring function and lifecycle; use owned
references or lifecycle events instead of polling for the owner.

## Distribute the complete INI

The ZIP carries the complete validated INI. Players receive that file with the
mod on both first installation and updates. Do not promise automatic merging,
key migration or preservation of previous values/comments. Local candidate
installation follows the same full-file replacement contract, taking the INI
from the selected archive rather than current working-tree defaults.

The [release workflow](release-and-installation-rule.md#installation-and-rollback)
owns process checks, recovery and readback. A backup restores the prior INI if
installation fails; it is not input to a merge. Container and settings writes
remain within the owning install operation.

## Adding a setting

1. Identify the value's owner, reload trigger and player-facing units. Preserve
   an existing read policy unless the requested behavior changes it.
2. Update the canonical INI and its validation contract, then wire the consuming
   BPT getters and application. Include the complete updated INI in the package.
3. Run the owning public producer. Use its diagnostics for a failed check;
   a successful build establishes structure, not the gameplay effect.
4. Test the selected lifetime in game: shared snapshots reach new and restored
   instances; entry reloads take effect at the next entry. Check missing/invalid
   input and fallback behavior. Check exact packaged-INI installation separately
   from runtime reload behavior.

The resulting mod documentation records the accepted settings contract under
the [documentation workflow](repository-documentation-rule.md), rather than
copying the parser implementation or presenting an untested policy as working.
