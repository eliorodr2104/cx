# Contiguous Dynamic Collection

**Naming status:** `Array<T>` is the working name, not a final resolution of the user's
Array/List/StaticArray preference. This document defines the container behavior
independently of that name.

## Distinct categories

| Form | Storage and lifetime |
| --- | --- |
| `T values[N]` | Ordinary fixed-size C array with C semantics |
| `Span<T>` | Non-owning contiguous view |
| Working `Array<T>` | Owning growable contiguous value-semantic collection |

A first-class StaticArray type would require a value-parameter or equivalent size
type mechanism; it is not declared implemented by rebranding a C array. Existing C
arrays are not assigned, passed, or copied like a new library value by magic.

## Value semantics and COW

```c
Array<int> a = [1, 2, 3]
Array<int> b = a
b.append(4)
```

`a` and `b` are independent logical values. Sharing backing storage is an implementation
optimization; a mutation must detach where necessary. A value element may still
contain a class reference, whose referenced object is shared under normal class rules.

## Desired surface

Count/capacity/emptiness, checked subscript, append/insert/remove, reserveCapacity,
removeAll, iteration, search, and scoped buffer access belong in the basic API.
Method names/return conventions and allocation failure effects need final API signatures.

Algorithms should be shared through protocols when appropriate:

```c
var positives = values.filter { $0 > 0 }
values.sort { $0 < $1 }
var ordered = values.sorted { $0 < $1 }
```

`sort` mutates a mutable receiver; `sorted` returns a separate value. `filter` is eager
unless explicitly named/documented lazy. It can allocate result storage. `contains`,
map/reduce, first-match, all/any satisfy, and enumerated adapters are desired. A method
named `where` would be an optional filter alias, not a second constraint system.

## Checked access and writeback

Library indexing is checked by default. Bounds-failure behavior must be specified
rather than assumed to throw/trap arbitrarily. Nested mutation of a value element
requires correct modify/writeback; setters and compound updates cannot silently mutate
a temporary or evaluate an index twice.

## Mutable views

Do not expose an unrestricted mutable Span that lets clients write into buffers after
COW sharing changes. See [Span](span.md) and G08. Allocation, mutation invalidation,
and escaping raw-pointer behavior need clear contracts.

Growth policy, small buffers, and COW thresholds are measured implementation choices.
