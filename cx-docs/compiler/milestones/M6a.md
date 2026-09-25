# M6a: Tuples

Status: **Completed** (implemented and verified in this checkout).

Baseline: the pinned checkout recorded in the [M0 record](M0.md).

First slice of roadmap row M6: the tuples of
[`language/tuples.md`](../../language/tuples.md), and the tuple half of the
[G01](../../OPEN-ISSUES.md#g01--c-grammar-collisions) collision *"tuple vs C
comma expression"*.

```c
(int id, float score) user = (id: 42, score: 1.5)
(int, float) divide(int a, int b) { return (a / b, (float)a / b) }

var (id, _) = user
user.score = user.$1 * 2
```

## The rule: the comma stays C's

A comma inside parentheses keeps meaning C's comma operator everywhere C can
read it. A tuple is recognized only where C has no reading:

- **A tuple type is `(`, a type name, and a comma at the top level of the
  parentheses.** In C, `(` and a type name open a cast or a compound literal,
  and those parentheses hold exactly one type name, so `(int, float)` is not
  C anywhere. It works wherever a type can be written: variables, members,
  parameters, return types, `typedef`, `sizeof`, casts and compound literals.
  Each element may carry a label: `(int id, float score)`.

  After another specifier, qualifier or attribute, as in `static (int, float)
  x`, C can read `(int, float)` as an abstract function declarator with an
  implicit int. So there the tuple type also needs a name or `*` after it,
  which no such declarator can have.
- **A labelled literal is a tuple anywhere.** No C expression puts `:` after
  a leading identifier inside parentheses, so `(id: 42, score: 1.5)` is never
  C.
- **An unlabelled literal is a tuple only where a tuple is expected**:
  - the initializer of a tuple variable, or of a `var` or `let`, which is not
    C;
  - the right-hand side of `=` whose left-hand side is a tuple;
  - an argument for a tuple parameter, in any function the call may name;
  - `return` in a function that returns a tuple.

  None of these positions exists in C, because C has no tuple types. Everywhere
  else, `(a, b)` is the comma operator: `int x = (a, b)` and `return (a, b)`
  in a function returning `int` keep their C meaning.
- **Destructuring is `var (a, b) =` or `let (a, b) =`.** Without the `=`,
  `var(a, b)` is a C89 call of an implicitly declared `var`. A call is never
  assigned to, so with the `=` it is no C.
- **No tuple has one element.** `(int) x` is a cast, and `(x: 1)` is
  rejected.

A line that begins with a tuple type starts a declaration, so M5b's implied
`;` holds before it. `1\n(int, float) t` cannot be a call, because a call takes
no type argument.

## Semantics

- **Storage.** A tuple is an implicit struct with the fields `$0`, `$1`, …,
  one per list of element types. Two tuple types with the same element types
  are the same type, so `(int n, float f)` and `(int, float)` convert freely,
  as the specification asks. Layout, ABI, copying and `sizeof` are the
  struct's.
- **Labels are sugar.** A labelled tuple type is an implicit typedef of that
  struct carrying the labels. `t.id` becomes `t.$0` for the element the label
  names. The labels of the type as written govern: after
  `(int, float) plain = user`, `plain.id` is an error. Labels are unique, and
  may not contain `$`. `_` means no label.
- **Literals take their destination's element types.** `(1, 2.5)` initializing
  an `(int, float)` converts each element, as initializing each field would.
  A literal whose labels disagree with its destination's is an error, since
  it is most likely a swap. Without a destination, as for `var`, each element's
  type is deduced as `var` deduces it.
- **Destructuring evaluates the initializer once**, into a hidden variable,
  and each name becomes a variable initialized from its element. `_` discards
  the element, and `let` makes the names const. It is available inside a
  function, since file-scope initializers must be constant.
- **Symbols.** A tuple in a signature mangles as the Itanium vendor type
  `u6$TupleI<element types>E`, which demangles as `$Tuple<int, float>`.
- **Printing.** Diagnostics show `(int, float)` and `(int id, float score)`.
  `-ast-print` prints tuple types, literals and destructuring as source that
  compiles to the same code.

## Permitted baseline C modes

Every C/GNU standard, as in M0. No valid C program changes meaning (see
Evidence).

## Implementation

| Area | Change |
| --- | --- |
| `clang/lib/Parse/ParseDecl.cpp` | `isCxTupleTypeStart`, `ParseCxTupleType`, the `(` case of the declaration specifiers; `isCxTupleLiteralStart`, `ParseCxTupleLiteral`; `ParseCxDestructuring`; the initializer is a tuple context |
| `clang/lib/Parse/ParseExpr.cpp` | tuple literals in `ParseCastExpression`; tuple contexts for assignment and arguments; a line starting with a tuple type ends the call suffix |
| `clang/lib/Parse/ParseStmt.cpp`, `Parser.h` | `return` is a tuple context; `var (a, b) =` is a declaration; statements and `for` start with a tuple type |
| `clang/lib/Sema/SemaCx.cpp` | `BuildCxTupleType`, `ActOnCxTupleLiteral`, `retargetCxTupleLiteral`, `TranslateCxTupleLabel`, `ActOnCxDestructuring` |
| `clang/lib/Sema/SemaInit.cpp`, `SemaExpr.cpp`, `SemaExprMember.cpp` | literals converted on initialization and assignment; label access |
| `clang/include/clang/Basic/Attr.td` | `CxTuple` on the struct, `CxTupleLabels` on the typedef |
| `clang/lib/AST/TypePrinter.cpp`, `StmtPrinter.cpp`, `ItaniumMangle.cpp` | printing and mangling |

Struct and typedef are found again by a name no source can spell, through the
identifier resolver. That is also how a PCH or clangd's preamble brings back
the types it already made, so a tuple type from a header is the same type in
the file that includes it.

## Evidence

**The corpus.** The M5a corpus and method again, before and after this
slice: 1,828 files under C89, GNU89, GNU17 and C23, with AST and diagnostic
fingerprints.

30 file-and-mode pairs changed, across 8 files, and every one of them depends on
the clock or the process. Five differ between two runs of the same compiler:
`Lexer/builtin_redef.c`, `Parser/crash-report.c`,
`Preprocessor/SOURCE_DATE_EPOCH.c`, `pr133574.c` and `pragma.c`. The other three
print `__DATE__`, which changed day between the two runs:
`Lexer/warn-date-time.c` and `Preprocessor/init-datetime-macros-{warns,nowarns}.c`.
No other file changed, including the SDK file.

The first comparison caught two defects, both fixed before closing:
- **A block literal's parameter list was read as a tuple.** In
  `^ __attribute__((format(printf, 2, 3))) (int arg, const char *f, ...) {}`,
  the parameters follow an attribute that the declaration specifiers had
  already taken. C reads them as an abstract function declarator with an
  implicit int. So after any specifier, qualifier or attribute, a tuple type
  now also needs a name or `*` after its `)`: C has no declarator that
  continues that way. `const (int, float)` as a parameter keeps its C89
  meaning too.
- **A crash on `(T)(f)(a, b)`.** After a cast, the parser holds `(f)` as a
  list with no type yet, and asking whether `f` takes a tuple read that null
  type.

**`tuples-c.c`** compiles C that looks like a tuple as C and as Cx, and
requires identical IR, in GNU89, GNU17 and C23:
- the comma operator in initializers, assignments, arguments, `return`,
  statements and a `for` header;
- casts and compound literals, including a cast inside a comma;
- a parenthesized declarator, and the valid C function-type parameter
  `int ((int, float))`;
- a block literal whose parameter list follows an attribute, and `(T)(f)(a, b)`;
- in C89, `var(a, b);` as a call, and a `const (int, float)` parameter.

## Tests

- **`tuples.c`**, in C17, GNU89 and C23, in IR, and after `-E` and after
  `-ast-print`:
  - tuple variables, members, parameters, return values, a `typedef` and a
    nested tuple;
  - labelled and unlabelled literals with conversion, labels and `$N`
    access, through a pointer too;
  - relabelling across a call, assignment of a literal, `sizeof`;
  - destructuring with `var`, `let` and `_`;
  - a tuple type on the line after an implied `;`;
  - the struct layout and the mangled parameter.
- **`tuples-errors.c`**: element types, labels, one-element tuples, label
  mismatch, destructuring errors, incompatible tuples, unknown members, and
  a `let` tuple.
- **`tuples-pch.c`** (with `Inputs/cx-tuple.h`): the types of a PCH and of an
  included header are the file's types.
- **`tuples-c.c`**: the C identity above.

The fuzzing corpus was rerun as for the earlier slices, together with 3,035
mutations and truncations of the tuple tests, under GNU89, GNU17 and C23 and
through code generation: no crash and no hang.

Verification run on this checkout: `clang/test` 48978 passed, 30 expectedly
failed, 0 unexpected failures, with `Index/crash-recovery-modules.m` excluded
(see [M4.4](M4.4.md#build-and-test-plan)).

## Known limitations

- **`var (a, b) =` only at the start.** Nested patterns, `for ((a, b) in …)`
  (M10), and destructuring at file scope are not supported.
- **Elements of managed types.** Copy and destruction of tuple elements with
  cleanup semantics come with M7; today an element copies as C copies the
  struct.
- **Unlabelled tuples in a struct initializer.** `{ (1, 2), 3 }` is C's
  comma; write `{ {1, 2}, 3 }` or label the elements. A tuple is a struct, so
  braces initialize it anywhere.
- **Element sugar is not kept.** `(size_t, int)` prints as its canonical
  element types, `(unsigned long, int)`.
- **Tuples have no `==`.** Equality is not in the specification, and a C struct
  has none.
- **After a declaration without an initializer**, `int a\n(int, float) t` is
  still C's `int a(int, float)`, a function declarator, and needs the `;`.
