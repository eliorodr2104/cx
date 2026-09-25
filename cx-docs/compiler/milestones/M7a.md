# M7a: `defer`

Status: **Completed** (implemented and verified in this checkout).

Baseline: the pinned checkout recorded in the [M0 record](M0.md).

First slice of roadmap row M7: the `defer` of
[`language/defer.md`](../../language/defer.md).

```c
FILE* file = fopen(path, "rb")
if (file != NULL) {
  defer { fclose(file) }
  useFile(file)
}
```

## The rule

`defer {` at the start of a statement registers its block for the exit of the
enclosing scope. `defer` is not reserved: anywhere else, and in C, it is an
ordinary name.

## Semantics

- **Registration** happens when execution reaches the statement; an exit
  before it does not run the block.
- **Order.** Defers of one scope run in reverse registration order.
- **Loops.** A defer in a loop body runs at the end of each iteration, and on
  `break` or `continue`.
- **Return.** The returned value is computed before the defers run, so
  `defer { *p = 5 }` after `return *p` returns the old value.
- **Jumps** out of the scope run the defers. A jump into a deferred block, or
  forward past a `defer` into its scope, is an error.
- **Inside the block**, `return`, and `break`, `continue` or `goto` that leave it,
  are errors, as are calls to `setjmp`, `longjmp` and their variants.
- **Non-local exits.** `longjmp`, `exit` and signals do not run defers.
- **Cx switch.** Each clause is a scope, so its defers run before the clause's
  implicit `break`.

## Representation

Clang already implements the C `_Defer` technical specification: its AST node,
jump checking and lowering. A Cx `defer` builds that same node without
`-fdefer-ts`, and `-ast-print` spells it `defer` in Cx. CodeGen, PCH and the
jump diagnostics need nothing new.

## Permitted baseline C modes

Every C/GNU standard, as in M0.

## Implementation

| Area | Change |
| --- | --- |
| `clang/lib/Parse/ParseStmt.cpp`, `Parser.h` | `defer {` starts a defer statement; a `defer` missing its block is an error; `return`, `break` and `continue` end before a `defer {` line |
| `clang/lib/AST/StmtPrinter.cpp` | `defer` in Cx |

## Evidence

- The full Clang lit suite passes, with the known deadlocking
  `Index/crash-recovery-modules.m` filtered out.
- The IR of `defer.c` is identical after `-E` and after `-ast-print`.

## Tests

- **`defer.c`**, as GNU17 and GNU89, then as IR, after `-E` and after
  `-ast-print`: reverse order, a loop with `break`, the returned value, an
  early `return`, a Cx switch clause, `goto` out of a scope, and `defer` as a
  variable name.
- **`defer-errors.c`**: a missing block, `return`/`break`/`continue` out of the
  block, `goto` into, past and out of it, a C `case` past a defer, and
  `setjmp`/`longjmp` inside it.

## Known limitations

- **Error propagation** runs defers once Cx errors exist (M13).
- **Managed locals** share the cleanup stack with defers once they exist
  (M7b, M11).
