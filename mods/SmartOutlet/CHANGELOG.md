# Changelog

## Next

- Configure storage ports as input or output using INI rules; several ports on
  one storage actor can have independent modes, EnablePortAtPercent and
  DisablePortAtPercent thresholds. Settings are explained directly in the INI.
- Output opens at the upper threshold and closes at the lower threshold;
  input uses the inverse behavior. Unconfigured ports keep their normal behavior.
- Restore the saved port state between thresholds. Without a saved record,
  start open and apply the normal thresholds.
- Show the controlled port's state with a soft glow: green pulses every
  6 seconds, red every 2 seconds. The indicator can be disabled per rule.
- Include configurations for energy storage and fresh/salt water tanks, plus
  an optional runtime Tier 1 unlock for the existing salt water tank technology.
