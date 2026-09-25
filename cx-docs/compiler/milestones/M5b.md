# M5b: Optional semicolons

Status: **Completed** (implemented and verified in this checkout).

Baseline: the pinned checkout recorded in the [M0 record](M0.md).

Second slice of roadmap row M5: the semicolon elision of
[`language/lexical-rules.md`](../../language/lexical-rules.md#optional-semicolons).

```c
var count = 0
var limit = 10
struct Point { int x
               int y }
```

## The rule

> Where C requires a `;` and there is none, a line break, a closing brace
> `}`, or the end of the file stands in for it.

It is the same principle as [M5a](M5a.md):
- **The rule only ever applies where C reports "expected ';'".** At that
  point the code is not valid C, so giving it a meaning changes no C program,
  in any mode.
- **Everything C can read as a continuation is read that way first**,
  because the parser only reaches the missing `;` once C's grammar has
  nothing else to accept:
  - `foo\n(bar);` stays one call;
  - `return\n compute();` returns `compute()`;
  - `x = a\n - b` is one subtraction;
  - `int a = 1,\n b = 2` is one declaration;
  - `struct S {…}\n s = {…};` declares `s`.

  The specification requires all of these.
- **A `for` header keeps its separators.** It checks its own `;` and never
  reaches the rule, so `for (i = 0\n i < n; …)` is still an error.
- **Only a boundary counts.** `v = 1 v = 2;` on one line is still an error.

One C reading is easy to miss. `break` and `continue` take a label in C2y,
so `break\nname = 0` reads as `break name`, which Clang parses in every
mode. That is C's reading, so it wins, and the statement needs its `;`.

**Where it applies.** Everywhere in Cx mode, since it cannot change valid
C, as for M5a. A C header that compiles in C is not touched.

**Seeing it.** `-Wcx-implicit-semicolon`, off by default, reports every
implied `;` with a fix-it that inserts it. A team that wants explicit
semicolons, or a migration in progress, can turn it on, or make it an error
with `-Werror=cx-implicit-semicolon`.

## Boundaries in detail

- **Line break.** The next token begins a line. For a macro, what counts is
  where it is invoked: a token inside an expansion never starts a line, so
  two statements from macros on one line still need a `;`.
- **Closing brace.** `{ return x }`, `{ return }` and `({ int t = f(); t })`.
- **End of file.** The last declaration may omit its `;`.
- **Preprocessed output keeps the line breaks.** `-E` writes each token on
  its source line, so compiling the output gives the same code.
  Output that joins lines (`-P` with `-fminimize-whitespace`) does not keep
  them.

## Implementation

Clang already required every `;` through two functions,
`Parser::ExpectAndConsumeSemi` and `Parser::ExpectAndConsume` with
`tok::semi`. Both now ask `TryCxImplicitSemicolon`, which accepts a boundary
in Cx mode and reports `warn_cx_implicit_semicolon`.

Three callers ran their error recovery unconditionally after asking for the
`;`, skipping to the next `}` whether or not one was found:
- a struct member;
- a continuation member;
- the end of a statement.

With a `;` implied that would have discarded valid code, such as every member
after the first in `struct Point { int x \n int y }`. Each now recovers only
when the `;` is really missing.

An implied `;` consumes nothing. At a token nothing can start, a recovery
loop would imply one, fail to parse, imply another, and never advance. The
fuzzing corpus found this: 138 mutated inputs hung. A `;` is therefore
implied at most once per token (`CxLastImpliedSemicolon`). The second time
at the same token is C's ordinary missing `;`, whose recovery advances.

A method body is parsed after its record's closing brace. The replay now
restores the parser's previous-token location, so a `;` implied after the
record, and its fix-it, point at the right place.

| Area | Change |
| --- | --- |
| `clang/lib/Parse/Parser.cpp`, `Parser.h` | `TryCxImplicitSemicolon`, called from both `;` expectations |
| `clang/lib/Parse/ParseDecl.cpp` | struct and continuation member recovery only on a real miss; previous-token location kept across a method-body replay |
| `clang/lib/Parse/ParseStmt.cpp` | statement recovery only on a real miss |
| `clang/include/clang/Basic/DiagnosticParseKinds.td`, `DiagnosticGroups.td` | `warn_cx_implicit_semicolon`, `-Wcx-implicit-semicolon` |

## Evidence

- **The corpus.** The M5a corpus and method again, before and after this
  slice: 1,828 files under C89, GNU89, GNU17 and C23, with AST and
  diagnostic fingerprints. 162 file-and-mode pairs changed, across 56
  files; most of them test the parser and are full of deliberate missing
  `;`. Each pair was checked with `-x c` in the same mode:
  - 154 pairs are code that is already an error in C there;
  - the other 8 are `Preprocessor/SOURCE_DATE_EPOCH.c` and
    `Preprocessor/pr133574.c`, whose output depends on the clock and differs
    between two runs of the same compiler.

  No valid C file changed.
- **`implicit-semicolons-c.c`.** C written across lines (calls, operators,
  subscripts, the conditional operator, `return`, a declaration list, a
  struct definition with declarators, `do`/`while`, `if`/`else`) compiles to
  IR identical as C and as Cx. It also compiles with
  `-Wcx-implicit-semicolon -Werror`, which shows that no `;` was implied in
  valid C.

## Tests

- **`implicit-semicolons.c`**:
  - struct members, a method body, a struct and an enum definition, and a
    typedef;
  - `var`, `let` and declaration lists;
  - `break`, `do`/`while`, `if`/`else` and `goto`;
  - a statement expression, a block, and the end of the file;
  - continuations checked in IR, also after `-E`;
  - the C2y named `break`, a same-line miss, and a `for` header still being
    errors.
- **`implicit-semicolons-c.c`**: the C identity above.
- **`implicit-semicolons-recovery.c`**: syntax errors followed by line
  breaks, in a struct and in a function, terminate with errors. This is a
  guard in the shape of the hung inputs. The inputs themselves are in the
  fuzzing corpus, which now completes without a hang or crash.

Verification run on this checkout: `clang/test` 48974 passed, 30 expectedly
failed, 0 unexpected failures, with `Index/crash-recovery-modules.m` excluded
(see [M4.4](M4.4.md#build-and-test-plan)).

## Known limitations

- **No restricted productions.** Unlike JavaScript, `return` followed by a
  line break does not end the statement, because the specification requires
  `return\n compute()` to return the value. The same goes for `++` and `--`:
  `a\n++b` is C's `a++ b`, an error, not two statements.
- **A line that begins with `(`, `[`, `-`, `*` or `&` continues the one
  before it** wherever C can read it so. Write the `;` to end the previous
  statement, as the specification's escape hatch says.
- **Trailing closures**, which interact with the boundary before `{`, are
  M12.
