# Dependency-Correct Implementation Roadmap

## Strategy

Implement vertical slices: source recognition, Sema, AST, diagnostics, lowering, and
relevant tests for one coherent feature. Leave the compiler runnable after each slice.
A whole-program language design does not imply a whole-language first patch.

This revision replaces the old "generics last" sequence. Iterable/Optional/Span,
protocol-associated types, and cross-module specialization depend on generic facilities;
they cannot honestly be called finished before those facilities exist.

## Milestones

| ID | Outcome | Dependencies / acceptance | Status |
| --- | --- | --- | --- |
| M0 | Pinned baseline, driver mode, `-x cx`, language options | Pure C behavior, driver/PCH tests, actual SHA recorded | **Completed** ([record](milestones/M0.md)) |
| M1 | `var`, `let`, contextual `null` | Keep semicolons initially; independent deduction and const tests | **Completed** ([record](milestones/M1.md)) |
| M1.1 | Separate `auto`, `__auto_type`, `var` and `let` in the AST | Four distinct placeholder spellings; no C behavior change | **Completed** ([record](milestones/M1.1.md)) |
| M2 | Module ownership and source identity skeleton | Preserve imported C declarations and unowned headers | **Completed** ([record](milestones/M2.md)) |
| M3 | Labels, overload lookup, compound references | Minimal deterministic experimental mangling and C-linkage checks | **Completed** ([M3a](milestones/M3a.md), [M3b](milestones/M3b.md), [M3c](milestones/M3c.md)) |
| M4 | Struct methods, `self`, access, continuations, `~mutating`, initializers | No stored layout change; trivial generated construction | **Completed** ([M4a](milestones/M4a.md), [M4b](milestones/M4b.md), [M4c](milestones/M4c.md), [M4d](milestones/M4d.md), [M4e](milestones/M4e.md), [M4f](milestones/M4f.md), [M4.1](milestones/M4.1.md), [M4.2](milestones/M4.2.md), [M4.3](milestones/M4.3.md)); hardening [M4.4](milestones/M4.4.md)–[M4.7](milestones/M4.7.md) completed |
| M5 | Semicolon elision and core syntax disambiguation | G01 rules and regression corpus before broad enablement | ([M5a](milestones/M5a.md), [M5b](milestones/M5b.md) completed; hardening [M5.1](milestones/M5.1.md) completed) |
| M6 | Tuples, payload enums, OptionSet core | Validity/layout, pattern matching, legacy enum preservation | **Completed** ([M6a](milestones/M6a.md), [M6b](milestones/M6b.md), [M6c](milestones/M6c.md), [M6d](milestones/M6d.md), [M6e](milestones/M6e.md), [M6.1](milestones/M6.1.md)) |
| M6.2 | Enum and recovery hardening | Expected type for payload arguments (`.wrap(.red)`); no `goto` into a Cx switch clause; error recovery that stops at a line break instead of the next `;` | |
| M7 | Unified cleanup and value operations | Defer, copy/destruction, partial initialization tests | **Completed** ([M7a](milestones/M7a.md), [M7b](milestones/M7b.md), [M7c](milestones/M7c.md)) |
| M7.1 | Field defaults that read earlier fields | Declaration order; only earlier defaulted fields | |
| M7.2 | Moving resource locals | `return a` and passing a local consume it; no use afterwards, no `deinit` on that path | |
| M8 | Protocol requirements and static conformance | Defaults, refinement, associated types, access/coherence | |
| M9 | Checked type generics within one compilation context | Constraints, same-type relations, concrete specialization | |
| M10 | Optional/Span and iteration primitives | Iterator protocol, ranges, for-in/destructuring; G03/G11 | |
| M11 | Class runtime and strong/weak ownership baseline | Correct refcount state machine and failure model first | |
| M12 | Callable types, noncapturing then capturing closures | ABI adapters; stack vs heap context lifetime; G07 | |
| M13 | Typed/general errors and throwing callables/init | Error protocol, cleanup, experimental ABI, generated C bridge | |
| M14 | Computed properties, subscripts, writeback, COW support | G08; no duplicate evaluation or writable shared aliases | |
| M15 | Separate module build artifacts and generic bodies | G09; private dependencies, serialization, incremental cache | |
| M16 | Existentials and dynamic witness invocation | Uniform ABI, boxed value semantics, generic/static witness gates | |
| M17 | PointerTag capability and all boundary cases | G05; target support, schema coherence, C interop tests | |
| M18 | Text, containers, algorithms, library integration | String/StaticString, Array naming decision, dictionary/Set | |
| M19 | Tooling, documentation, portability and release audit | Feature matrix, artifacts, C regression case study, measured checkpoints | |

These are dependency groups, not a schedule or a promise of one patch per row. A
milestone may be split; runtime foundations can progress in parallel after their
contracts are fixed. Do not call a feature complete by quietly special-casing its
only example or disabling its interaction tests.

Status values are recorded per milestone under [milestones/](milestones/). An empty
status means not started.

## Gated work

Resolve the relevant semantic gate immediately before that feature. For example,
COW mutation rules precede mutable Span export; a weak-reference synchronization
contract precedes a weak-load performance comparison. Benchmarks select correct
implementations, not language meaning.

M11 can reuse runtime mechanisms for M12 environments. M13 can start with concrete
errors; general `any Error` is completed when the existential machinery is ready.
M16 must explicitly reject unsupported generic/static existential calls until their
entry ABI is implemented rather than claiming universal POP support early.

## Module artifacts and generics

Single-translation-unit generics are a bootstrap milestone, not a permanent requirement
to put every public generic body in headers. M15 completes the accepted `.h` declaration /
`.c` definition workflow. Plain multi-file driver invocation is not enough by itself.

## Per-milestone record

Each implementation ticket records: source contract; permitted baseline C modes;
AST/Sema changes; target/runtime requirements; positive/negative tests; C collisions;
known limitations; benchmark checkpoint if relevant; and a runnable demonstration.

No compiler release number, benchmark result, or user-repository state is invented
by this roadmap.
