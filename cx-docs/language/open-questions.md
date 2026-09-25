# Remaining Language Decisions

This is a routing index, not a second conflicting specification. The authoritative
register is [OPEN-ISSUES](../OPEN-ISSUES.md).

## Before implementing the affected feature

- **G01:** C grammar collisions: comma/tuple expressions, C23 raw enums, preprocessing
  numbers, trailing closures, and implicit type-name lookup.
- **G02:** exact same-name/optional label behavior, synthesized initializer call shapes,
  and enum payload label rules.
- **G03:** the proposed generalization of `?` beyond classes/String, raw-pointer optional
  exceptions, and optional extraction/chaining syntax.
- **G04:** cross-file continuations, defaults without a module, incomplete types, and
  duplicate-definition/interface rules.
- **G05:** tagged pointer casts, equality, nulls, arithmetic, C boundaries, and schema
  coherence on supported targets.
- **G06:** generic/static/Self protocol requirements through existentials and generic
  implementation dependencies across modules.
- **G07:** ownership contracts, manual-RC spelling/returns, failure/OOM, weak promotion,
  synchronization, resource-copy rules, and non-local C control flow.
- **G08:** stored/computed property addressability, writeback, mutation access, and COW
  mutable views.
- **G09:** module ownership under macro expansion, source qualification, C header identity,
  artifact invalidation, and coherence across libraries.
- **G10:** container naming, String C-conversion lifetime, Unicode indexing/equality,
  and literal inference.
- **G11:** collection/iterator contracts, bounds failure, mutation invalidation, counts,
  and closure effect propagation in algorithms.
- **G12:** enum/OptionSet raw-value policy, recursion, complement universe, exhaustive
  switch/fallthrough, and binary schema compatibility.

## Deliberately future

Class inheritance, a package manager, general reflection/comptime, custom allocator
frameworks, generic value parameters, and a high-level I/O framework are not required
to start the compiler. They are not implicitly accepted under vague "modern C" goals.

## Measurements are not missing semantics

Error-channel ABI selection, inline existential-buffer size, code-size/specialization
tradeoffs, and ARC representation performance are checkpointed in
[compiler/benchmarking.md](../compiler/benchmarking.md). These experiments happen when
implementations exist. They must not be confused with correctness decisions such as
whether a weak load can race with destruction.
