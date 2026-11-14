# Error handling and defined outcomes

- For supported runtime conditions, every in-scope input or requested action
  must have a defined outcome. Never silently return from an error check and
  leave promised behavior missing or the mod in an indeterminate, partially
  applied state. A warning that accompanies broken behavior is not a substitute
  for a working implementation.
- Required authored components, assets and identities are project contracts.
  Establish the affected structural requirements when authoring or changing
  them instead of hiding violations behind silent fallback branches. Keep
  necessary input/output checks in the owning operation; this is not a mandate
  for a separate test or a full structural audit on every build. The
  [test scope rule](testing-rule.md) distinguishes those checks from regression
  coverage.
  Successful compilation or packaging proves only the checks actually performed;
  it does not establish every structural requirement. When removing a guard in
  reliance on such a contract, identify the evidence that establishes it.
- Required structure does not guarantee runtime lifetime or availability. Even
  a mandatory object can become unavailable through destruction, unloading or
  owner teardown during supported play. Preserve guards and meaningful failure
  responses where those situations can reach the operation. Distinguish them
  from an authored object that was missing or misconfigured from the start.
  Removing a redundant guard does not guarantee how Unreal will react to a
  broken project contract, and does not require a replacement failure mechanism.
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
  Permission to crash is not a requirement to cause or guarantee a crash. Do not
  add a crash mechanism, runtime dependency or investigation solely to turn an
  accepted possibility of failure into deterministic termination. Such work needs
  a separate functional requirement; ordinary graph cleanup does not imply one.
  This policy is not authorization to deliberately crash a live user session;
  controlled crash experiments still follow the research and test safeguards.
- Deliberately ignored out-of-scope events and verified idempotent no-ops are
  normal outcomes, not errors. Keep them distinct from a failed operation on
  an object the mod is responsible for; document that boundary explicitly.
