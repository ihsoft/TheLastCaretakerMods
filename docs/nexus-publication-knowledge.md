# Nexus publication workflow (observed UI; recheck when changed)

Use this guide for the Nexus page, upload form and visible publication state.
The [release workflow](release-and-installation-rule.md) owns release-number
selection, supported Updates, the exact tested ZIP, changelog, and post-publication
retention. Complete those prerequisites before uploading; a local build or
completed upload does not itself establish publication.

## Prepare the page and file

1. Determine whether the mod already has a Nexus page. A repository
   `version.txt` value of `0` does not answer that question. Compare the proposed
   release with the existing downloadable files through the release workflow,
   including parallel files for still-supported game Updates.
2. Create or update the page and upload the selected ZIP in the intended file
   category. Enter the target release number as a plain integer in the file-version
   field, such as `1`, not `1.0`. Use the prepared player-facing changelog.
3. Confirm the saved file's category, version and identity. Distinguish a
   prepared page, a saved file in a draft, and a public file listing. An enabled
   Publish control or an opened public-page tab is not evidence that the exact
   file is public.
4. Inspect the public listing and version after publication, or use the user's
   explicit report that he completed the final step. State which evidence was
   used. Then perform the release workflow's version, changelog and retention
   updates using the same ZIP.

The site's controls can change. Verify each transition through the current
visible state rather than treating this sequence as a fixed automation API.

## If the site behaves unexpectedly

- For images, use the visible upload action. Complete any crop dialog and wait
  for the resulting preview or gallery image; transfer completion alone is not
  the final state.
- If Save is disabled, check the form's required fields and saved section state.
  A displayed category may still require an explicit selection; disabled Save
  may also mean there are no pending changes.
- After navigation or upload, check the resulting page or section. If a control
  appears to do nothing, inspect its destination or state before retrying.

Nexus's [guidance for mod authors](https://help.nexusmods.com/article/136-best-practices-for-mod-authors)
also covers descriptions, images and file categories.
