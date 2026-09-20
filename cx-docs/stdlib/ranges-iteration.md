# Ranges, Iterators, and Collections

## Ranges

`Range<T>` is the working half-open value type behind `a..<b`; `ClosedRange<T>` is
behind `a...b`. Bounds are values, not an allocated list of every element.

Integer iteration advances under a checked termination rule. A closed range ending
at the maximum integer must terminate without overflowing after the last element.
Reversed ranges, empty ranges, signed/unsigned mixing, and explicit steps need an
agreed API. Do not silently use wrapping or an infinite loop.

## Iterator

```c
protocol Iterator {
    type Element
    Optional<Element> next()
}
```

A present element advances iteration; absence terminates it. Mutating next state is
normal. Whether repeated next after exhaustion is always absent or has another
precondition must be finalized in G11.

Iterable provides an associated iterator type conforming to Iterator, whose Element
matches the iterable's Element, and an iterator-construction operation. Use the
normal associated-type/same-type constraint engine rather than a special for-loop
collection whitelist.

## For-in and enumerated

```c
for (value in values) {
    consume(value)
}

for ((value, index) in values.enumerated()) {
    consumeIndexed(value, index)
}
```

Enumerated yields value first, index second. Standard finite-container indices/counts
use a suitable `size_t`-based contract, not an assumed printf `%d` int. Iterator state
and Optional results can optimize away without changing their semantic behavior.

## Collection capabilities

Collection adds Index, start/end indices, index advancement, count, and read subscript.
MutableCollection adds a writable/modify-capable element contract. RandomAccessCollection
adds distance/offset operations with explicit constant-time expectations. Not every
iterable is multipass, indexable, finite, or O(1)-counted.

A String grapheme collection cannot pretend ordinal indexing has the same complexity
as a contiguous byte span. A Set/Dictionary iterator need not promise stable order.

## Algorithm contracts

Shared protocol extensions can implement filter/map/search/reduce and adapters.
Eager results need owning storage. Lazy adapters preserve source lifetime dependencies.
Throwing callbacks/iterators require effect propagation rules; no ad hoc swallowing
of errors. Reference iteration, invalidation during mutation, and callback mutation
of the iterated container are G11.
