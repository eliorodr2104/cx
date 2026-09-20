# 0006 - Synthesized Construction and Cleanup

## Status

Accepted constructor suppression rule; initialization analysis details are gated.

## Context

Memberwise boilerplate should disappear, but a custom initializer should define invariants without an automatic alternative bypassing them.

## Decision

Generate constructors when no user init is declared. Required fields become construction inputs; declaration defaults apply automatically. Any custom init declaration suppresses generated initializers. Init bodies can throw. Automatic field destruction remains even when a custom non-throwing deinit hook exists.

## Consequences

Generated construction does not require source declarations like `int x x`. Explicit body statements retain source order; automatic field order does not justify reordering effects. Failure cleans constructed fields and storage without calling normal deinit on a never-completed object.

## Implementation gates

G02 for call shape; G07 for defaults/delegation, field state transitions, self escape, failure cleanup and OOM. Constructor visibility cannot expose inaccessible fields/types accidentally.

## Related contracts

[Initializers](../language/initializers.md), [Cleanup](../runtime/initialization-and-cleanup.md).
