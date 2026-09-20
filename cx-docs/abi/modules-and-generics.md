# Module Artifacts and Cross-Module Generics

## Source interface vs compiler interface

Headers remain the textual declaration interface. Compatible compiler artifacts can
carry checked generic bodies, conformance descriptors, metadata/layout information,
and implementation dependencies for downstream code generation. They are generated
build products, not an additional source extension users must maintain.

An artifact does not automatically add unseen public functions to ordinary source
lookup unless a future explicit interface-loading rule permits it. `#include` remains
the source mechanism.

## Generic implementation availability

```c
// algorithms.h
#module Algorithms
T maximum<T: Comparable>(T a, T b)
```

The body can reside in `algorithms.c`. Downstream specialization then requires the
matching artifact/body information or a supported shared generic entry. Without
those inputs, the compiler cannot invent machine code for an unknown body.

A library exposing open-ended generics may need to distribute code-bearing compiler
metadata in addition to headers and binary libraries. Such artifacts can expose
implementation information even when they are not textual source; this is not a
promise of secrecy or of cross-compiler-version compatibility.

## Internal dependencies

A public generic may depend on internal functions/types. Correct specialization needs
serialized transitive implementation material, ABI-reachable helper entries, or a
shared implementation strategy. These details can remain inaccessible to source
lookup while being available to the compiler. "Only public declarations are serialized"
is insufficient as a universal rule.

## Build identity

Artifacts include module identity; compiler-format and ABI versions; target/data layout;
selected C/Cx modes; relevant build flags/macros; header/dependency fingerprints; and
conformance information. Macro-dependent headers cannot share stale semantic results
merely because their path is unchanged.

Module assignment comes from physical source directives and explicit build mappings.
An included foreign header can have a different owner from the root compilation.
Unowned C headers retain their original C identity.

## Scheduling and duplicates

Module assembly requires a deliberate driver/build step. Running `clangx a.c b.c` is
not automatically a shared-AST compilation. Parallel/incremental production must
avoid partial or competing artifacts and invalidate dependent specializations.

Identical specializations can be deduplicated with target-appropriate linkage or a
specialization cache. Shared generic implementations are a supported option where
runtime-generic witnesses require them or code-size benefits justify them.

## Access and coherence

Artifact identity supports `internal` and owner-extension rules but is not a security
capability. Public retroactive conformances must be discoverable for whole-build
conflict checks. Conflicting module names/packages and dynamically added conformances
need explicit policies rather than link-order precedence.
