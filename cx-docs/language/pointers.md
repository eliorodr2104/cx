# Raw Pointers

A raw `T*` keeps selected C semantics. It can be null, can alias manually managed
memory, and does not automatically retain or free what it points to.

```c
int* pointer = null
int* legacy = NULL;
```

`null` is a contextual typed null-pointer spelling; legacy `NULL` and C's `nullptr`
where supported retain their existing meaning. `nil` is not a raw-pointer synonym.
No required non-null pointer type or `unsafe` scope is introduced.

## Lifetime and representation

Allocation, casts, arithmetic, comparisons, address-taking, dereference, atomics,
and volatile operations remain subject to C and the selected implementation.
A non-null pointer is not a live-allocation proof. Inspecting a freed pointer is not
a portable allocation-state query; do not promise that all invalid pointer values
can safely be compared.

Class references and C pointers are different static concepts. In particular,
`User*` must not silently mean "manual ownership of User". Addressing a reference
slot and obtaining an object's raw address need a separate interop definition.

`void*` remains available. The earlier idea of `Any` as an alias/opaque value has not
been finalized and is not used to replace it in these documents.

## Member access

`pointer->field` remains C. The implicit struct-method receiver permits `self.field`
without establishing a general `.` shorthand for arbitrary raw pointers.
That general shorthand remains optional design work.

PointerTag operations are an explicit Cx capability. Their auto-masking does not
make an external C library aware of tags. See [Pointer Tagging](pointer-tagging.md).
