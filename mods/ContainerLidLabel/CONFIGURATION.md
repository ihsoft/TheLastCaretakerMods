# Container Lid Label configuration

## Configuration

Runtime file: `Voyage/Content/Paks/ContainerLidLabel.ini`.
Authored default: [ContainerLidLabel.ini](Assets/ContainerLidLabel.ini).

```ini
# Large-container mass limit in kilograms.
LargeContainerMaxWeightKg=200
```

The flat parser trims whitespace, accepts standalone `;` and `#` comments, and
uses the last valid matching key. Inline comments are unsupported. Missing or
invalid input falls back to the embedded 200 kg default. Values must be finite,
positive and representable as a positive float.

The generated Large class caches the result once while its class default object
remains loaded. Containers and persistent-load callbacks reuse that value;
class unload/reload may reread. Restart the game to apply INI changes. Lowering
the limit never clears, recreates or trims stored stacks.

For an older installation using `MaxWeightKg`, rename the key to
`LargeContainerMaxWeightKg`, preserving the value and comments. The old key is
not an alias. The common container installer does not manage this external INI;
back it up, migrate it separately and verify the result.
