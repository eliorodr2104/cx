# Closures and Callable Types

Cx callables are first-class values, distinct from C function pointers. Their
source syntax remains return-type-first.

```c
int (bool, int) operation
int (enabled: bool, value: int) labeledOperation
int (value: int) throw(ParseError) parser
```

Calls obey the type's interface:

```c
operation(true, 10)
labeledOperation(enabled: true, value: 10)
var result = try parser(value: 10)
```

Callable labels and error effects survive in the static type. Equal representation
does not imply identical call syntax.

## Literals and contextual typing

```c
int (int) twice = { value in
    value * 2
}

int (int, int) add = {
    $0 + $1
}
```

A single-expression closure may return implicitly. A multi-statement body uses
ordinary returns. `$0`, `$1`, and named closure parameters get their types from the
expected callable signature where available. Explicit closure-parameter type syntax
and capture-list syntax remain language details to settle; no global inference is
promised for a context-free `{ $0 > 0 }`.

## Trailing closures

```c
var positives = values.filter { item in
    item > 0
}

values.sort {
    $0 < $1
}
```

A final callable argument may be written after the parenthesized arguments; when it
is the only argument, the empty argument parentheses may be omitted. A trailing
closure is still an argument, not a block executed automatically at creation.
The exact rule for omitting an explicit final parameter label requires grammar tests.

## Captures

The baseline captures a value with normal value semantics, retains an ARC class
reference as required, and copies a raw pointer without extending pointee lifetime.
Mutable shared captures, weak captures, and manual-RC capture ownership need explicit
contracts; they are not inferred from the fact that Swift offers similar syntax.

Escaping environments receive sufficient lifetime. Nonescaping environments may be
stack allocated or eliminated when proven safe. No claim is made that every closure
is inlined or that every capture is free.

## Uniform materialized ABI

A general materialized callable has an invocation entry and an environment reference.
Every value of a given callable type has one compatible ABI, even if its environment
is empty. The compiler knows each generated environment's concrete layout.

Optimization may eliminate the pair/environment and inline a known call. It must
not change a public stored callable's size based on which closure happens to occupy
it. See [Callable ABI](../abi/callables.md).

## C function pointers

```c
int (*callback)(int)
```

This remains a plain C-compatible function-pointer type. A known context-free,
non-throwing function or closure may convert through an ABI-correct entry/thunk.
An arbitrary already type-erased callable cannot be assumed context-free from its
signature alone. Captured state must never be silently discarded.

A runtime `context == null` check is not by itself enough: the invocation entry may
still use the hidden-context ABI. A real C-compatible entry is required.

## Function references and labels

`convert(value:)` selects a declaration family; `&convert(value:)` requests a raw
address after overload/ABI resolution. A Cx callable may retain its label interface.
Erasing labels to an otherwise matching unlabeled callable is the accepted direction;
implicit relabeling is not.
