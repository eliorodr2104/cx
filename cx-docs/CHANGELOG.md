# Documentation Change History

This file records changes to the Cx documentation. Compiler implementation changes
belong in the [milestone records](Compiler/milestones/README.md).

## 2026-09-25

### Editorial Review

- Replaced the upstream LLVM README with a Cx project overview, build instructions,
  a runnable example, test guidance, and direct links to the documentation.
- Rewrote the documentation index around reader goals and current implementation
  status.
- Removed references to chats, assistants, attached archives, delivery packages, and
  other authoring-process details.
- Applied consistent English wording across project documentation.
- Removed em dashes and checked the documentation for emoji.
- Tightened status language so design proposals do not read as implemented features.
- Applied Apple Human Interface Guidelines writing principles where they fit technical
  documentation: direct language, concise instructions, consistent terms, and clear
  outcomes.

### Technical Corrections

- Updated compiler documentation to recognize that `clangx` and `clang -x cx` are
  implemented rather than planned interfaces.
- Made milestone records the authoritative source for implementation status.
- Reframed validation around the repository contents instead of obsolete archive
  counts and hashes.

## 2026-09-20

### Design Consolidation

- Established the language, compiler, ABI, runtime, and standard-library document
  structure.
- Added decision, issue, source, validation, and roadmap registers.
- Recorded parser, ABI, ownership, weak-reference, generic-witness, C bridge, mutable
  view, and C-string lifetime gates.
- Separated correctness requirements from benchmark checkpoints.

The 2026-09-20 work also corrected several early design assumptions. C23
fixed-underlying enums remain valid C; raw nullable pointers do not provide an extra
Optional niche; unrestricted existential types require one materialized layout; C
callbacks need ABI-correct entries; and public generic artifacts may depend on private
implementation details.
