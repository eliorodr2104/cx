# Memory and Ownership

Cx separates value semantics, reference ownership, and raw addresses.

## Value types

Structs, tuples, and payload enums copy values. Managed members participate in
synthesized copy/destruction; raw C pointers do not. Only an enum's active payload
is alive and participates in ownership operations.

A value containing an ARC reference is not a deep copy of the referenced object.
Library COW values preserve logical value independence while sharing buffers when
legal.

## ARC classes

An ordinary class uses strong ARC ownership. The compiler supplies ownership
operations at initialization, assignment, return/capture boundaries, and lifetime
end. Optimization may remove redundant operations, not observable lifetime effects
for resources whose language contract requires them.

For assignment, acquire/preserve the incoming value before releasing an old value
when self-assignment or aliasing can occur. Do not implement `release(old)` followed
by `retain(new)` blindly.

## Manual reference counting

The declaration spelling remains undecided. The accepted semantic model is:

```c
// Foo denotes a class using the not-yet-finalized manual-RC policy.
Foo first  = Foo(...)        // owned +1
Foo second = first.retain()  // owned +1
Foo view   = first           // non-owning alias; no retain

first.release()
second.release()
```

`release()` decrements ownership and destroys at zero. There is no general
`destroy(object)` that ignores other owners. Parameters borrow by default unless
an API explicitly documents/transports ownership.

Return ownership must be visible in declarations or another fixed API contract;
callers cannot infer whether they own a +1 by inspecting an unavailable function body.
The annotation/convention and ownership of MRC fields/captures remain G07.

## Raw pointers

C allocation and pointer operations remain C. `free(p)` does not null every alias;
a non-null pointer is not proof of a live allocation. No `unsafe` block is required.
Raw handles in a copying value require an explicit ownership design to avoid double
release/free.

## Weak and synchronization

Weak loads must either acquire a live strong reference or return absence. They
must not hand out a dangling pointer during destruction. Safe weak promotion and
cross-thread strong ownership require correctness rules before performance tests
choose inline counters, atomics, or side-table details.

The absence of a tracing collector means strong cycles can leak. Weak references
break suitable non-owning back edges; the runtime does not silently collect cycles.

## Cleanup

`defer`, managed locals, fields, and failed initializers use a common cleanup model.
See [Runtime Ownership](../runtime/ownership.md) and
[Initialization and Cleanup](../runtime/initialization-and-cleanup.md).
