# Module Build and Artifact Integration

## Explicit build product

A Cx module artifact is a compiler interface, not a user-written source file and not
a runtime object. It can preserve declarations, conformances, generic bodies, and
layout/effect/access data needed by downstream compilation.

The source mechanism remains `#include`. Module ownership does not automatically load
all private definitions or find implementation files by filename guessing.

## Build phases

A viable build needs explicit steps for interface discovery, validation/type checking,
implementation-body availability, artifact assembly, and code generation. The exact
scheduling is a prototype decision. Ordinary Clang compiles translation units
separately even when several `.c` arguments appear in one driver command.

Generic bodies in `.c` require an artifact, a supported shared generic entry, or
another explicitly provided compilation context. Missing required information must
produce a rebuild/artifact-path diagnostic, not a link-time mystery.

## Dependencies

A public generic may call an internal helper. Exported specialization information must
preserve that dependency through serialized helper material, an ABI-callable internal
entry, or an equivalent correct scheme. Keeping implementation data in an artifact
does not make it public for source lookup, and hiding every internal dependency
unconditionally makes specialization impossible.

## Cache key and validation

Include compiler/module format/ABI versions, target and data layout, C dialect/Cx mode,
relevant flags, macro/header fingerprints, module identity, and dependency artifact
identities. Changed `#module` ownership or a different preprocessing configuration
can invalidate semantic data even if the visible filename is unchanged.

Write artifacts atomically and handle parallel producers deterministically. Reject
incompatible/stale artifacts with useful diagnostics. The initial serialized format
is compiler-private, not a long-term distribution promise.

## Headers and access

A source file's build-assigned module does not assign that module to every included
header. Foreign and system C declarations must keep their ownership/linkage. Preserve
physical ownership and macro expansion provenance through PCH/serialized AST.

See [Module ABI](../abi/modules-and-generics.md) and
[Module design rationale](../design/0004-modules-and-ownership.md).
