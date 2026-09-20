# Protocols and Protocol-Oriented Programming

Protocols specify behavior without adding stored fields to conforming values.
Conformance is explicit; an accidental matching method set does not silently create
conformance.

```c
protocol Drawable {
    ~mutating void draw()
}

protocol Collection {
    type Element
    size_t count { get }
    ~mutating Element element(size_t index)
}
```

Properties in a protocol describe access requirements using `{ get }` or
`{ get set }`. Associated types describe a conformance's chosen types, not additional
unwritten generic arguments.

## Conformance and defaults

```c
extension Rect: Drawable {
    ~mutating void draw() {
        renderRect(self)
    }
}
```

A type implementation matching a requirement supplies its witness. Otherwise an
applicable protocol-extension default may supply it. Without either, conformance
fails. Matching considers labels, types, effects, and receiver mutation guarantees.

An extension-only method that is not declared as a requirement is not an additional
polymorphic witness slot. Same-name methods do not retroactively change that fact.
Default selection is part of conformance construction, not a runtime name search.
Overlapping equally applicable defaults are an error until a unique rule applies.

## Refinement and composition

```c
protocol Shape: Drawable, Hashable {
    float area { get }
}

void inspect<T: Drawable & Serializable>(T value)
void inspectErased(any Drawable & Serializable value)
```

Refinement creates a named protocol with inherited requirements. Composition requests
multiple contracts without creating a new protocol declaration.

## Generics and existentials

`T: Drawable` preserves a generic type and permits specialization. It does not require
an `any` container. `any Drawable` explicitly requests type erasure.

```c
any Collection collection
any Collection<Element == String> strings
```

The unconstrained existential offers only operations whose use can be typed with
its available information. The constrained one preserves knowledge of `Element`.
Two separately erased `any Equatable` values do not prove the same concrete `Self`.
Input positions involving unknown `Self` cannot be called arbitrarily.

## Associated types and constraints

```c
protocol KeyedCollection {
    type Key: Hashable
    type Value
}

extension Collection where Element == String {
    // String-specific operations.
}
```

`Self: P` means conformance. `Self == ConcreteType` means type equality; equality with
a protocol name is not a substitute for conformance.

## Static and generic requirements

Static factories returning `Self` and methods with their own generic parameters are
accepted capabilities. How those requirements are invoked through erased values
requires a metatype/generic-witness ABI. A witness is not always just a pointer to
one fully concrete non-generic function. See G06 and [Existential ABI](../abi/existentials.md).

## Coherence and ownership

Retroactive conformance is allowed where the type/protocol are visible. A warning
highlights the case where the declaring module owns neither. Conflicting conformances
for one semantic `(type, protocol)` pair are errors, including across linked modules.
Conditional conformances must not overlap ambiguously.

Conformance alone adds no instance storage. Generic shared implementations may still
use metadata/witness arguments; specialization can remove them.
