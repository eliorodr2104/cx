# Pointer Tagging

Cx supports low-bit tagging on supported pointer representations without a distinct
`TaggedPtr<T>` source wrapper. The static pointer type remains `T*`.

```c
enum NodeTag: PointerTag<Node> {
    marked,
    deleted
}
```

`PointerTag<Node>` implies OptionSet. Each atomic case consumes one independent bit;
the empty set means no flags. No explicit `UInt2` or tag width is required.

## Capacity

The compiler derives the available low-bit capacity from the target's guaranteed
alignment of `T`. For power-of-two alignment A, capacity is `log2(A)`.
Alignment 8 gives three bits/three independent flags/eight combinations. Pointer
width alone does not imply this capacity.

Reject declarations with too many atomic flags. Validate concrete generic
instantiations when their alignment becomes known. Reject unsupported address
spaces/representations instead of assuming every pointer is an integer address.

This type-capacity check is not proof that a pointer produced by arbitrary casts,
packed storage, or integer arithmetic is correctly aligned. Valid live aligned
storage remains a precondition. G05 covers unaligned extensions and provenance.

## Operations

```c
Node* node = getNode()
node.addTag(.marked)
node.addTag([.marked, .deleted])

if (node.hasTag(.marked)) {
    inspect(node.tags())
}

node.removeTag(.deleted)
node.clearTags()
```

Mutation changes the pointer binding/slot, not all aliases or the pointee object.
Mutating a by-value pointer parameter does not update its caller's variable.
The meaning of multi-flag `hasTag` must be fixed as all/any membership before API
release; it is not inferred from an ambiguous name. `tags()` produces the option set.

These operations require no heap allocation. Const pointer bindings cannot be
mutated through them; atomic pointer slots need atomic operations, not a racy
load-mask-store disguised as one method call.

## Dereference

Cx-aware dereference masks the associated low bits before the memory access without
changing the stored tagged representation:

```c
node.addTag(.marked)
var next = node->next
```

The tag on `node` remains. Casting its representation to an integer exposes the bits
on supported targets. High-address tagging is outside this capability.

A complete policy is still required for pointer arithmetic, equality/hash identity,
null-plus-tags, `void*`, unrelated casts, indexing, one-past values, and tag inference
across translation units. Auto-dereference is the selected direction, not evidence
that these operations have already been solved.

## Foreign boundary

Do not pass a tagged pointer to ordinary C, a syscall, or an allocator expecting the
original address. A conservative explicit pattern is:

```c
Node* clean = node
clean.clearTags()
legacyUse(clean)
```

This clears a copy. The original remains tagged. An additional nonmutating clean-view
API may be added after naming is chosen. Freeing must use the original allocation
address and the allocation's actual deallocation contract.

## Coherence

One unambiguous tag schema must describe each associated pointer type across a build.
A conformance in one file cannot silently change the interpretation of pointers in
another file lacking that information. Type ownership, imported schema metadata,
and all affected C-boundary rules are correctness gates before optimization.
