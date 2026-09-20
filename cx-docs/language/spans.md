# Span

`Span<T>` is a non-owning contiguous view supplied by `libcx`.
Its logical representation is a data pointer and an element count, not ownership or
capacity. It does not allocate, grow, or keep another object alive.

```c
int values[4] = { 1, 2, 3, 4 };
Span<int> view = values
```

A known-bound C array may convert without allocation. A raw pointer alone cannot
supply a length:

```c
Span<int> view = Span(pointer, count)
```

## Access

Standard span indexing is bounds-checked by default:

```c
int value = view[index]
view[index] = value + 1
```

The optimizer may eliminate proved-redundant checks. Access through `view.data[index]`
uses raw C pointer semantics. Bounds checks do not validate whether the underlying
allocation is still alive.

`Span<const T>` is the read-only element view. `let Span<T>` would freeze the view
binding, not by itself establish deep immutability of pointed-to values; exact
subscript receiver/writeback behavior must follow G08.

## Library behavior

Desired operations include count/emptiness, indexed access, subspan, first/last,
contains, iteration, and generic algorithms. Eager `filter` generally produces owning
storage because matching elements need not remain contiguous. A lazy adapter is a
different, explicit abstraction.

A span returned from a COW container needs lifetime and mutation restrictions. COW
sharing does not automatically make arbitrary escaping mutable spans safe.
See [Span library contract](../stdlib/span.md).
