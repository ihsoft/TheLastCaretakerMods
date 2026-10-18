# Nexus publication workflow (observed UI; recheck when changed)

This guide covers the Nexus page after a candidate has passed the shared
[release gates](release-and-installation-rule.md). The site's controls can
change; use the current visible state to verify each transition. The release
rule owns version identity and the exact ZIP to publish.

## Prepare the page and file

1. Read the mod's `version.txt`. The target numbered release is its value plus
   one, including V1 when the file contains `0`. Determine separately whether
   the mod already has a Nexus page; `0` does not answer that question.
2. Use the exact selected, game-tested ZIP. Do not rebuild it merely for the
   upload or substitute another archive under the same version.
3. Create or update the mod page, then upload the ZIP as the main file. Enter
   the target release number as a plain integer in the Nexus file-version
   field, such as `1`, not `1.0`. Confirm the saved file's category, version,
   and identity before publication.
4. Distinguish a prepared page, a saved file in a draft, and a public page.
   A completed upload or enabled Publish control is not evidence that the mod
   is public. After publication, verify the public state or use the user's
   explicit report that he completed the final step. Only then advance
   `version.txt` under the release rule; state when publication was reported
   by the user rather than independently observed.

## If the site behaves unexpectedly

The following controls were observed during one publication and are not a
fixed automation interface:

- For images, use the visible upload action. Complete any crop dialog and wait
  for the resulting preview or gallery image; transfer completion alone is not
  the final state.
- If Save is disabled, check the form's required fields and the saved section
  state. A displayed category may still require an explicit selection; a
  disabled Save may also mean there are no pending changes.
- After a navigation or upload action, check the resulting page or section.
  If a control appears to do nothing, inspect its current destination or state
  before retrying.

Nexus's [guidance for mod authors](https://help.nexusmods.com/article/136-best-practices-for-mod-authors)
also covers clear descriptions, images, and categorizing the main file.
