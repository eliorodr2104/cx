# M4b — Continuations

Status: **Completed** (implemented and verified in this checkout).

Baseline: the pinned checkout recorded in the [M0 record](M0.md).

Second slice of roadmap row M4, after [M4a](M4a.md). Access control is
[M4c](M4c.md) and generated memberwise construction is [M4d](M4d.md); M4 is not
complete until both land.

## Source contract

```c
// counter.h
#module Counters
struct Counter {
    int value;
    void increment();
    ~mutating int current();
};
```

```c
// counter.c
#module Counters
#include "counter.h"

struct Counter {
    void increment() { self.value += 1; }
    ~mutating int current() { return value; }
    void resetLocalState() { self.value = 0; }   // a new helper
};
```

- A block naming an **already complete owned type** is a continuation that
  implements its members, not a second definition.
- A continuation **cannot add a stored field**. The record's definition is
  never reopened: no `ActOnTagStartDefinition`, no `ActOnFields`, so the layout
  cannot change even by accident.
- A method in a continuation whose name, parameter types and argument labels
  match a declared member **is that member's definition**, and the two are one
  entity with one symbol. Local parameter names may differ.
- Exactly one implementation of a member is allowed; a second is a
  redefinition.
- A continuation may introduce a member the primary definition did not declare.
- Reopening is permitted when the continuation and the primary definition are
  **owned by the same module**, or, with no module, when both are in the **same
  file**. A type from an ordinary C header, or one owned by another module,
  keeps C's redefinition rule.

## Permitted baseline C modes

Every C/GNU standard, as in M0. Reopening a complete tag is an error in C, so
the form takes a position C cannot use, and every case this milestone does not
accept still produces C's `redefinition of ...`.

## Implementation

| Area | Change |
| --- | --- |
| `clang/include/clang/Sema/Sema.h` | `SkipBodyInfo::CxContinuation`; `isCxContinuationOf` |
| `clang/lib/Sema/SemaDecl.cpp` | at the tag-redefinition site, a Cx continuation returns the existing type instead of diagnosing |
| `clang/lib/Sema/SemaCx.cpp` | `isCxContinuationOf`; `findCxMethodDeclaration` and the redeclaration link in `ActOnCxMethodDeclarator` |
| `clang/lib/Parse/ParseDeclCXX.cpp` | a definition body that is a continuation takes the continuation path |
| `clang/lib/Parse/ParseDecl.cpp` | `ParseCxContinuationBody` |

The continuation is routed through the existing `SkipBodyInfo` channel, which
is already how `ActOnTag` tells the parser "this body needs different
handling". `ParseCxContinuationBody` adds members straight to the finished
record, which is what makes "no stored layout change" structural rather than a
rule someone has to remember to check.

Member identity reuses M3's answer: name, parameter types and argument labels.
A matching method gets `setPreviousDeclaration`, so one declaration and its
definition are one entity, Clang's own redefinition check covers a second
implementation, and the consumer links against the symbol the header promised.

## AST and Sema changes

None beyond the redeclaration link. A continuation produces no new node: its
methods are `FunctionDecl`s in the same `RecordDecl` as the primary
definition's.

## Target and runtime requirements

None.

## Tests

`clang/test/Cx/`:

- `continuations.c` with `Inputs/cx-counter.h` and `Inputs/cx-counter-impl.c` —
  a header declares, a separate translation unit implements, a third uses; one
  symbol per declared method, and the consumer references exactly those.
- `continuations-diags.c` — a continuation adding a field, and a member
  implemented twice.
- `continuations-ownership.c` — a type owned by another module and a type from
  an ordinary C header both keep C's redefinition error; a type defined in the
  same owned file is reopenable.

Verification run on this checkout: `clang/test` 48936 passed, 30 expectedly
failed, 0 unexpected failures.

## C collisions

- Every case that is not a Cx continuation still reports C's `redefinition`
  error with its usual note, which `continuations-ownership.c` pins for both a
  foreign C header and another module's type.
- A continuation cannot introduce storage, so no C program's layout or
  aggregate initialization can change.

## Known limitations

- **No access control**, so a helper introduced only in a continuation is
  public rather than private by default, and `private`, `internal` and
  `private(set)` do not exist. M4c.
- ~~**No near-miss diagnostics.**~~ Closed by [M4.2](M4.2.md): a definition
  whose labels or parameter types drifted from the declaration it meant to
  implement is reported where it is written, under `-Wcx-near-miss`.
- **An unimplemented declared member is not reported.** Nothing checks that
  every declared method acquired a definition somewhere in the module; that
  needs the module-wide view of M15.
- **The unowned rule is conservative.** Two files with no module cannot share a
  type's continuation even inside one program. G04 owns the unnamed-module
  rule, and same-file is the conservative reading it proposes.
- **Only structs.** Classes and protocols do not exist yet (M11, M8).
- **No `extension` blocks.** An extension is a separate construct from a
  continuation and is not implemented.

## Runnable demonstration

```sh
clangx -c counter.c -o counter.o     # the continuation defines the members
clangx -c app.c -o app.o             # a third file only sees the header
clangx counter.o app.o -o app && ./app   # 42
```

## Benchmark checkpoint

Not applicable.
