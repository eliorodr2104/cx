# Subscripts

Cx subscript declarations provide indexed access for library types without hardcoding
Array or Span as special pointer-like objects.

```c
T subscript(size_t index) {
    get {
        return data[index]
    }

    set {
        data[index] = newValue
    }
}
```

A single body without explicit accessor blocks is a getter, as for computed properties.
The declaration can be a protocol requirement using `{ get }` or `{ get set }`.

## Calls and overloads

```c
var item = values[index]
values[index] = replacement
```

Subscript overload selection uses index types and the ordinary Cx constraints.
A range subscript may return another value or view depending on the library contract.
No implicit ownership choice follows just from using brackets.

Raw C arrays and pointers retain builtin C indexing. A library subscript can add
bounds checks; it must not silently change all raw indexing in the translation unit.

## Receiver and writeback

Read-only receivers may invoke compatible getters. Setters and mutating operations
must obey receiver/access rules. `values[i].mutate()` on a value element is not
implemented correctly by mutating a discarded getter copy: it requires a defined
writeback or address-based modify protocol.

`values[indexFunction()] += rhs` evaluates the storage selection once. Alias checks,
COW uniqueness, error cleanup, and setter invocation counts must be specified before
this form is implemented. These are G08, not mere optimizer details.

## Bounds

Array and Span subscripts are checked by default. Proved-safe checks may be removed.
The exact out-of-bounds response and checked throwing/optional alternatives are
library-contract decisions in G11. Raw `data[index]` retains the programmer's C
responsibility for address, range, and lifetime validity.
