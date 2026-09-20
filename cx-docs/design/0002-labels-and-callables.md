# 0002 - Type-First Labels and Callable Interfaces

## Status

Core syntax accepted; same-name optional-label shorthand remains pending.

## Context

Labels improve same-typed argument readability, but Swift-style type-after-name function declarations do not match the intended C appearance. Overloaded function addresses also need explicit selection.

## Decision

Use `Type externalLabel localName` for explicitly labeled ordinary parameters and plain `Type localName` for positional C parameters. Use `label: expression` at calls and compound references such as `&move(x:)`. Cx callable types can retain labels and effects: `int (value: int) throw(E) fn`. C function pointers keep C syntax and ABI.

## Consequences

Labels can distinguish overloads and callable interfaces without becoming runtime strings or Objective-C selectors. A raw callback projection erases labels only after selecting a C-compatible entry. The same-name label convenience and generated initializer argument surface must not be decided accidentally by examples.

## Implementation gates

G02; target-type disambiguation and ABI-correct callback conversion. A noncapturing property of a literal is not guaranteed for every erased callable value with the same signature.

## Related contracts

[Functions](../language/functions.md), [Callables](../language/closures.md), [Callable ABI](../abi/callables.md).
