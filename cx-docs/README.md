# Cx Documentation

Cx extends Clang's C frontend with opt-in language features while preserving C source
files, preprocessing, toolchain integration, and interoperability. These documents
define the language, explain the compiler work, and separate implemented behavior
from planned features.

Start with the language guide if you want to write Cx. Use the compiler and milestone
documents when changing the frontend.

## Start Here

1. [Language Guide](language/README.md) describes Cx syntax and semantics.
2. [Milestone Records](Compiler/milestones/README.md) lists implemented compiler work.
3. [Implementation Roadmap](Compiler/implementation-roadmap.md) shows dependencies and
   later milestones.
4. [Decisions](DECISIONS.md) records accepted directions and provisional contracts.
5. [Open Design Gates](OPEN-ISSUES.md) lists decisions required before specific
   features can ship.

## Documentation Areas

| Area | Use It For |
| --- | --- |
| [Language](language/README.md) | Syntax, type rules, source behavior, and C compatibility |
| [Compiler](Compiler/README.md) | Clang integration, analysis, lowering, testing, and milestones |
| [Design](design/README.md) | Design choices, alternatives, consequences, and unresolved gates |
| [ABI](abi/README.md) | Values, calls, symbols, errors, C bridges, and build artifacts |
| [Runtime](runtime/README.md) | Ownership, weak references, initialization, and cleanup |
| [Standard Library](stdlib/README.md) | Collections, text, ranges, protocols, Optional, Result, and Span |
| [Development Helpers](dev/README.md) | Local commands for building, testing, and editor integration |

The capitalized `Compiler` directory is retained for path compatibility. Use its exact
case in links and scripts.

## How to Read Status

Milestone records describe compiler behavior that has been implemented and tested in
this fork. Design chapters can also describe later milestones. When the two differ,
the milestone record is the source of truth for current implementation status.

Code examples in future-facing chapters illustrate the intended design. A chapter
must say when syntax or behavior is proposed, gated, or incomplete. Performance
claims require a recorded benchmark; correctness and lifetime rules do not depend on
benchmark results.

## Reference Documents

- [Change History](CHANGELOG.md)
- [Documentation Validation](VALIDATION.md)
- [Technical Sources](SOURCES.md)
