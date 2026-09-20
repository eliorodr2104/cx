# Foundational Library Protocols

The initial shared abstractions include Equatable, Comparable, Hashable, Iterator,
Iterable, Collection, MutableCollection, and RandomAccessCollection.

## Equality

```c
protocol Equatable {
    static bool operator ==(Self lhs, Self rhs)
}
```

Equality laws and their relationship to identity should be documented. `!=` can be
derived as the negation of the chosen equality operation. Raw pointers/classes must
not gain unexpected deep equality merely because collections need comparisons.

## Ordering

Comparable provides an ordering contract, not only an operator name. Deriving every
comparison from `<`/`>` is valid only under the stated laws. Floating-point NaNs and
partial orders are not a total order automatically; do not derive `<=` as `!(>)`
for types where those differ.

The protocol can choose separate partial/total-order facilities or require explicit
operations. That API/law decision is G11 and precedes generic sort correctness.

## Hashing

Hashable refines the equality contract. Equal keys must hash compatibly within the
container's hashing context. Hash values are not promised stable across process,
seed, platform, or version. A serialized identifier must use a separate stable format,
not the container's incidental hash result.

## Collection algorithms

Protocol extensions should share search, map/filter/reduce, enumeration, and other
applicable behavior without hardcoding all containers. Constraints reflect actual
requirements: sort needs writable random-access or a chosen fallback algorithm;
contains may need Equatable; a generic Iterator need not provide a cheap count.

Generic methods through erased protocols require the ABI capability described in
[Existentials](../abi/existentials.md); adding a requirement is not proof its invocation
is already supported.

## Core-known capabilities

Error, OptionSet, and PointerTag are compiler-recognized semantic capabilities exposed
through canonical declarations. User code must not obtain privileged behavior merely
by declaring an unrelated local protocol with the same spelling.
The compiler identifies the canonical declaration, not just its text name.
