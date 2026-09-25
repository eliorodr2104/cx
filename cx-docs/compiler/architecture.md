# Compiler Architecture

## Scope and baseline

Cx extends the Clang C frontend. It is not a source-to-C transpiler and does not
implement a parallel C parser. Existing C behavior remains the baseline under the
selected supported C/GNU dialect and target.

`clangx` is the current driver name, and `clang -x cx` selects Cx explicitly. Both
interfaces are implemented in this fork. The [M0 record](milestones/M0.md) identifies
the upstream revision, target, build configuration, and validation evidence.

## Pipeline

```text
source buffers
    -> Clang lexing/preprocessing
    -> Parser <-> Sema
    -> source-aware semantic AST
    -> Clang CodeGen
    -> ordinary LLVM IR
    -> LLVM optimization/target code generation
    -> object files
    -> linker
```

Lexing and preprocessing collaborate; they are not necessarily two complete file
passes. Parser and Sema are interleaved. CodeGen can consume completed declarations
incrementally rather than waiting for an entire whole-program AST.

## Reuse rules

Reuse C lookup, declarators, types, diagnostics, and lowering when their semantics
match. Reuse C++/Objective-C machinery selectively, not their language behavior by
accident. A new Cx syntax spelling does not automatically require a new AST node;
a new ownership/control-flow contract may.

Keep Cx routines in dedicated implementation units where useful, but share Clang's
Parser, Sema, ASTContext, and CodeGen state. There is no independent Cx AST universe.
An input-kind enum entry, if convenient to the driver, does not require inventing
Cx11/Cx17/Cx23 as independent languages.

## Ownership of responsibilities

| Layer | Responsibility |
| --- | --- |
| Driver | Language/defaults, target/SDK, build actions, artifact inputs/outputs |
| Preprocessor | Existing C processing and explicit Cx module ownership events |
| Parser | C-compatible syntax recognition, source boundaries, recovery |
| Sema | Types, access, effects, overloads, conformance, ownership validity |
| AST | Typed meaning and useful source distinctions |
| CodeGen | ABI-specific execution, cleanup paths, adapters and metadata |
| Runtime | Refcounts, weak promotion, environment/value operations |
| libcx | Containers, text, ranges, algorithm APIs |

The dedicated `#module` directive is a deliberate preprocessor addition. The old
claim that the preprocessor would receive no semantic additions is superseded.

## Cost model

Frontend syntax does not inherently cost runtime instructions. Methods do not add
instance storage. Captures and existentials have general materialized representations,
but optimization can remove them where legal. No universal inlining, allocation-free
existential, or zero-cost generic guarantee is made.

One static ABI type has one materialized layout. Representation cannot switch between
one/two/five words merely because a different runtime value is assigned.

## Completion and gates

This architecture is sufficient for vertical implementation milestones. It is not
proof that grammar, ownership, ABI, and library edge cases are finished. See
[the issue register](../OPEN-ISSUES.md), [roadmap](implementation-roadmap.md), and
[reference documentation](../SOURCES.md).
