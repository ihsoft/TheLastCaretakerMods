# Error handling and defined outcomes

- Every in-scope input or requested action must have a defined outcome. Never
  silently return from an error check and leave promised behavior missing or
  the mod in an indeterminate, partially applied state. A final mod must work
  or fail unmistakably; a warning that accompanies broken behavior is not a
  substitute for a working implementation.
- Error reporting is a development/investigation aid, not a production feature
  in its own right. Do not add permanent error panels, notification frameworks,
  polling or diagnostic packages, or block an otherwise scoped change on
  developing them. Introduce targeted diagnostics when investigating a concrete
  problem, then remove them from the final mod. Build/install tools and agent
  handoffs must still report their failures and actual outcomes.
- Prefer proven stock contracts over speculative defensive checks. Validate
  before mutation where needed and preserve or restore a coherent state on
  failure. A silent fallback to stock behavior is not success when the requested
  mod behavior was not performed. Do not remove necessary safety checks or
  introduce unbounded retries merely to avoid dealing with a failure.
- In a critical unrecoverable failure, an explicit fatal error that terminates
  the game is permitted as a last resort: crashing is very undesirable, but
  preferable to silently leaving the mod nonfunctional or continuing in a
  corrupt/indeterminate state. Preserve actionable failure context and avoid
  corrupting persistent data. Do not use a crash for ordinary out-of-scope input
  or when safe recovery is available; do not invent crashes merely to satisfy
  this rule.
  This policy is not authorization to deliberately crash a live user session;
  controlled crash experiments still follow the research and test safeguards.
- Deliberately ignored out-of-scope events and verified idempotent no-ops are
  normal outcomes, not errors. Keep them distinct from a failed operation on
  an object the mod is responsible for; document that boundary explicitly.
