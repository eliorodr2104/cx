# Frontend Pipeline

## Case study: independent inferred declarations

```c
var count = 42,
    ratio = 0.5
```

Macro expansion has already had the opportunity to replace `var`. If it remains an
identifier, the parser consults the current context and C type-name lookup. A visible
C typedef called `var` wins over inferred-declaration syntax.

For the Cx interpretation, parse declaration specifiers, each complete declarator,
and its initializer. The deduced placeholder belongs independently to each declarator:
`count` becomes `int`, `ratio` becomes `double`. Do not mutate one shared deduced type
and accidentally impose it on the next declarator.

Sema builds normal variable declarations where possible. Source type information
retains that `var` was written; CodeGen sees the resolved types. `let` shares deduction
and adds the approved top-level const/binding contract.

## Information retained at each stage

A parameter `int width newWidth` needs a real local declaration named `newWidth`
and the semantic external label `width`. The label is not disposable parser trivia.
A call retains source label locations but emits arguments according to the selected
function's resolved ABI.

`try`, `defer`, closures, property writeback, and explicit existential construction
retain semantic nodes/metadata until the relevant lowering. Rewriting them into
ordinary C syntax during parsing would lose language-specific diagnostics and cleanup
meaning.

## Modules during preprocessing

`#module Geometry` registers ownership for its physical file. Include entry/exit and
macro expansion retain enough provenance for access/linkage decisions. An included
C header does not inherit its consumer's module merely because it is on the same
token stream. `-E` output and preprocessed-input support need ownership-preserving
serialization or explicit limitations.

## Semantic and implementation boundaries

Sema establishes which function/property/protocol requirement is used, what conversions
are legal, and what initialization/effects are required. CodeGen implements those
decisions using target ABI lowering. It does not solve overload ambiguity by choosing
the first function it encounters.

LLVM receives operations such as loads, stores, calls, branches, aggregate accesses,
and ownership-runtime calls. It does not need a `CxClass` or `CxTry` instruction.

## Source fidelity

Retain source locations for `var`/`let`, labels, nullable/optional spellings, named vs
positional tuple access, and generated constructors/accessors. This allows diagnostics
to point to user syntax while CodeGen operates on normalized semantics.
Clang implementation references are collected in [SOURCES](../SOURCES.md).
