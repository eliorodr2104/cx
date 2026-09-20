# Tuples

Tuples are lightweight product values with heterogeneous element types.

```c
(int, String) user
(int id, String name) labeledUser
```

## Access

```c
user.$0
user.$1
labeledUser.id
labeledUser.name
labeledUser.$0
```

Positions are zero-based compile-time element selections, not dynamic homogeneous
array indices. Named access and positional access select the same stored elements.
Labels do not reorder storage.

## Construction

```c
String name = "Mario"
var user = (
    id:   42,
    name: name
)
```

The explicit String binding avoids silently deciding how a context-free string
literal inside a tuple is inferred. `(42, name)` is the desired unlabeled form in
a Cx tuple context; its ambiguity with C's comma expression is G01.

In particular, legacy `return (a, b);` must continue to evaluate the C comma
expression. Documentation must not promise that every parenthesized comma list
becomes a tuple.

## Destructuring

```c
var (id, name) = user
var (_, name) = user

for ((value, index) in span.enumerated()) {
    consume(value, index)
}
```

Destructuring is positional and creates individual value bindings. `_` is a discard
only in the pattern context. The initializer is evaluated once, not once per binding.

## Type compatibility and lifetime

Tuples use value semantics, with memberwise managed copy/destruction. Corresponding
positional types determine structural assignment compatibility; changing element
labels does not require a runtime conversion. The destination's labels govern later
named access. Duplicate/ambiguous labels are diagnosed.

ABI layout is deterministic for a target/ABI version but may contain padding.
Use a named struct for a domain identity, invariants, methods, or an explicit C API.
See [Enum and Tuple ABI](../abi/enums-and-tuples.md).
