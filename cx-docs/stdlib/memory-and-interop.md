# Memory, C Interoperability, and Library Boundaries

## libc remains the foundation

Cx keeps ordinary C allocation, byte/string APIs, files, threads, and platform headers.
The standard library initially supplements them with values and protocol abstractions.
A high-level I/O or allocator framework can wrap C later; it is not required to begin
Cx adoption.

A C ABI function's shape does not itself reveal ownership, pointer lifetime, mutability,
or whether it stores a callback. Bridges need documented contracts.

## Allocation failure

Array/String/Dictionary/Set growth and boxed environments can allocate. The default
OOM behavior is not yet a complete language decision. Choose a coherent policy
before presenting append/construction as unconditionally non-throwing or infallible.
Recoverable allocation, trapping allocation, and freestanding custom hooks should
not be mixed by undocumented exceptions.

## Raw access

Span and raw `.data` operations do not retain sources. COW mutable buffer access must
establish exclusivity and uniqueness for its full valid interval. String-to-C
conversion must handle NUL termination, embedded NUL behavior, and pointer lifetime.
No `unsafe` keyword is needed to document those preconditions clearly.

## Runtime-free use

Pure C and trivial Cx features should work without automatically linking a Unicode
or ARC runtime. A class/escaping closure/existential use can introduce its required
runtime dependencies explicitly through the build. Freestanding availability and
allocation hooks are implementation gates, not assumed properties of hosted libc.

## Wrappers and migration

Prefer thin Cx wrappers or protocol conformances over rewriting a proven C library
just for syntax. Preserve the C core as a dual-build baseline when that is the goal.
Measure changes in generated code, allocation, semantics, and performance during
implementation. Do not equate a prettier source API with automatically faster code.

A representative C dictionary is a planned case study. No port is recorded here.
