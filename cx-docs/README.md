# Cx Design Documentation

**Revision:** 2026-09-20 consolidated draft.

This is the current Cx design baseline: updated language/compiler documentation plus
source-model rationale, ABI/runtime contracts, and a standard-library proposal.
Everything is written in English. The canonical language name is **Cx**.

The pack is sufficient to begin incremental compiler work. It is not a claim that
Cx is already implemented, that its grammar/ABI is fully frozen, or that every later
assistant suggestion has been approved by the user.

## Start here

1. [Decisions and status](DECISIONS.md): accepted design, baseline representation,
   proposals, implementation gates, and deliberately future work.
2. [Open issues by feature](OPEN-ISSUES.md): decisions needed before the affected
   implementation, not prerequisites to starting M0.
3. [Implementation roadmap](Compiler/implementation-roadmap.md): dependency-correct
   vertical slices and their acceptance requirements.

## Areas

| Directory | Purpose |
| --- | --- |
| [language](language/README.md) | Source forms and language meaning |
| [Compiler](Compiler/README.md) | Clang integration, analysis, lowering, tests, milestones |
| [design](design/README.md) | Why the selected direction was chosen; consequences and gates |
| [abi](abi/README.md) | Materialized values, calls, symbols, errors, C bridges and artifacts |
| [runtime](runtime/README.md) | Ownership, weak references, construction and cleanup |
| [stdlib](stdlib/README.md) | Optional/Result, Span, text, collections and algorithms |
| [dev](dev/README.md) | Local shell helpers for building and running the fork |

`Compiler` intentionally keeps the capitalized folder name requested for the existing
pack. Other directory names are lowercase. Internal links use that exact case.

## Installation

The archive contains a top-level `cx-docs/` directory. Extract it alongside or merge
it into the existing `cx-docs` in the repository. Save/commit current local documents
before replacement. The pack does not modify compiler source or the local repository.

All 38 baseline Markdown filenames in language/Compiler are retained and updated.
A separate changes diff uses paths relative to `cx-docs`; local edits may require a
manual merge. `manifest.json` records source/archive checksums and output file hashes.

## Reading examples

Code snippets describe the target Cx design. They are not asserted to compile today.
Small snippets may omit surrounding declarations/includes. Proposed or gated syntax
is identified in its chapter. Ordinary C examples retain C meaning.

Benchmarks are [implementation checkpoints](Compiler/benchmarking.md); no performance
results are invented. Correctness-sensitive lifetime/grammar decisions are gates, not
questions left to a benchmark.

[CHANGELOG](CHANGELOG.md) summarizes changes and corrections.
[VALIDATION](VALIDATION.md) explains checks and limitations.
[SOURCES](SOURCES.md) collects primary technical references.
