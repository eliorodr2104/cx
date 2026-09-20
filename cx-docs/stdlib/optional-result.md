# Optional and Result

## Optional values

```c
enum Optional<T> {
    some(T),
    none
}
```

Optional is a normal value abstraction: a present T or absence. It supports iteration
termination, dictionary misses, and values that need not exist. Copies/destruction
operate only on a present payload.

```c
Optional<int> count = .some(3)
Optional<int> missing = .none
```

`User?` and the requested `String?` provide nil-capable source forms. The proposed
uniform rule `T? == Optional<T>` beyond those cases needs the explicit confirmation
recorded in G03. This document does not silently change every raw pointer declaration.

`Optional<T*>` has at least the semantic states absent, present-null, and present
non-null. Because null is already valid for T*, it cannot alone encode absence.
No representation optimization may collapse distinct states.

## Result as data

```c
enum Result<T, E> {
    success(T),
    failure(E)
}
```

Result stores or composes outcomes. It does not define the language error channel.
An algorithm may cache `Result<Data, ParseError>` without throwing; a function may
return Data and declare `throw(ParseError)` without returning Result.

A bridge that captures a throwing operation into Result may be a library convenience,
but its API is not an implicit rewrite of every `try`.

## Desired API

Pattern matching is the core operation. Optional/Result transformations such as map,
flatMap, and defaulting should use ordinary callables and documented error propagation.
An absent branch must not evaluate its present-value transformation. A default
producer should be lazy if the API promises lazy fallback.

Operator sugar (`??`, forced unwrap, `try?`) and throwing transformation overloads need
a separate contract. Do not add them solely because their names are familiar.

## Representation

Use the enum value ABI. Non-nil reference payloads can offer a niche; arbitrary values
may need a discriminator. ABI size is not necessarily payload size plus exactly one
byte. Alignment/padding and the selected layout algorithm apply.
