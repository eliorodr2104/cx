# 0007 - Properties and Indexed Library Access

## Status

Accepted surface direction; addressability/writeback is not yet fully specified.

## Context

Stored fields should not require handwritten forwarding accessors. Computed values and modern containers need ergonomic property/bracket access.

## Decision

Stored fields have implicit read/write access. Getter-only computed bodies omit `get`; explicit `get`/`set` supports computation on both paths. `private(set)` and `internal(set)` restrict mutation. Subscript declarations use the same accessor model. Protocol access requirements remain explicit.

## Consequences

Computed access may execute code even without call parentheses. Equal source syntax does not imply identical binary ABI or addressability. Nested mutation and compound assignment cannot simply mutate/discard a getter copy. Stored-to-computed migration may require a rebuild or adapter.

## Implementation gates

G08: getter/setter effects, receiver mutability, mutable address exposure, exactly-once base/index evaluation, in-place modify/writeback, and COW exclusivity. These precede performance claims.

## Related contracts

[Properties](../language/properties.md), [Subscripts](../language/subscripts.md), [Span](../stdlib/span.md).
