# Iteration

Cx adds `for (... in ...)` while retaining the traditional C loop.

```c
for (value in span) {
    printf("%d\n", value)
}

for ((value, index) in span.enumerated()) {
    consume(value, index)
}
```

The loop introduces its bindings with inferred types. `enumerated()` produces
`(value, index)`, deliberately in that order. It does not use the opposite order
merely because another language does. Index type is intended to be `size_t` for
standard finite collections, not `int` by assumption.

## Iteration protocol

The current standard-library direction is `Iterable` producing an `Iterator`, with
`next()` returning `Optional<Element>`. The absence case terminates iteration.
This is source semantics; optimization need not materialize an Optional or existential
on every iteration.

Iteration over ordinary value elements produces values, not automatic writable
aliases to the underlying collection. Mutating a class-valued element can still
mutate its referenced object. Reference iteration syntax, binding mutability, iterator
invalidation, and iterator errors need G11 before implementation.

## Ranges

```c
for (i in 0...5) {
    // 0, 1, 2, 3, 4, 5
}

for (i in 0..<5) {
    // 0, 1, 2, 3, 4
}
```

`...` is closed; `..<` excludes the upper bound. Range iteration uses the same
protocol model. A closed range ending at the largest integer must terminate without
incrementing past that integer. The lowering is not blindly `i <= upper; ++i`.

## C loops

```c
for (int i = 0; i < count; ++i) {
    use(values[i]);
}
```

C control flow and array semantics remain unchanged. Range tokenization and flexible
semicolon elision are separate compatibility gates.

See [Ranges and Iteration](../stdlib/ranges-iteration.md).
