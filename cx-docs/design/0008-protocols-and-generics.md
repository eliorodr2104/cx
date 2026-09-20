# 0008 - Protocol-Oriented Generic Abstraction

## Status

Accepted capability set; existential generic invocation has an ABI gate.

## Context

POP is a central Cx abstraction rather than a reduced optional feature. Static specialization and explicit runtime erasure need different visible forms.

## Decision

Support explicit conformance, defaults, refinement/composition, associated types, conditional extensions/conformances, static and generic requirements, checked generics, and `any P` including constrained associated types. Prefer a conformer implementation over a matching default witness. Extension-only methods are not automatically new requirements.

## Consequences

Conformance does not add per-instance storage. Concrete/generic code can specialize; shared generics may use witnesses without boxing. A generic requirement through any needs more than a naive pointer to one monomorphization. Type-erased Self inputs are not automatically mutually compatible.

## Implementation gates

G06: generic/static witness entry ABI, metatypes/opening, constrained default selection and overlap, retroactive whole-build coherence, and generic body dependencies across artifacts.

## Related contracts

[Protocols](../language/protocols.md), [Generics](../language/generics.md), [Existentials](../abi/existentials.md).
