# Option Sets

`OptionSet` is a compiler-known capability for enum-style sets of independent flags.
It is not a mutually exclusive enum-state encoding.

```c
enum Permission: OptionSet {
    read,
    write,
    execute
}

Permission granted = [.read, .write]
granted |= .execute
```

Each atomic case consumes one bit. Three atomic cases permit eight combinations,
including the empty set. The compiler chooses an adequate backing representation
unless a valid explicit backing type constrains it.

The exact underlying-type spelling/default policy remains subject to C-compatible
enum disambiguation. PointerTag does not require the programmer to write a width.

## Algebra

The desired shared set surface includes union, intersection, symmetric difference,
difference, membership, subset/superset, and disjointness:

```c
var both       = a & b
var either     = a | b
var different  = a ^ b
var remaining  = a - b

bool separate = a.isDisjoint(with: b)
```

Operations produce the same option-set type. Compound assignments update a mutable
binding once. `join` may be an alias for union if adopted; the canonical API naming
remains a library decision, not a new semantic operation.

Complement `~` needs a defined universe. For a closed option set, the recommended
rule is complement within declared valid bits, not blindly setting every padding or
future bit. Raw imported bitmasks need their own checked/unchecked construction
contract before this is frozen.

## Protocol integration

`PointerTag<T>` implies this capability. Other protocols may provide convenient set
methods but must not change the underlying flag interpretation.

## Open surface details

Explicit atomic/composite case values, import of unknown bits, empty literals, the
backing integer default, and the exact complement universe are G12. The implementation
must reject capacity overflow and duplicate atomic assignments rather than truncate.
