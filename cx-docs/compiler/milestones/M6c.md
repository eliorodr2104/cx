# M6c: Payload enums

Status: **Completed** (implemented and verified in this checkout).

Baseline: the pinned checkout recorded in the [M0 record](M0.md).

Third slice of roadmap row M6: the payload enums of
[`language/enums.md`](../../language/enums.md#payload-enums), built on the Cx
enums of [M6b](M6b.md) and the tuples of [M6a](M6a.md).

```c
enum Token {
  case integer(int)
  case location(int line, int column)
  case next(Token *)
  case end
}

Token a = .integer(7)
Token b = Token.location(line: 10, column: 4)
Token c = flag ? .end : .location(3, 4)
```

## Representation

A Cx enum with a parenthesized case is a payload enum. The enum itself is a
simple Cx enum (M6b), the **tag**, with one case per case. The values are an
implicit struct holding the tag and a union of the payloads:

```c
struct <enum Token> {
  enum Token $tag;             // one byte here
  union {
    int integer;               // a one-element payload is its type
    (int line, int column) location;
    Token *next;
  } $payload;                  // end has no payload and no member
};
```

Every source spelling of the enum as a type names that struct: `enum Token`,
`Token` (M5a), a typedef, and `Token *` inside the enum's own body. Diagnostics,
`-ast-print` and symbols show it as `enum Token`. The struct, its tag and its
union are not reachable from source; a payload is read by matching (M6d).

Three implicit attributes carry the design, so PCH and preambles need nothing
new: `CxPayloadRecord` on the enum names the struct, `CxPayloadEnum` on the
struct names the enum, and `CxEnumPayload` on each case holds its payload type
and labels.

The parser knows a body is a payload enum before reading it, by looking for a
`name (` at the top level of the body, so that `Token *` in a payload already
names the struct.

## Semantics

- **Payloads** are written as tuple types. One element is the payload itself,
  since a one-element tuple does not exist; several make a tuple (M6a).
  Element types are those tuples accept; `void`, function and incomplete types
  are errors.
- **No recursion by value.** A payload holding the enum itself is an error; a
  pointer to it is fine.
- **No backing type** and no `rawValue`: a payload enum with `: type` is an error.
- **Construction**: `.case(args)`, `Enum.case(args)` and, for a case without a
  payload, `.case` / `Enum.case`. Arguments follow tuple literal rules: labels
  are optional, written labels must match, and each element converts as
  initialization would. `.case` without its payload, `.case()` on a case
  without one, and a wrong number of values are errors.
- **Contexts** are those of M6b: initializer, `=`, argument, `return`, both
  branches of an expected `?:`, and braced arrays of the enum.
- **Layout**: C struct layout of tag then union, so the size is the tag, padding
  to the union's alignment, and the largest payload. A zero-initialized value
  holds the first case with a zeroed payload.
- **No comparison**: `==` and `!=` on payload enum values are errors that point
  to matching, until protocol-based equality (M8).
- **Values** copy and assign as the struct they are. Every payload is a plain
  copy in M6, so the aliasing rule for replacing a payload (G12) has nothing
  to do yet; M7 has to honour it.
- **Symbols**: the struct mangles as its enum, as a raw enum does.
- **Printing**: `-ast-print` prints `case location(int line, int column)` and
  constructions as `Token.location(3, 4)`, which compile back to the same code.

## Permitted baseline C modes

Every C/GNU standard, as in M0.

## Implementation

| Area | Change |
| --- | --- |
| `clang/include/clang/Basic/Attr.td` | `CxPayloadEnum`, `CxPayloadRecord`, `CxEnumPayload` |
| `clang/lib/Parse/ParseDecl.cpp` | `isCxPayloadEnumBody`; payloads in `ParseEnumBody`; `ParseCxTupleElements` and `ParseCxLabeledArguments`, shared with tuples and construction |
| `clang/lib/Parse/ParseExpr.cpp` | `ParseCxEnumCaseSuffix`: a payload after `.case` and `Enum.case` |
| `clang/lib/Sema/SemaCx.cpp` | the value struct, payload types, `ActOnCxEnumCaseCall`, the value built as a compound literal with a designator into the union; `getCxEnum` for the struct; no construction through `Token(...)` |
| `clang/lib/Sema/SemaType.cpp`, `SemaDecl.cpp`, `SemaCx.cpp` | every spelling of the enum as a type resolves to the struct (`getCxTagSpellingType`); `ActOnEnumBody` completes it |
| `clang/lib/Sema/SemaExpr.cpp` | comparisons |
| `clang/lib/AST/TypePrinter.cpp`, `DeclPrinter.cpp`, `StmtPrinter.cpp`, `ItaniumMangle.cpp` | printing and mangling as the enum |

## Evidence

- The full Clang lit suite and the Clang unit tests pass, with the known
  deadlocking `Index/crash-recovery-modules.m` filtered out.
- C is unaffected by construction: every change is behind a Cx enum, which C
  cannot declare.

## Tests

- **`enums-payload.c`**, as GNU17 and GNU89, then as IR, after `-E` and after
  `-ast-print`: layout and sizes, every construction form, conversions,
  contexts, a pointer to the enum in its own payload, a typedef, zero values
  and a braced array.
- **`enums-payload-errors.c`**: backing type, recursion, `void` payloads, a
  missing or extra payload, counts, labels, comparison, `rawValue`, hidden
  members, `switch`, and conversion from an integer.
- **`enums-payload-pch.c`** (with `Inputs/cx-payload.h`): through a PCH and
  through `-include`.

## Known limitations

- **No recursion by value**, no managed payload types (M7), no generic payload
  enums (M9), no `==` (M8).
- ~~**Payload arguments have no expected type**: `.wrap(.red)` needs
  `.wrap(Color.red)`.~~ Closed by [M6.2](M6.2.md).
- **Matching** is M6d; until then a payload is written but not read.
