# Nexus publication workflow (observed UI; recheck when changed)

This guide covers the Nexus page and the shared
[release gates](release-and-installation-rule.md). For a requested publication,
complete the version check in step 1 before selecting a numbered release;
complete the upload steps after the candidate passes those gates. The site's
controls can change; use the current visible state to verify each transition.
The release rule owns version identity and the exact ZIP to publish.

## Prepare the page and file

1. Read the mod's `version.txt`. The target numbered release is its value plus
   one, including V1 when the file contains `0`. Determine separately whether
   the mod already has a Nexus page; `0` does not answer that question. Compare
   the proposed number with files already published on that page. If the
   repository value is stale or the proposed number is occupied, resolve the
   target before publishing a numbered candidate. Check
   which game Updates the existing downloadable files support. When main and
   beta Updates coexist, decide whether the new file is parallel to an older
   supported file or replaces it before opening the upload form.
2. Use the exact selected, game-tested ZIP. Do not rebuild it merely for the
   upload or substitute another archive under the same version. If an explicit
   decision changed the target number after the game test, keep the original
   ZIP and build manifest as evidence, and identify both the tested candidate
   label and the actual release number. The retained directory must name the
   actual release; inspect version-bearing ZIP content before reusing its bytes.
3. Require the canonical archive name `<Mod>-V<N>.zip`. Read the mod's
   `CHANGELOG.md`, review the `Next` section against the exact selected ZIP, and
   use it as the Nexus file changelog. It must describe the net player-visible
   difference from the latest published release, not the commits or discarded
   candidates that led there. If it is empty or materially incomplete, stop
   instead of reconstructing or inventing release notes. The changelog is
   publication input and is not included in the ZIP.
4. Create or update the mod page, then upload the ZIP as the main file. Enter
   the target release number as a plain integer in the Nexus file-version
   field, such as `1`, not `1.0`. Confirm the saved file's category, version,
   and identity before publication.
5. Distinguish a prepared page, a saved file in a draft, and a public page.
   A completed upload or enabled Publish control is not evidence that the mod
   is public. Opening a public-page tab alone is not verification: inspect the
   visible file listing and version. After publication, verify the public state
   or use the user's explicit report that he completed the final step. Only
   then advance `version.txt`, finalize the matching changelog section under
   the release rule, and retain that same ZIP through the shared installer's
   `-RetainPublishedRelease` mode. This is the first point at which the package
   belongs in `artifacts/installations/`. State when publication was reported
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
