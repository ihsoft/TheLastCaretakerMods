# PalletContainerSnap stable rules

Repository rules in [../../AGENTS.md](../../AGENTS.md) also apply.

- `Build-PalletContainerSnap.ps1` is the only public producer. It prepares a
  candidate below `Tmp/PalletContainerSnap/`; it never installs or launches the
  game.

- Stock override scopes must emit the `_P` container stem. The legacy
  unsuffixed `StockLargeRoute` scope is not a supported new build; use the
  byte-identical priority experiment instead. `OwnedCanary` remains additive.

- The four authored slot transforms and their source fingerprint live in
  `Contracts/pallet-container-snap-contract.json`. Do not tune them to pass a
  runtime check without new model/origin evidence and an explicit decision.

- Keep the stock Default generator's `GenerateLocation` and validation path.
  The owned `SetupLocation` override calls its exact parent first and changes
  candidate fields only for the exact stock pallet class.
