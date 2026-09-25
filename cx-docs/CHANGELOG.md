# Documentation Change History

This file records changes to the Cx documentation. Compiler implementation changes
belong in the [milestone records](compiler/milestones/README.md).

## 2026-09-25

### Enum Design

- Settled the Cx enum design: a body that uses `case` selects a Cx enum, with raw and
  simple enums, payload enums, exhaustive switches without fallthrough, and option sets.
- Updated the G01, G02, and G12 gates, the decision register, the roadmap, the enum ABI
  notes, and every enum example to the `case` syntax.

### Editorial Review

- Replaced the upstream LLVM README with a Cx project overview, motivation, build
  instructions, a runnable example, test guidance, and direct links to the
  documentation.
- Rewrote the documentation index around reader goals and current implementation
  status.
- Split the milestone index into features and hardening passes.
- Renamed the `Compiler` directory to `compiler`.
- Tightened status language so design proposals do not read as implemented features.

### Technical Corrections

- Updated compiler documentation to recognize that `clangx` and `clang -x cx` are
  implemented rather than planned interfaces.
- Made milestone records the authoritative source for implementation status.
- Removed the file manifest and the validation register, which duplicated the
  milestone records.

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
