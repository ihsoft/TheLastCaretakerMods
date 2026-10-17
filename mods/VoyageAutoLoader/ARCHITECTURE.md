# VoyageAutoLoader architecture

This asset-only bootstrap discovers `.autoload` sidecars beside already mounted
mod containers. Its generated menu GameMode and controller own a worker that
reads sidecars through Voyage's Blueprint-callable file functions, selects the
requested menu or gameplay phase, checks for an existing entry actor, and
attempts one spawn. The entry mod owns its later readiness and initialization.
The bootstrap ships without a native DLL, UE4SS, or DML payload.

[README.md](README.md) describes the sidecar format, generated packages, build
path, and runtime limits. [Mod rules](MOD-RULES.md) retain the loader's specific
constraints. [DML-PROPOSAL.md](DML-PROPOSAL.md) is an external proposal, not a
runtime dependency.
