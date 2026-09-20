# 0004 - Modules Without Replacing Include

## Status

Accepted `#module` direction; build/provenance details remain gated.

## Context

Internal access needs an owner shared by several translation units. Header ownership must not become the module of whichever source happens to include it.

## Decision

Use optional `#module Name` ownership per physical source/header and keep `#include`. Build metadata may assign the root source and explicitly mapped headers. Imported foreign owners are allowed. Plain C headers remain C rather than inheriting the consumer module.

## Consequences

Modules support internal access, same-owner extension privileges, canonical identities, linkage defaults, and artifact association. They are not a package manager or security boundary. Same-named public declarations still need a source lookup/qualification rule; linker names alone do not solve it.

## Implementation gates

G09: macro expansion ownership, `-E` and PCH round trips, header guards/conditional configuration, unnamed files, root/build conflicts, source qualification, and cache keys.

## Related contracts

[Modules](../language/modules.md), [Module artifacts](../Compiler/module-artifacts.md), [Module ABI](../abi/modules-and-generics.md).
