# 0011 - A Modern Layer Above libc

## Status

Core surface accepted; several naming and lifetime choices remain proposals.

## Context

Cx should provide values/algorithms absent from C without duplicating its complete platform ecosystem.

## Decision

Add Optional, Result, Span, owning dynamic collections, String/StaticString, ranges, collection/equality/hash protocols, Dictionary and Set. Keep checked library indexing and raw C escape hatches. Use protocol extensions for shared algorithms. High-level I/O and custom allocator frameworks can come later.

## Consequences

Span is not an owning array. Filter can allocate. COW needs exclusivity for mutable views. C strings need termination/lifetime rules; a cast cannot always be free. A selected C dictionary can provide a future unchanged-code portability case, but no port is recorded here.

## Implementation gates

G03 for general optional sugar; G10 for Array/List/StaticArray naming and String conversion; G11 for iterator/bounds/hash/API contracts. StringView is not part of the chosen public library surface.

## Related contracts

[Library index](../stdlib/README.md), [Optional](../stdlib/optional-result.md), [Array](../stdlib/array.md), [String](../stdlib/string.md).
