# Changelog

## Next

- Aim at a supported storage socket and press V (Interact 3) to select smart
  input, smart output or normal. Each port remembers its own mode. The HUD hint
  shows the next/current modes and their enable/disable percentages.
- Configure available modes and fill thresholds per storage class in the INI.
  Missing modes are skipped; unconfigured storage has no extra action.
- Smart output opens at the high threshold and closes at the low threshold;
  smart input does the reverse. Preserve the enabled state between thresholds
  across saves; missing state starts enabled before thresholds are applied.
- Show enabled ports with a slow green pulse and disabled ports with a faster
  red pulse. Glow can be disabled per mode; normal restores the stock appearance.
- Supply battery output at 80/20 and fresh/salt water tank input at 20/100,
  plus optional Tier 1 research access for the salt water tank and its ingredients.
