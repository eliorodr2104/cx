# Span Library Contract

`Span<T>` describes contiguous storage as a pointer plus element count. It owns no
allocation, carries no capacity, and does not retain another container's buffer.
`Span<const T>` is the corresponding read-only element view.

## Construction

Known-bound compatible C arrays can supply pointer/count automatically. Pointer
construction needs an explicit count and valid aligned storage for that many elements.
An empty span may use a null pointer only under a defined zero-length contract; its
implementation must avoid dereferencing or performing invalid arithmetic on it.

## Access and operations

Desired operations include `count`, `isEmpty`, indexing, `subspan`, first/last,
`contains`, iteration, and generic collection algorithms. Checked indexing validates
an index against the span count, not the allocation's actual lifetime.

Raw `.data` access remains a C escape hatch. It must not be represented as equivalent
to checked library indexing in documentation.

A subspan preserves contiguity and shares the original lifetime. Eager filter cannot
generally return a Span of only matching elements: the selected elements may not be
contiguous. It returns owning storage, or a separately named lazy adapter.

## Lifetime and invalidation

The caller keeps backing storage alive and stable. Freeing, reallocation, some container
mutations, or destruction of an owning source can invalidate spans. Copying a span
copies the view, not the underlying values.

## COW interaction

A mutable span into a COW Array requires exclusive mutable access and uniqueness
before writing. Uniqueness at the instant a pointer is produced is not sufficient if
the array can later be copied while that pointer remains live.

The recommended initial approach is a scoped mutable-buffer callback: establish
uniqueness, forbid conflicting container access/sharing during the callback, and end
the view's valid mutation interval on return. The exact spelling is not frozen.
An unrestricted escaping mutable `span()` must not be exposed as safe value-semantic
API without a complete alternative contract.

Immutable views also require documented invalidation/lifetime. A normal Span has no
hidden retain; an owning slice would be another abstraction and is not smuggled into
this type.

## Tests

Zero-length views, last-element access, overflow constructing pointer/count ranges,
readonly conversion, subspan boundaries, COW copying during view access, and dangling
sources need explicit tests/contracts. Benchmark B06 follows those semantics.
