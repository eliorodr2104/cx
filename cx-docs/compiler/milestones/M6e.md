# M6e: Option sets

Status: **Completed** (implemented and verified in this checkout).

Baseline: the pinned checkout recorded in the [M0 record](M0.md).

Fifth slice of roadmap row M6: the option sets of
[`language/optionsets.md`](../../language/optionsets.md), on the Cx enums of
[M6b](M6b.md).

```c
enum Permission: OptionSet {
  case read, write, execute
}

Permission granted = [.read, .write]
granted |= .execute
if (granted.contains(.write)) grant()
```

## The rule

`: OptionSet` after the name of an enum whose body uses `case` makes it an
option set. The body already makes it a Cx enum, so `OptionSet` is not read as
a C23 backing type there; an enum without `case` keeps its C23 meaning, even
with a typedef named `OptionSet` in scope.

## Semantics

- **Bits.** Case i is bit i, in declaration order. The backing type is the
  smallest unsigned integer with a bit per case: 8, 16, 32 or 64 bits. More
  than 64 cases is an error. Cases take no `= value` and no payload.
- **Values** are sets of flags, of the enum's type; `.read` is a set of one.
- **Literals.** `[.read, .write]` is the union of its elements and `[]` the
  empty set, where an option set is expected: an initializer, `=`, `==`, `!=`,
  an argument, `return`, and the right operand of a set operator.
- **Operators.** `|`, `&`, `^` and `-` (difference) between sets of one type,
  their compound forms, which update the left operand once, `~` (complement
  within the declared bits), and `==`/`!=`. Other operators, mixing two option
  sets or a set and an integer, ordering, and use as a condition are errors.
- **Tests.** `a.contains(b)` (every flag of `b` is in `a`), `a.isSubset(of: b)`,
  `a.isSuperset(of: b)`, `a.isDisjoint(with: b)`; each evaluates its operands
  once.
- **`rawValue`** is the bitmask as the backing integer.
- **No `switch`**: an option set is not a closed set of states.

## Representation

An option set is its backing integer. Everything lowers to integer
operations on the enum type, so CodeGen and PCH need nothing new:

| Source | Lowering |
| --- | --- |
| `[.a, .b]` | `(a \| b)` |
| `[]` and constant sets | an integer constant of the enum type |
| `~a` | `(a ^ [every case])` |
| `a - b` | `a & (b ^ [every case])` |
| `a.contains(b)` | `(b & (a ^ [every case])) == []` |

A constant set prints as the literal of the cases it holds, so `-ast-print`
output compiles to the same code.

## Permitted baseline C modes

Every C/GNU standard, as in M0.

## Implementation

| Area | Change |
| --- | --- |
| `clang/include/clang/Basic/Attr.td` | `CxOptionSet` |
| `clang/lib/Parse/ParseDecl.cpp` | `: OptionSet` before a `case` body; `ParseCxOptionSetLiteral`; an expected type for labelled arguments |
| `clang/lib/Parse/ParseExpr.cpp` | set literals, operand contexts of set operators, `set.contains(...)` |
| `clang/lib/Sema/SemaDecl.cpp` | bits, backing type, no values |
| `clang/lib/Sema/SemaCx.cpp`, `SemaExpr.cpp` | literals, operators, complement, tests; no payloads, no switch; `rawValue` |
| `clang/lib/AST/StmtPrinter.cpp`, `DeclPrinter.cpp` | constant sets and `: OptionSet` |

## Evidence

- The full Clang lit suite passes, with the known deadlocking
  `Index/crash-recovery-modules.m` filtered out.
- The IR of each enum test file is identical to the IR of its `-ast-print`
  output. That comparison found a missing pair of parentheses around a literal
  in a printed test, fixed and guarded by an IR check.
- `enums-raw-c.c` keeps `enum Flags : OptionSet { F0 }` with a typedef named
  `OptionSet` a C23 enum.

## Tests

- **`optionsets.c`**, as GNU17 and GNU89, then as IR, after `-E` and after
  `-ast-print`: backing types at 8/9, 17 and 64 cases, bit values, literals,
  every operator and compound form, the masked complement, the tests, and a
  constant global.
- **`optionsets-errors.c`**: values, payloads, 65 cases, other operators,
  mixed sets, integers, ordering, conditions, literals without a type, test
  labels, and `switch`.

## Known limitations

- **No construction from a raw bitmask** and no treatment of unknown bits until
  checked construction (M10); explicit bit values, composite cases and an
  explicit backing type remain G12.
- **A literal needs its type** from the context; `[.read]` alone is an error.
- **`-ast-print`** shows the lowering of `~`, `-` and the tests, which compiles
  to the same code.
