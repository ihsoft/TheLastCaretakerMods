# Large-container lid label architecture

## Completed scope

The user accepted the installed `ContainerLidLabel v3-test` on 2026-10-13.
The large FortiHold container shows its name below the numeric count together
with its selected icons. Icons flank the count; a sole icon remains on its
native left or right side. The stock side display retains its own text/icon
policy. Count, load bar, inventory and lid geometry are inherited unchanged.

Implementation, accepted artifact identity and recovery references are owned by
[ContainerLidLabel](../mods/ContainerLidLabel/README.md) and its
[completed backlog](../mods/ContainerLidLabel/BACKLOG.md). This investigation
is closed; additional container features require their own scope.

## Version-bound contracts

These findings were inspected on Steam build `25880952`, Unreal Engine 5.8.
A changed fingerprint requires revalidation before reusing the reconstructed
native mirrors, Blueprint contracts or relocated stock parent.

Target: `/Game/Blueprints/Interactives/Containers/BP_Inventory_PlayerBox_Large`.
Its stock parent is `BP_Inventory_PlayerBox`; the stock large child has no own
functions and carries inherited component overrides. Its item identity remains
`DA_Item_PlayerBoxLarge`.

| Display | Component | Attachment | Data source |
| --- | --- | --- | --- |
| Load bar | `ResourceMeter` | `PlayerBox_Lid` | `GetPercentWeightUsed` to material `ProgressLevel` |
| Numeric count | `TextRender` | `PlayerBox_Lid` | `GetTotalNumItems` to `K2_SetText` |
| Stock side name | `VoyageContainerTextWidget` | body `StaticMesh` | inventory `Name` to `BPI_PlayerBoxWidget.SetText` |

The inventory field `VoyageInventoryWeightLimited` is declared by
`BP_Inventory_Container_Base_WeightLimited`, not by `BP_Inventory_PlayerBox`.
Its native component inherits `VoyageBaseInventoryComponent`, which declares
`Name`. Exact declaring owners matter when generating reflected references.

## Text and icon lifecycle

The stock side component uses `/Game/UI/Game/Inventory/BP_PlayerBox_Text`.
`UpdateTextWidget` gets that particular component's widget and supplies the
inventory name. `InitTextWidget` supplies size selection and font. The widget
uses the stock VoyageRoboto font, centered text, clipping and ellipsis.
The actor serializes `MaxNameLength=25`; enforcement was not separately traced.

BeginPlay binds `OnInventoryNameUpdated`; that event and `PostLoadUpdateName`
reach `UpdateTextWidget`. Construction also invokes text initialization and
updates. Our duplicate participates through these existing overrides rather
than polling for name changes. The numeric renderer has its own inventory
change binding, so reusing it for the name would fight the stock count update.

The stock parent hides only its own `VoyageContainerTextWidget` when icons are
visible. Our `LidLabelWidget` is a separate instance with independent visibility:
its template is visible, and our icon hook never hides it. The common widget
asset is not modified or shipped by this mod.

Stock side icon materials are dynamic instances created by the parent.
The lid planes share those same instances and copy the corresponding side
visibility after the original `UpdateIconMaterial` call. Async stock material
updates therefore reach the same objects; the mod adds no loader or scans.

## Implementation boundary

The producer freshly extracts the stock large Blueprint and relocates it to
`/Game/Mods/ContainerLidLabel/StockOrigin/BP_Inventory_PlayerBox_Large`.
The equal-length relocation replaces exactly two package-name occurrences,
preserves `.uasset` length and leaves `.uexp` bytes identical. A generated
child occupies the original large Blueprint identity and inherits that snapshot.

The runtime container contains exactly these two packages. Its `_P` suffix
expresses stock override priority. Editor-only native mirrors, parent/widget
placeholders and the shared stock widget are excluded from the package set.
Another mod overriding the same original large Blueprint can conflict with it.

The generated child adds one widget and two icon planes attached to inherited
`PlayerBox_Lid`. It overrides only `InitTextWidget`, `UpdateTextWidget` and
`UpdateIconMaterial`, calling the exact stock parent functions first. The last
hook changes materials and visibility only on the new icon planes.

Accepted lid-local layout, in centimetres:

| Element | Position | Size/settings |
| --- | --- | --- |
| Lower name | `(0, 66.4, 13.73)` | draw size `200x110`, scale `0.12`, VoyageRoboto Bold 12, wrapping disabled on this instance |
| Left icon | `(-17, 62.27709, 13.73)` | uniform plane scale `0.05`, 5 cm square |
| Right icon | `(17, 62.27709, 13.73)` | uniform plane scale `0.05`, 5 cm square |

The name's rotation is `(Pitch=90, Yaw=90, Roll=0)`. Icon planes retain flat
rotation. Icons have fixed positions, including single-icon selection; there
are no runtime recentering branches. The stock count is near lid Y=62.27709
and meter near Y=51.802948. Screenshot measurements selected the initial
placement; the user subsequently accepted the actual in-game appearance.

## Research pitfalls and validation boundary

Moving only the side widget's transform is insufficient: its SCS attachment
remains the body while the opening timeline rotates the lid. Local UE 5.8
`SCS_Node.cpp` shows that inherited template selection does not replace the SCS
parent used by `SetupAttachment`. Adding a template `AttachParent` value alone
is not an established reparenting mechanism. A lid-owned duplicate avoids this
and preserves the side display.

Compile, cook, exact package inventory and independent CUE4Parse readback
verified the generated ownership, calls and component settings. Guarded
installation verified all installed hashes and retained the exact predecessor.
The user confirmed icon layout/single-icon side retention and then accepted
simultaneous name-and-icon display. This is game validation of the requested
behavior, not a claim of exhaustive testing of save migration, all name lengths,
arbitrary digit counts, or every mod combination.

The installed version identity remains `v3-test`: finalizing source and records
does not relabel or regenerate the game-tested bytes. Durable installation
provenance and recovery details belong to the mod's documentation.
