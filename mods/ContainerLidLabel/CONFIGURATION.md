# Container Lid Label configuration

## Configuration

Runtime file: `Voyage/Content/Paks/ContainerLidLabel.ini`.
Authored default: [ContainerLidLabel.ini](Assets/ContainerLidLabel.ini).

```ini
# Large-container mass limit in kilograms.
LargeContainerMaxWeightKg=200
```

The shared parser trims whitespace, treats `#` outside a quoted string as a
comment, and matches keys case-sensitively. A semicolon is not a comment. The
last matching key wins; if that occurrence is invalid, the schema fallback of
200 kg wins. Values must be finite, at least 1 kg, and representable as a
float. Values that convert to zero or overflow float storage are invalid.

The generated Large class caches the result once while its class default object
remains loaded. Containers and persistent-load callbacks reuse that value;
class unload/reload may reread. Restart the game to apply INI changes. Lowering
the limit never clears, recreates or trims stored stacks.

For an older installation using `MaxWeightKg`, rename the key to
`LargeContainerMaxWeightKg`, preserving the value and comments. The old key is
not an alias. The producer prepares and validates the shipped INI but does not
install it. Use the shared merge tool to prepare a migrated file, then back up
and replace the installed file through a separately guarded installation step.
