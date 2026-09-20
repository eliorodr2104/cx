# Extensions

Extensions add behavior or protocol conformance without changing stored layout.
They are distinct from reopening a type to implement its declared interface.

```c
extension Rect {
    ~mutating float perimeter() {
        return 2 * (self.width + self.height)
    }
}

extension Rect: Drawable {
    ~mutating void draw() {
        renderRect(self)
    }
}
```

## Conditional behavior

```c
extension Array<T> where T: Equatable {
    // Operations requiring element equality.
}

extension Collection where Element == String {
    // Operations specialized to String elements.
}

extension Drawable where Self: Serializable {
    // Behavior requiring both protocols.
}
```

The same constraint engine checks declarations, generics, defaults, and conditional
conformances. `where` is retained as the current spelling. No separate arbitrary
compile-time expression language is implied.

## Access

An extension of a type in its owning module may access that type's private state.
An extension from another module gets only externally accessible members. Visibility
of a declaration is still required: module identity does not automatically import
unseen methods or source files.

No extension adds stored fields. Computed properties, methods, and conformances
can be added under the source/access rules. Public/internal API declarations must
be visible before their out-of-line implementation; compilation does not edit headers.

A continuation of an existing type is owner-module implementation. Foreign modules
must use `extension`, not pretend to be a second primary implementation. The exact
same-module/no-module access details are recorded in [Source Model](source-model.md).

## Dispatch

A protocol-extension method matching a protocol requirement can be a default witness.
A method absent from the requirement list is extension-only behavior. See
[Protocols](protocols.md) for the dispatch distinction and coherence checks.
