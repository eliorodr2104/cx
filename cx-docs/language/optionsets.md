# Option Sets

`OptionSet` is a compiler-known capability for enum-style sets of independent flags.
It is not a mutually exclusive enum-state encoding.

```c
enum Permission: OptionSet {
    case read, write, execute
}

Permission granted = [.read, .write]
granted |= .execute
if (granted.contains(.write)) grant()
```

`: OptionSet` after the name of a Cx enum, one whose body uses `case`, makes it an option
set. An enum without `case` keeps its C23 meaning even when a type named `OptionSet` is
visible. Cases take no payload and no `= value`.

## Bits and representation

The case at position i, in declaration order, is bit i. Three cases permit eight
combinations, including the empty set. The backing type is the smallest unsigned integer
that holds every bit: 8, 16, 32 or 64 bits. More than 64 cases is an error, not a
truncation. The size and ABI of an option set are those of its backing integer.

## Values and literals

`.read` is a set holding one flag. `[.read, .write]` is a set literal and `[]` the empty
set, both valid where an option set is expected. A `[` cannot begin a C expression, so
the literal changes no C program.

## Algebra

| Operation | Meaning |
| --- | --- |
| `a \| b` | union |
| `a & b` | intersection |
| `a ^ b` | symmetric difference |
| `a - b` | difference |
| `~a` | complement within the declared bits |
| `\|=`, `&=`, `^=`, `-=` | update the left operand, evaluated once |
| `==`, `!=` | equality |
| `a.contains(b)` | every flag of `b` is in `a` |
| `a.isSubset(of: b)`, `a.isSuperset(of: b)` | set inclusion |
| `a.isDisjoint(with: b)` | no flag in common |
| `a.rawValue` | the bitmask as the backing integer |

Operands are option sets of the same type; a `.case` or set literal takes the other
operand's type. An option set does not convert to or from an integer and is not a
condition by itself: `if (p & .read)` is an error, and `p.contains(.read)` states the
test. Ordering, shifts, and other arithmetic do not apply, and an option set cannot be
the condition of a switch.

## Protocol integration

`PointerTag<T>` implies this capability. Other protocols may provide convenient set
methods but must not change the underlying flag interpretation.

## Open surface details

Construction from a raw bitmask and the treatment of unknown bits arrive with Optional.
Explicit case bit values, composite cases, and an explicit backing type remain G12.
