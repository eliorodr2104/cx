# Documentation Validation

Last reviewed: 2026-09-25.

The checks below cover Cx documentation in this repository. They do not replace
compiler, runtime, ABI, or performance testing.

## Scope

The editorial scope is the repository README and every Markdown file under
`cx-docs`. Upstream LLVM documentation outside that directory remains under the LLVM
project's own editorial process.

The review checks that Cx documentation:

- is written in English
- uses direct, respectful, and technically precise language
- contains no em dashes, emoji, chat references, or delivery-process notes
- distinguishes implemented behavior from proposed or gated work
- uses stable names for Cx, Clang, LLVM, commands, files, and language constructs
- keeps code examples, commands, links, headings, and fenced blocks structurally valid

## Status Rules

[Milestone records](Compiler/milestones/README.md) are the source of truth for
implemented compiler behavior. Design chapters describe the intended language and can
include future work. The [decision register](DECISIONS.md) labels accepted,
provisional, gated, and deferred choices. The [open design gates](OPEN-ISSUES.md)
identify contracts that must be settled before their features can be completed.

No document may infer implementation status from design completeness alone.

## Structural Checks

Run these checks after documentation changes:

```sh
git diff --check -- README.md cx-docs
```

Check relative Markdown links and heading anchors with the repository's documentation
checker when available. If no checker is configured, inspect changed links directly
and verify every changed file has one top-level heading and balanced code fences.

The editorial audit also searches for forbidden punctuation and authoring artifacts.
It must ignore code tokens whose spelling is part of the language or an external ABI.

## Technical Evidence

Documentation can cite the following evidence:

- focused tests in `clang/test/Cx`
- milestone-specific build and test commands
- AST, IR, diagnostics, and object-symbol inspection
- separate compilation and C interoperability tests
- benchmark results that include hardware, operating system, compiler revision,
  flags, input data, and variance

Cross-compilation proves that the compiler produced an output for a target. It does
not prove that the output ran correctly. Sanitizer success adds evidence but does not
prove memory or thread safety.

## Checks Outside This Editorial Pass

The 2026-09-25 rewrite did not run the Clang test suite, runtime race tests, ABI
compatibility tests, generated C header builds, or performance benchmarks. It changed
documentation only. Compiler claims continue to rely on the evidence recorded in each
completed milestone.

The documentation does not claim a frozen grammar, stable public ABI, complete memory
safety, or production readiness. Those claims require the feature-specific work in
the [testing strategy](Compiler/testing.md) and
[benchmark checkpoints](Compiler/benchmarking.md).
