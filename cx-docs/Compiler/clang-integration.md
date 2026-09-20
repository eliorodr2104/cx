# Clang Integration

## Baseline inspection

Before patching, record the actual checkout SHA and build configuration. Documentation
names below are architectural targets; confirm current APIs in that checkout rather
than assuming an online latest branch exactly matches it.

## Driver and frontend state

Extend the existing driver to recognize the Cx invocation and explicit Cx input.
Represent the frontend as the selected C dialect plus Cx capability. An additional
input-kind enum is acceptable where the driver needs it; it must not duplicate the
entire C standard matrix.

Propagate the mode through CompilerInvocation and LangOptions. Register it as
semantic/PCH-relevant rather than an ignorable optimization option. Pure C invocation
must remain unaffected.

## Map of implementation areas

| Area | Expected work |
| --- | --- |
| `clang/lib/Driver` | Invocation mode, input selection, artifact/bridge actions |
| `clang/lib/Frontend` | Option parsing/round-trip, action setup, serialization inputs |
| `clang/include/clang/Basic` | Options, diagnostic definitions, target-facing queries |
| `clang/lib/Lex` | `#module`, provenance/events, gated lexical support |
| `clang/lib/Parse` | Contextual syntax and continuation/member parsing |
| `clang/lib/Sema` | Cx rules using shared Clang semantic state |
| `clang/include/clang/AST`, `clang/lib/AST` | Types, nodes, ownership/access metadata |
| `clang/lib/Serialization` | New AST forms and compatibility validation |
| `clang/lib/CodeGen` | Lowering, ownership, thunks, witnesses, C exports |
| `clang/test/Cx` | Planned Cx-focused regression tree |

Dedicated `ParseCx*.cpp`, `SemaCx*.cpp`, and `CGCx*.cpp` files can organize additions.
Do not invent a second Parser/Sema just to keep files physically separate.

## Module directive

The preprocessor recognizes `#module` and records file ownership, not a macro token
replacement. SourceManager provenance and include transitions are required. Exact
macro-expansion ownership, `-E` round-trip representation, header guards, and cache
keys are G09.

Build module assignment applies to the primary source and explicitly mapped headers.
It must not relabel arbitrary system/foreign C headers. A mismatched foreign header
owner is normal; a conflicting assignment to the same source is an error.

## AST integration checklist

New nodes affect more than parsing: visitors, dumping/printing, serialization/import,
structural equivalence, profiling/canonical types, template/generic dependency flags,
constant evaluation where relevant, CodeGen dispatch, debug information, and tooling
must be reviewed. A file compiling once does not establish complete integration.

## ARC and optimizer reuse

Study Objective-C ARC and cleanup machinery, but do not feed custom Cx runtime calls
into Objective-C-specific optimization assumptions without a semantic proof. Clang
ARC support does not automatically implement Cx weak side tables or ownership ABI.

## Tooling and generated output

Use the same AST/diagnostics for clangd and command-line tools. New grammar support
requires explicit tooling/formatter work. Generated C headers, artifacts, and tests
are build outputs; compiling never rewrites checked-in user headers.

See [SOURCES](../SOURCES.md) for official Clang architecture references.
