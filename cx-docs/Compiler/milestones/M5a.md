# M5a — Implicit tag names

Status: **Completed** (implemented and verified in this checkout).

Baseline: the pinned checkout recorded in the [M0 record](M0.md).

First slice of roadmap row M5, and one of the six collisions
[G01](../../OPEN-ISSUES.md#g01--c-grammar-collisions) names outright:
*"implicit tag-name/type conveniences"*.

## What it is

```c
struct Size { int width; int height; };

Size s = Size(width: 80, height: 40);   // no typedef anywhere
void take(Size v);
Size make(void);
```

A tag declared with `struct`, `union` or `enum` is usable as a type name on
its own. The `typedef struct X { ... } X;` ceremony becomes optional, and
`struct X` keeps working everywhere it works today.

Two things in the language needed it:
- **A type can now name itself** inside its own methods and initializers. The
  typedef only exists after the whole declaration, and method bodies are
  replayed before that.
- **Construction** reaches a type name in expression position. Before this
  slice that was only a typedef'd or annotated name ([M4d](M4d.md)).

## The rule: C first, always

The requirement is that every valid C program keeps its meaning, in every
C and GNU mode. The rule that meets it is:

> A bare tag name is a type name **only where nothing in the ordinary
> namespace has that name, and only in a position C cannot read**.

This means:
- **Any ordinary declaration of the name wins, at any scope.** That covers a
  variable, function, parameter, enumerator, typedef, or a library builtin a
  C call would implicitly declare. `struct stat` beside `stat()` is
  unchanged. So is `int T; void f(void) { struct T {…}; T = 1; }`: `T` is
  the global variable.
- **Where C reads the tokens, C wins.** Three constructs read an identifier
  that happens to be a tag name, and all three keep their C reading:
  - C89 implicit int (`static T;`, `T = 1;` at file scope);
  - the implicit function declaration of C89–C17 (`T(3)`);
  - K&R identifier lists (`int f(T) int T; {}`).
- **Everywhere else C rejects the code**, with "must use 'struct' tag",
  "unknown type name" or "use of undeclared identifier". Cx accepts it,
  meaning the tag.

This differs from C++, where the innermost tag hides an outer ordinary name.
That rule would change valid C, so Cx does not use it.

### Where a bare tag name is a type

| Position | Accepted when the name is followed by | Otherwise |
| --- | --- | --- |
| Declaration: variable, parameter, member, typedef, return type | an identifier, `*`, `const`, `volatile` or `restrict` | C's reading or C's error |
| Inside expression parentheses: `sizeof(T)`, `_Alignof(T)`, `(T)x`, `(T *)p`, `(T){…}` | `)`, `*` or `[` | C's reading; `(T(…))` may be an implicit call |
| Construction `T(…)` | a labelled first value (`T(width: 80)`); from C23, any form | C's implicit function call (C89–C17) |

Declarations reuse C's own recovery. `ParseImplicitInt`, and `ClassifyName`
at the start of a statement, already recognised a tag name used without its
keyword and reported "must use 'struct' tag". Cx accepts that recovery
silently when nothing ordinary has the name and the next token begins a
declarator. The implicit-int check sits in front of it and keeps C's reading
wherever C89 would declare the name.

In an expression, `(T` followed by `)`, `*` or `[` can only be a type in
Cx: C would take `T` as an undeclared identifier. `(T(` is excluded because
it may be an implicit call.

For construction, `Tag(` is a call in C89–C17. So a bare tag starts a
construction only with a labelled first value, which no call has, or from
C23, which has no implicit declarations. A typedef'd name constructs in
every form, as before.

### Answers to the design's open questions

- **Tags gain no linkage or ownership.** A type is not an entity with a
  symbol.
- **A tag from an unowned C header is usable in an owned file, and in an
  unowned one.** Visibility is not ownership, and the rule changes no C
  meaning anywhere, so it applies throughout Cx mode.
- **An enum tag and an enumerator of one name:** the enumerator is ordinary,
  so it wins (`enum Color { Color }`).
- **Anonymous and typedef-only tags** have no name to use, and are
  unaffected.

## Evidence

G01 asks for a regression corpus, a dialect matrix, idiom tests and `-E`
round trips.

**The corpus.** I compiled 1,828 files in `-x cx` under C89, GNU89, GNU17
and C23, before and after this slice, and compared fingerprints of the AST
dump (addresses stripped) plus the diagnostics: 7,312 comparisons. The files
are every `.c` test in `clang/test/{C,Sema,Parser,Preprocessor,Lexer}`, plus
one file including 40 SDK headers (`stdio.h`, `sys/stat.h`, `pthread.h`,
`netinet/in.h`, …).
- Eight files differ.
- Six of them differ from one run of the same compiler to the next:
  `__DATE__`, `__TIME__`, `SOURCE_DATE_EPOCH`, and the two tests that crash
  on purpose and print temporary paths.
- The other two change only on code that is already an error in C:
  - `Parser/declarators.c` loses its two "must use 'struct' tag" errors on
    `xyz` and `myenum`, which Cx now accepts;
  - `Sema/cxx-as-c.c` is C++ code with five errors in C either way; its
    `Foo f;` is now valid.
- The SDK file and every other valid C input are identical.

The comparison caught two effects on C code, both fixed before closing:
- **The tag lookup declared library builtins as a side effect.** It looked up
  the ordinary name first, which creates the builtin for any undeclared
  call. It now looks the tag up first and asks the ordinary name only when a
  tag exists.
- **Enum tags were treated as possible constructions.** In C23 this changed
  error recovery on `enum __declspec …`. Only a struct or union tag now
  counts.

**The dialect matrix and idioms.** `implicit-tag-names.c` runs under C89,
GNU89, C99, GNU17, C23 and GNU23. `implicit-tag-names-c.c` compiles the
known idioms as C and as Cx, and requires identical IR, in GNU89, GNU17 and
C23:
- `struct stat` with `stat()`;
- `struct timeval` with a local `timeval`;
- `typedef struct Node Node`;
- a tag with an enumerator of its name;
- a tag shadowed by an inner variable;
- C89 implicit int and an implicit call on a tag name;
- a K&R identifier list naming a tag.

**`-E`.** The rule depends on nothing the preprocessor removes. The test
compiles the preprocessed output to the same code.

## Permitted baseline C modes

Every C/GNU standard, as in M0. No valid C program changes meaning (see
Evidence).

## Implementation

| Area | Change |
| --- | --- |
| `clang/lib/Sema/SemaCx.cpp` | `getCxImplicitTagType`: the tag, looked up first, and only when nothing ordinary has the name; `getCxConstructionType` takes a bare tag when the caller allows it |
| `clang/lib/Parse/ParseDecl.cpp` | `ParseImplicitInt` accepts a tag name silently before a declarator; `TryAnnotateCxImplicitTagInParens`; `isCxImplicitTagConstruction` |
| `clang/lib/Sema/SemaDecl.cpp` | `ClassifyName`: the tag-name recovery is silent before a declarator; `Tag(` for a struct or union is left to the expression path |
| `clang/lib/Parse/ParseExpr.cpp`, `Parser.h` | construction and parenthesized type names consult the rule |
| `clang/include/clang/Sema/Sema.h` | the two Sema entry points |

## Tests

- **`implicit-tag-names.c`** (six dialects, IR, and IR after `-E`):
  - declarations, parameters, members, typedefs, return types and qualified
    uses;
  - `sizeof`, `_Alignof`, casts and compound literals;
  - labelled construction, and a type naming itself in its methods;
  - an ordinary declaration winning at file and block scope;
  - an enumerator winning over its tag;
  - `Size;` still an error;
  - `Size(3)`, a call before C23 and a construction from C23.
- **`implicit-tag-names-c.c`**: the C idioms above, IR identical as C and as
  Cx in GNU89, GNU17 and C23.

Verification run on this checkout: `clang/test` 48971 passed, 30 expectedly
failed, 0 unexpected failures, with `Index/crash-recovery-modules.m` excluded
(see [M4.4](M4.4.md#build-and-test-plan)).

## Known limitations

- **Some positions that C rejects are still rejected**, because only the
  common ones were opened. Examples:
  - `T (*fp)(void);` — a parenthesized declarator after a bare tag;
  - `_Generic(x, T: …)`;
  - a bare tag directly after `(` of a parameter list, `f(T)`, which C reads
    as a K&R list.

  Each is safe to open later case by case, with the same evidence.
- **Before C23, `Tag(value)` without a label is C's implicit call**, so it is
  not a construction. In an owned file the call is then an error, because
  its implicit declaration has no prototype ([M4.6](M4.6.md)).
- **It does not remove the other five G01 collisions**, which still gate the
  rest of M5: tuple versus comma expression, raw enums, compact ranges,
  generic angles and trailing closures.
