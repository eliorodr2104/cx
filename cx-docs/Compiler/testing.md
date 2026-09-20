# Testing Strategy

## Layers

Use the existing LLVM/Clang regression infrastructure and add Cx coverage at the
first layer where a behavior is observable. Parser tests still normally run a
frontend with Sema, so supply necessary declarations rather than expect arbitrary
undeclared examples to stop cleanly at syntax recognition.

| Area | What must be checked |
| --- | --- |
| Driver | Input kinds, explicit override, dialect, module options, output actions |
| Parser | Positive/negative syntax and C collision/recovery cases |
| AST | Semantic entities, labels/effects, generated origins, serialization |
| Sema | Type/access/effect/conformance/initialization acceptance and rejection |
| Diagnostics | Ranges, relevant notes, useful candidates, conservative fixes |
| CodeGen | ABI calls, cleanup paths, direct dispatch, representation invariants |
| Execution | Observable lifetime, errors, copies, mutation, iteration |
| Compatibility | Unchanged C parsing, behavior, headers, ABI and linking |
| Artifacts | Separate builds, cache keys, ownership, stale/missing dependency errors |

## LLVM tools

Use `lit` to drive tests, Clang `-verify` for expected diagnostics, and FileCheck for
selected IR/AST/output properties. Avoid full fragile text snapshots when a small
property check is sufficient. Test commands using `-x cx` work from M0 onwards; the
Cx regression tree lives in `clang/test/Cx`.
See the official references in [SOURCES](../SOURCES.md).

## Differential C tests

Compile matched C inputs with the pinned baseline mode and Cx mode under equivalent
options. Classify feature-macro differences and implementation extensions. Compare
acceptance for valid supported C, and execution only for defined deterministic cases.
Identical assembly is not the correctness oracle.

Cover multiple C/GNU dialects and representative target triples. Cross-target IR
checks do not require executing foreign binaries; execution tests need a real supported
runner/emulator. Do not report them passed just because cross-compilation succeeds.

## High-risk interaction tests

- Multiple independent `var` declarators and retained pointee const.
- Comma expressions, fixed-underlying C23 enums, range preprocessing tokens.
- Labels on definitions, callbacks, compound references, generics, and default args.
- Continuations split across files; private/internal access; original C header identity.
- Setter restrictions, subscript compound assignment, and index evaluation exactly once.
- Custom init suppression, default order, failure after every initialized field.
- Copy/self-assignment of ARC-containing structs/enums and COW mutable access.
- Weak promotion racing last release; no access to deinitializing objects.
- Escaping closures and conversion to genuine C ABI callbacks.
- Existentials with class, small value, large value, and over-aligned payloads.
- Generic requirements through `any`, overlap/coherence, and helper dependencies.
- Tagged pointer nulls/casts/arithmetic/atomics/foreign calls on supported targets.
- Generated throwing C bridge used from a separately compiled pure C translation unit.

## Completion rules

A fix brings a test that failed before it. A feature has positive, negative,
compatibility, diagnostic, AST, lowering, and runtime tests as relevant. Cross-file
features need separate compilation tests, not one concatenated source file.

Use sanitizer-enabled runtime tests where supported. Sanitizer success is additional
evidence, not proof of semantics or thread safety.

## Performance and claims

Performance experiments are attached to implementation milestones in
[Benchmarking](benchmarking.md). Unit/regression tests can enforce deliberate invariants
such as no method storage in a struct. Do not claim that all optimized closure calls
must always inline.

This documentation pack itself was checked as documents, not compiled/executed as a
Cx implementation. See [VALIDATION](../VALIDATION.md).
