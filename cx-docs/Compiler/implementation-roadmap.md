# Dependency-Correct Implementation Roadmap

## Strategy

Implement vertical slices: source recognition, Sema, AST, diagnostics, lowering, and
relevant tests for one coherent feature. Leave the compiler runnable after each slice.
A whole-program language design does not imply a whole-language first patch.

This revision replaces the old "generics last" sequence. Iterable/Optional/Span,
protocol-associated types, and cross-module specialization depend on generic facilities;
they cannot honestly be called finished before those facilities exist.

## Milestones

| ID | Deliverable | Dependencies / acceptance | Status |
| --- | --- | --- | --- |
| M0 | Pinned baseline, driver mode, `-x cx`, language options | Pure C behavior, driver/PCH tests, actual SHA recorded | **fatto** ([record](milestones/M0.md)) |
| M1 | `var`, `let`, contextual `null` | Keep semicolons initially; independent deduction and const tests | |
| M2 | Module ownership and source identity skeleton | Preserve imported C declarations and unowned headers | |
| M3 | Labels, overload lookup, compound references | Minimal deterministic experimental mangling and C-linkage checks | |
| M4 | Struct methods, `self`, access, continuations, `~mutating` | No stored layout change; trivial generated construction | |
| M5 | Semicolon elision and core syntax disambiguation | G01 rules and regression corpus before broad enablement | |
| M6 | Tuples, payload enums, OptionSet core | Validity/layout, pattern matching, legacy enum preservation | |
| M7 | Unified cleanup and value operations | Defer, copy/destruction, partial initialization tests | |
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
