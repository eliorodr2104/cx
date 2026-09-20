# 0010 - Enum Values, Option Sets, and Pointer Tags

## Status

Accepted capabilities; C enum disambiguation and pointer boundaries remain gates.

## Context

Cx needs convenient payload values and flag sets while retaining classic C enums and low-level pointer code.

## Decision

Preserve C enum semantics. Add payload/protocol enums and compiler-known OptionSet behavior. PointerTag<T> implies OptionSet, derives low-bit capacity from alignment/target support, and exposes pointer operations without a TaggedPtr wrapper. Cx-aware dereference masks associated tags.

## Consequences

A fixed enum base is not sufficient to distinguish Cx from C23. Independent flags are not mutually exclusive states. Low-bit availability is not determined by pointer width alone. Arbitrary raw casts, foreign calls, equality, and atomics still require explicit rules.

## Implementation gates

G01, G05, G12. Payload niches must exclude only impossible live values; null is not an extra niche for raw nullable pointers. Enum copy/assignment must preserve aliased incoming payloads before old cleanup.

## Related contracts

[Enums](../language/enums.md), [OptionSet](../language/optionsets.md), [PointerTag](../language/pointer-tagging.md).
