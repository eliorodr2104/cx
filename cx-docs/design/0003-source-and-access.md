# 0003 - Type Continuations and Access

## Status

Accepted source model; detailed cross-file validation remains a gate.

## Context

The project prefers grouped implementations over C++ qualified method definitions and wants headers to contain the declared stored state. Private helpers should be convenient in both structs and classes.

## Decision

Reopen an already defined owned type to implement members. The primary definition owns all stored fields; continuations/extensions cannot add storage. Primary members default public. Existing implementations inherit declared access; newly introduced helpers default private. Explicit public/internal implementations require visible interface declarations.

## Consequences

Visibility depends on declaration context, not simply filename suffix. A whole private type can still live in one `.c`. Compiler errors can propose header edits, but compilation never mutates user source. Private layout is visible in headers even when Sema prevents member access.

## Implementation gates

G04: incomplete types, multiple continuations, duplicate bodies, near-miss signatures, and unowned translation-unit boundaries. G08: access restrictions must include address-taking/writeback, not just direct assignment.

## Related contracts

[Source model](../language/source-model.md), [Access control](../language/access-control.md).
