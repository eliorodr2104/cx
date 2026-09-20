# Generics and Constraints

Cx generic bodies are checked against declared requirements. They do not rely on
substitution into arbitrary concrete types to discover missing operations later.

```c
T maximum<T: Comparable>(
    T a,
    T b
) {
    return a > b ? a : b
}

struct Pair<A, B> {
    A first
    B second
}
```

## Inference and explicit arguments

```c
var result = maximum(10, 20)
var explicitResult = maximum<int>(10, 20)
```

Infer using argument types, a contextual result type, and constraints. Ambiguous or
unconstrained type variables are diagnosed. Explicit type arguments choose generic
arguments; they are not user-written template specializations with alternate bodies.

Type parameters are the initial scope. Generic integer/value parameters, packs,
and unrestricted compile-time expressions are future work. Ordinary C fixed arrays
remain available without inventing `StaticArray<T, N>` prematurely.

## Constraint vocabulary

```c
void inspect<T: Drawable & Serializable>(T value)

void process<T: Collection>(T value)
where T.Element == String {
    // ...
}
```

Supported design primitives are conformance (`T: P`), protocol composition (`P & Q`),
same-type constraints (`T == U`), and associated-type paths. Commas separate multiple
where requirements. `&` combines protocols, not arbitrary Boolean constraints.

Generic types are invariant by default. There is no automatic `Array<A>` to `Array<B>`
conversion merely because values of A could convert to B.

## Methods and conditional conformances

```c
extension Array<T>: Equatable where T: Equatable {
    // Requirement implementations.
}
```

Generic methods and generic protocol requirements are supported in the design.
Their dynamic invocation requires additional ABI work, not a promise that every
requirement is served by one concrete monomorphized witness.

## Type checking and code generation

Check bodies using requirements before specialization. Representation-dependent
validation can still occur for concrete instantiations, for example size/alignment
or a PointerTag capacity check. "Checked once" does not mean ignoring those checks.

Prefer concrete specialization when practical. Shared generic code can use metadata
and witnesses when required or profitable. Neither boxing nor dynamic dispatch is
mandatory merely because a generic parameter exists.

## Separate compilation

Declarations may reside in `.h` and bodies in `.c`. Cross-module specialization
requires compatible compiler artifacts or an explicitly supported shared entry.
Passing several filenames to an ordinary Clang driver does not automatically merge
their ASTs. Missing generic implementation metadata is an actionable build error.

See [Module and Generic ABI](../abi/modules-and-generics.md) and
[the dependency-correct roadmap](../Compiler/implementation-roadmap.md).
