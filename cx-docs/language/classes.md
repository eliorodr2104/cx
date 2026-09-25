# Classes

Classes represent object identity. A class binding contains a reference rather than
a value copy of the object's stored fields. Ordinary `class` uses ARC by default.

```c
class User {
    private String name
    private(set) int id

    init(
        String name newName,
        int    id   newId
    )

    String displayName()
    void rename(String to newName)
}
```

## Implementation

```c
class User {
    init(
        String name newName,
        int    id   newId
    ) {
        self.name = newName
        self.id   = newId
    }

    String displayName() {
        return self.name
    }

    void rename(String to newName) {
        self.name = newName
    }
}
```

The implementation normally resides in a `.c` including the type's header. The
same model permits a private type to be fully declared and implemented in one `.c`.
Fields cannot be introduced by reopening an already defined class.

## Lifetime

```c
User a = User(name: "Mario", id: 42)
User b = a
```

Both references refer to the same instance. ARC balances strong ownership. At the
last strong release, the runtime begins destruction, invokes any custom `deinit`,
destroys managed fields in reverse order, and reclaims storage.

`let` freezes a class binding, not the object. A class method is not automatically
read-only because the caller's binding is `let`.

## Absence and weak references

`User` is non-nil; `User?` can hold `nil`. Weak references do not keep the object
alive and must be safely promoted to strong optional values before use. The current
weak spelling is `weak User parent`; detailed qualifier composition is a gate.
See [Nullability](nullability.md) and [Weak References](../runtime/weak-references.md).

## Manual RC

The manual-RC variant retains the same reference-counting concept, but ownership
acquisition/release is explicit. Its declaration spelling remains unresolved;
`unmanaged`, `~arc`, and other alternative spellings remain unselected.

## Inheritance and metadata

Class inheritance is deferred to a future version. Protocol conformances and
ordinary methods do not insert one dispatch pointer per method into every object.
The baseline reference/header model is in [Reference Types](../abi/reference-types.md).
