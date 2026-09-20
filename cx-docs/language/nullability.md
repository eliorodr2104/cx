# Nullability and Optional Values

## Two absence concepts

`null` is the typed null-pointer literal for raw C pointers. `nil` represents the
absence of an optional Cx value/reference. Neither means that an arbitrary old
allocation is known to have been freed.

```c
Node* node = null
User? user = nil
String? name = nil
```

Class optionality and optional String support are desired. The latest proposed
unification is `T?` as sugar for `Optional<T>` for other value types too. This is
recorded as a **proposed generalization**, not a silently accepted change to every
C type; see G03. `Optional<T>` itself is an accepted library abstraction.

## Raw pointers remain C

A `T*` can be null without a modifier. It is not non-null by default and Cx adds no
required non-null qualifier to ordinary pointer syntax. The previously rejected
`T*?` spelling is not silently reintroduced by the optional-value proposal.
An explicit `Optional<T*>` can be discussed separately: an absent optional and a
present null pointer would be distinct states, not a free extra niche.

```c
int* legacy = NULL; // legacy C remains valid
int* native = null  // native Cx spelling
```

`null` should reuse Clang's `nullptr` semantics where compatible, while retaining
contextual lookup so existing identifiers/macros called `null` continue working.
Null pointer representation is target-defined, not universally all-zero bytes.

## Optional class and String values

A plain `User` or `String` cannot contain `nil`. `User?` or `String?` can. Setting an
ARC optional to `nil` releases its previous strong ownership if present. Another
strong reference may keep that object alive.

`String?` distinguishes absence from an empty `String`. It does not make an empty
string null-terminated storage disappear or convert the String into a raw pointer.

## Access and extraction

No unchecked implicit conversion from optional to non-optional is permitted.
The complete syntax for checked binding, chaining, defaulting, and forced extraction
has not yet been selected. Documentation must not invent a working `!` or `??`
operator contract without that decision.

With the explicit library enum, ordinary pattern matching is available:

```c
Optional<int> result = .some(42)
switch (result) {
    case .some(value):
        consume(value)
    case .none:
        handleMissing()
}
```

If the generalized sugar is accepted, `nil` denotes `.none` for the corresponding
optional type and no C legacy expression is reinterpreted.

## Weak loads and boundaries

Weak storage yields a safely promoted optional strong reference or absence. It is
not a raw dangling pointer. Raw `null` and Cx `nil` are not interchangeable at ABI
boundaries without an explicit conversion contract.

See [Optional and Result](../stdlib/optional-result.md) and
[Weak References](../runtime/weak-references.md).
