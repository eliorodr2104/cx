# Variables and Type Inference

Ordinary typed C declarations remain available. `var` and `let` add initializer-based
inference without introducing a separate declaration syntax for ordinary functions.

## Independent deduction

```c
var count = 10,
    ratio = 10.0,
    scale = 10.0f,
    limit = 10L
```

These independently deduce `int`, `double`, `float`, and `long`. There is no single
shared inferred base type for the entire declaration. Each binding needs an
initializer and a uniquely deducible type.

```c
var missing // error: no initializer
```

The supported numeric suffixes are those of the selected C dialect until an
explicit Cx literal proposal says otherwise. Examples do not assume `10d` or `10f`.

A visible C typedef or macro named `var` retains its C interpretation.

`var` and `let` are separate declarations, not new spellings of C `auto` or the
GNU `__auto_type` extension. Both of those keep their selected-dialect meaning
inside Cx mode, including the C rule that one `__auto_type` declaration cannot
deduce two different types. A Cx rule never reaches a C placeholder, and the
compiler reports each of the four specifiers under the name that was written.

## `let`: inference plus top-level const

```c
int value = 10;
let pointer = &value

*pointer = 20       // allowed: pointee is mutable
pointer = null     // error: binding is constant
```

The inferred declaration corresponds to `int* const pointer`, not `const int*`.
A value-type binding becomes immutable; a class binding cannot be reassigned but
the referenced object's permitted mutation remains possible.

```c
let user = User(...)
user = otherUser    // error
user.rename(...)   // permitted if the method is otherwise accessible
```

## Deduction details

Inference uses an initializer's value type, not its incidental storage identity.
It preserves pointee qualification, optionality, and Cx callable labels/effects.
Top-level qualification of the source expression must not accidentally make a
new `var` binding immutable. C array/function decay and explicit declarator
modifiers need dedicated tests against the chosen deduction rules.

`var` does not perform unbounded inference from all future uses. A closure written
only as `{ $0 > 0 }` needs a contextual parameter type or an explicitly typed form.

Multiple declarators, self-reference, earlier-binding scope, qualifiers, and arrays
are listed in [the feature matrix](../Compiler/feature-matrix.md).

## Stored fields

An inferred stored field needs a declaration-site initializer from which to infer
its type. Writing `let id` without a type or initializer is not a complete field
declaration. Deferred initialization of explicitly typed immutable fields is part
of [the initializer contract](initializers.md).
