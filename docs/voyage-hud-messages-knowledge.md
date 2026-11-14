# Stock HUD message cards

Voyage's stock message list can display temporary text from a mod Blueprint,
including development diagnostics, without changing inventory, replacing the
HUD, overriding stock assets or shipping a native DLL. Reflection and stock
Blueprint inspection on Steam build `25982825` establish the interface below;
in-game observation confirms arbitrary text cards and their expiry, including
a 20-second lifetime. This build identifies the evidence, not an applicability
requirement. Recheck affected contracts when current evidence disagrees.

## Sending a text card

Use the live `VoyagePlayerController` and its callable
`AddMessageBySlot(MessageContent, int32 Slot, float ThrottleTime) -> bool`.
Read `LootMessageListIndex` for the slot and `LootMessageSubClass` for the
content's widget class; do not hardcode a slot number. The inspected stock
`BP_VoyagePlayerController` selects `BP_LootMessage_C` and has
`LootMessageDuration = 4.0`. A caller can supply a different duration.

For a plain text card, populate these members of `/Script/Voyage.MessageContent`:

| Member | Value and purpose |
| --- | --- |
| `UniqueContentKey` | Nonempty `FString` identifying the message; use a mod-prefixed key to avoid collisions with other callers. |
| `Header`, `Body` | `FText` containing the title and details. |
| `duration`, `Delay` | Float lifetime and delay; an immediate card uses `Delay = 0`. |
| `bShowImage` | `false` for text-only content. |
| `SubClass` | The controller's live `LootMessageSubClass`. |

`BP_LootMessage` contains header and body text widgets, but no footer widget.
Setting `MessageContent.Footer` therefore does not supply another visible line
in this layout. Put details in `Body` and keep them short: the observed layout
fits a header and two body lines; five body lines overflowed its background.
Text length, wrapping and screen layout still need visual acceptance for the
intended content. This mechanism does not imply that persistent diagnostics
belong in a released mod.

## Acceptance, readiness and throttling

The native call rejects an empty key. It returns false when the HUD, game widget
or requested message-list widget is unavailable, and does not queue delivery.
A true result means that the list returned a message widget; it does not prove
that all text fits or that the player read it.

With `ThrottleTime > 0`, throttling is keyed by `UniqueContentKey`. The native
path records an accepted attempt's timestamp **before** checking HUD readiness.
An early failed attempt can consequently throttle a later attempt even though
no card appeared. For caller-managed delivery, a zero throttle plus bounded
event-driven retries avoids that interaction. Mark the event delivered only
after a true result; discard it when it becomes irrelevant and limit repeats.
Do not turn a missing HUD into an unbounded notification loop or treat a card
attempt as guaranteed failure reporting during early load.

Stock `BP_Notification_Area` uses both `AddMessageBySlot` and
`ExpireMessageWithKey`, providing an inspection target for explicit expiry.
Automatic grouping behavior and the exact inventory-removal callers are not
established by the text-card contract.

## Editor declarations and reinspection

An editor-only declaration used to construct a `MessageContent` value must
match the complete native struct, including field order and types. A mirror
containing only the visible text fields is insufficient for Blueprint struct
expressions. The inspected contract has 30 fields and occupies 360 bytes;
matching the size alone is not proof that the fields match. The table above is
a usage subset, not a struct declaration. UObject declarations exposing selected
properties do not justify a partial value-struct mirror.

When implementing this interface or investigating a mismatch, use the public
[asset inspection tools](../tools/asset-inspection.md) to inspect the current
`MessageContent` mapping and these stock packages:

- `/Game/Game/BP_VoyagePlayerController`: selected class and duration defaults.
- `/Game/UI/Game/Message/BP_LootMessage`: displayed fields and widget layout.
- `/Game/UI/Game/Message/BP_MainInGameMessageList`: message-list class.
- `/Game/Blueprints/Interactives/Notificaton_Areas/BP_Notification_Area`:
  stock sending and explicit expiry calls.

For return and throttle behavior, follow `VoyagePlayerController.AddMessageBySlot`
from its reflected exec thunk to the implementation with the public
[executable inspection tools](../tools/executable-inspection.md). Reflection
construction helpers are not the function body. Editor declarations support
compilation; the packaged Blueprint calls the game's native implementation.
