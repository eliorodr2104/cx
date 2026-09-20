# 0005 - Values, References, and Const Receivers

## Status

Accepted; resource-copy and manual-RC details still require contracts.

## Context

Cx should retain C-like value types while making shared object lifetime convenient. A pointer declarator must not double as an ownership-policy switch.

## Decision

Structs/tuples/payload enums are values. Classes are reference types with ARC by default. Managed members get synthesized ownership operations; raw pointers stay raw. `let` is inferred top-level const. Struct methods mutate by default; `~mutating` is an explicit receiver promise. Use `self`, never `this`.

## Consequences

A copied struct can share the class objects referenced by its members without becoming a reference type itself. Const receivers are shallow, not pure. Adding methods does not add instance storage. A lint can suggest nonmutation but does not change the declared ABI/API based on a body.

## Implementation gates

G07: raw resource-owning value copies, MRC return/field/capture ownership, weak synchronization, OOM and non-local cleanup. Manual-RC declaration spelling remains open.

## Related contracts

[Structs](../language/structs.md), [Memory](../language/memory.md), [Ownership runtime](../runtime/ownership.md).
