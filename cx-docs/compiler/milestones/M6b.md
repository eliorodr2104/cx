# M6b: Raw and simple enums

Status: **Completed** (implemented and verified in this checkout).

Baseline: the pinned checkout recorded in the [M0 record](M0.md).

Second slice of roadmap row M6: the raw and simple enums of
[`language/enums.md`](../../language/enums.md), and the enum half of the
[G01](../../OPEN-ISSUES.md#g01--c-grammar-collisions) collision *"fixed-underlying
C23 enums vs modern raw enums"*.

```c
enum Status: unsigned short {
  case ok = 200, created
  case notFound = 404
}
enum Direction { case north, south, east, west }

Status s = .ok
unsigned short code = s.rawValue
enum Direction heading = flag ? .north : Direction.south
```

## The rule: `case` selects a Cx enum

A body that uses `case` is a Cx enum; every other enum is C, including
`enum E : unsigned char { A }` in C23. `case` is a keyword C cannot place in an
enumerator list, so the rule changes no valid C program and needs no lookahead
beyond the token after `{`.

A Cx enum is a Clang scoped enum, the representation of C++'s `enum class`,
created in C. Scoped enums already keep their cases out of the enclosing scope,
out of integer promotion and out of the arithmetic and bitwise operators, in
every language. What C mode still let through is closed explicitly (see
Semantics).

## Semantics

- **Clauses.** `case a, b = 2` lists cases; a line break, `;` or the next `case`
  ends a clause, and a trailing comma before `}` is accepted. Mixing `case`
  clauses with C enumerators is an error, as is an anonymous Cx enum.
- **Raw enums** name a backing type after `:` and follow C's values: 0 first,
  then one more than the previous case. A value must fit the backing type, and
  two cases may not share one.
- **Simple enums** have no backing type, no `= value` and no `rawValue`. They
  are stored in the smallest unsigned integer holding every case: 1 byte up to
  256 cases, then 2, then 4.
- **Case type.** After the `}` every case has the enum's type, in every C mode.
  Inside the body a case has the backing type, so `second = first + 1` works.
- **Names.** `Status.ok` names a case, through the tag or a typedef; a variable
  named `Status` keeps C's member access. `.ok` takes its enum from the expected
  type: a variable initializer, the right operand of `=`, `==` and `!=`, an
  argument whose parameters agree on the enum, `return`, both branches of a
  `?:` whose value is expected, and the elements of a braced array of the enum.
  A bare `ok` is an error that names `.ok` and `Status.ok`.
- **Designators stay C.** In braces, `.name` is a designator when the chain of
  `.name` and `[...]` reaches `=`; otherwise it is a case.
- **Conversions.** `s.rawValue` is the only way out, as the backing type. There
  is no implicit conversion to or from an integer or another enum, no C cast
  across (`(int)s`, `(Status)1`), no compatibility with the backing integer in
  pointer or redeclaration types, no use as a condition (`if`, `&&`, `||`,
  `?:`), no ordering, and no passing to `...`. Only `==` and `!=` compare.
- **`switch`** on a Cx enum is an error until pattern matching (M6d).
- **Layout and symbols.** A raw enum is its backing type in the ABI. Cases are
  constants without symbols; an enum in a signature mangles as any tag type of
  the module.
- **Printing.** `-ast-print` prints `enum Status : unsigned short { case ok =
  200, created, ... }`, `Status.ok` and `s.rawValue`, which compile back to the
  same code.

## Permitted baseline C modes

Every C/GNU standard, as in M0. Cx defaults to GNU17, so nothing depends on a
C23-only rule.

## Implementation

| Area | Change |
| --- | --- |
| `clang/lib/Parse/ParseDecl.cpp` | a `case` body passes the `case` location as the scoped-enum keyword to `ActOnTag`; clauses in `ParseEnumBody`; the initializer is a `.case` context, also for array elements |
| `clang/lib/Parse/ParseExpr.cpp` | `Status.ok` through a tag or typedef; `.ok` in `ParseCastExpression`; contexts for `=`, `==`, `!=`, arguments and `?:` branches |
| `clang/lib/Parse/ParseStmt.cpp`, `ParseInit.cpp`, `Parser.h` | `return` context; `isCxDesignatorAhead` and braced array elements |
| `clang/lib/Sema/SemaDecl.cpp` | simple backing type, case type after the body, duplicate values, no values in simple enums |
| `clang/lib/Sema/SemaCx.cpp` | `getCxEnum`, `getCxEnumQualifier`, `ActOnCxEnumCase`, `getCxCaseArgumentType`, `BuildCxEnumMember`, `diagnoseCxEnumCondition` |
| `clang/lib/Sema/SemaCast.cpp`, `SemaExpr.cpp`, `SemaExprMember.cpp`, `SemaStmt.cpp`, `clang/lib/AST/ASTContext.cpp` | the C-mode leaks: casts, type merging, conditions, ordering, varargs, `switch`; `rawValue`; the bare-name hint |
| `clang/lib/AST/DeclPrinter.cpp`, `StmtPrinter.cpp` | Cx spellings |

`Status.ok` and `.ok` are ordinary references to the case and `rawValue` an
implicit integral cast, so PCH and preambles need nothing new: the scoped flag
and the written backing type are already serialized. Writing the first PCH of
a module-owned header with an enum exposed an older defect, fixed separately:
the module name of the ownership record got its identifier ID after the
identifier table was written.

## Evidence

- The full Clang lit suite passes, with the known deadlocking
  `Index/crash-recovery-modules.m` filtered out.
- **`enums-raw-c.c`** compiles C enums, a C23 fixed-underlying enum, and one
  whose base is a typedef named `OptionSet`, as C23 and as Cx, and checks that
  `case` in an enum is still an error in C.
- The M5a before/after corpus was not re-run for this slice; the `case` rule
  only applies to bodies C rejects.

## Tests

- **`enums-raw.c`**, as GNU17 and C23, then as IR, after `-E`, and after
  `-ast-print`: raw, simple and body-referencing enums, sizes, case types,
  qualified and contextual cases in every supported context, a variable that
  shadows the enum's name, designators, `rawValue`, and mangled signatures.
- **`enums-raw-errors.c`**: every clause error, simple values, duplicates,
  overflow, unknown and bare cases, missing contexts, and every conversion rule.
- **`enums-raw-pch.c`** (with `Inputs/cx-enum.h`): cases, backing types and
  `rawValue` through a PCH and through `-include`.
- **`module-ownership-pch-enum.c`**: a C enum in a module-owned PCH.

## Known limitations

- **`.case` for a struct field in braces or a tuple literal element** needs
  `Type.case` until M6.1, which resolves it against the destination after
  parsing.
- **`.case == value`** needs the enum on the left; `value == .case` works.
- **No construction from an integer** until checked construction with Optional
  (M10).
- **No `switch`** until pattern matching (M6d).
- **The bare-name hint** does not see cases of an enum read from a PCH; the
  error is then C's undeclared identifier.
- **`-ast-print`** spells `_Static_assert` as C23's `static_assert`, as upstream
  Clang does, so its output compiles as C23.
