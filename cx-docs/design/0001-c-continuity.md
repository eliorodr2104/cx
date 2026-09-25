# 0001 - C Continuity and Incremental Adoption

## Status

Accepted direction; compatibility proof remains feature-by-feature.

## Context

Cx is intended to modernize existing C projects without changing source extensions, rewriting every header, or replacing raw memory access. The existing Clang C frontend is the implementation base.

## Decision

Keep `.c`, `.h`, the selected C/GNU dialect, ordinary preprocessing, C declarations, and C ABI for legacy entities. New syntax is contextual where needed. Explicit Cx constructs may add behavior; they must not silently reinterpret a valid legacy token sequence.

## Consequences

Conservative compatibility is an active design constraint, not merely a compiler flag. There is no required `unsafe` scope. C23 fixed-base enums, comma expressions, and preprocessing-number tokenization expose real conflicts that a keyword policy alone cannot solve. Cx-specific modules and source syntax need not compile with an unmodified C compiler.

## Implementation gates

G01 and G09. Record the actual Clang SHA and selected compatibility modes. Add collision tests with every parser change. No blanket promise is made for arbitrary invalid C accepted after compiler recovery.

## Related contracts

[C compatibility](../language/c-compatibility.md), [Compiler contract](../compiler/compatibility.md).
