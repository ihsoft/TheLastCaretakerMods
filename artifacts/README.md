# Local analysis artifacts

Everything below this directory, except this README, is local scratch data and
must not be committed. Game assets, mappings, JSON exports, pseudocode, cooked
packages, reports generated directly from them, and installed-package backups
are tied to a particular executable and become misleading after a game update.

The repository keeps instead:

- repeatable extraction and inspection tools under `tools/`;
- stable architectural conclusions under `docs/`;
- failed approaches and their evidence under `docs/research-pitfalls.md`;
- active experiment state under the relevant backlog document.

Create a fresh build fingerprint before an investigation, place all generated
output below `artifacts/`, and promote only human-reviewed conclusions to
versioned documentation. Never treat a file in this directory as a source of
truth without checking its recorded game fingerprint.
