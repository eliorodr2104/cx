# M6d: Pattern matching

Status: **Completed** (implemented and verified in this checkout).

Baseline: the pinned checkout recorded in the [M0 record](M0.md).

Fourth slice of roadmap row M6: the pattern matching of
[`language/enums.md`](../../language/enums.md#pattern-matching), over the raw and
simple enums of [M6b](M6b.md) and the payload enums of [M6c](M6c.md).

```c
switch (token) {
  case .integer(value):
    use(value)
  case .location(line, _):
    at(line)
  case .north, .south:
    vertical()
  case .end:
    finish()
}
```

## The rule

A `switch` whose condition has a Cx enum type is a Cx switch. Every other
switch is C's, with fallthrough, labels in nested blocks and Duff's device.
The parser decides from the condition's type once it is parsed, so no C switch
changes meaning.

## Semantics

- **Labels** name cases as `.name` or `Type.name`; the type must be the
  switch's enum. `case .a, .b:` shares one body. C's integer labels are errors.
- **Bindings.** `.case(a, _, c)` binds each payload element to a name, or
  discards it with `_`; the count must match the payload. Bindings are `const`
  copies, visible in their clause only. `case .location:` ignores the payload.
  A label with several cases binds nothing.
- **No fallthrough.** Each clause ends at the next label; `break` still leaves
  the switch and `continue` its loop. Each clause is its own scope, so a
  declaration may follow the label directly. An empty clause followed by
  another label is an error with a fix-it that joins them.
- **Top-level labels only.** A `case` or `default` of a Cx switch inside a
  nested block is an error.
- **Exhaustiveness.** Without `default`, every case must be handled; the error
  lists the missing ones. Duplicates are errors. A `default` no value reaches is
  warned about (`-Wcx-unreachable-default`).
- **Invalid values.** Without a `default`, a value that matches no case traps.
  Only memory reinterpretation can make one.

## Lowering

The Cx switch is an ordinary C `SwitchStmt`, so code generation, PCH, the
analyzer and tools see nothing new:

```c
switch (Token $m = token; (int)$m.$tag) {        // init-statement slot
  case 0: { const int value = $m.$payload.integer; use(value); break; }
  case 1: { const int line = $m.$payload.location.$0; at(line); break; }
  case 2: { finish(); break; }
  default: __builtin_trap();                     // no user default
}
```

- The condition is evaluated once into a hidden variable (`CxMatch`), held by
  the switch's init-statement, which Clang's C AST and CodeGen support.
- The switch tests the tag, or a raw enum's value, as its backing integer, and
  each case label is that case's value.
- Each clause's body is a compound statement: the bindings
  (`CxPatternBinding(index)`), the clause's statements, and an implicit `break`
  without a location.
- The trapping `default` has no location either; both are how printers tell
  them from source.

## Permitted baseline C modes

Every C/GNU standard, as in M0.

## Implementation

| Area | Change |
| --- | --- |
| `clang/include/clang/Basic/Attr.td` | `CxMatch`, `CxPatternBinding` |
| `clang/lib/Parse/ParseStmt.cpp` | `ParseParenExprOrCondition` hands a Cx enum condition to the switch; `ParseCxSwitchBody` parses clauses, patterns and bindings; nested labels are diagnosed in `ParseCaseStatement`/`ParseDefaultStatement` |
| `clang/lib/Sema/SemaCx.cpp` | `ActOnCxSwitchStart`, `ActOnCxSwitchCase`, `ActOnCxSwitchBindings`, `ActOnCxSwitchFinish`, `isCxSwitch`, `diagnoseCxNestedSwitchLabel` |
| `clang/lib/AST/StmtPrinter.cpp` | `-ast-print` prints the Cx switch from its lowering |

## Evidence

- The full Clang lit suite passes, with the known deadlocking
  `Index/crash-recovery-modules.m` filtered out; `switch.c` also compiles a C
  switch with fallthrough to its usual IR.

## Tests

- **`switch.c`**, as GNU17 and GNU89, then as IR, after `-E` and after
  `-ast-print`: several cases per label, qualified labels, raw values in the
  jump table, no fallthrough, bindings with `_`, ignored payloads, `default`,
  `break` and `continue`, nested switches, the trap, and a C switch.
- **`switch-errors.c`**: missing cases, duplicates, wrong enum, integer labels,
  empty clauses, nested labels, unreachable `default`, binding errors, and a
  write to a binding.
- **`switch-pch.c`**: matching a payload enum from a PCH and from `-include`.

## Known limitations

- **Patterns** with values, nested patterns, guards, labels in bindings and
  `if case` are future work.
- **A Cx switch needs a braced body.**
- **`goto` into a clause** skips its bindings' initialization, as C allows for
  any declaration.
