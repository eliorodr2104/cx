# Stored and Computed Properties

Stored fields have implicit read/write access; writing manual forwarding getters and
setters is unnecessary.

```c
class User {
    String name
    private(set) int id
}
```

The access modifiers apply without requiring accessor functions or additional
instance fields. A direct stored access may lower to a load/store.

## Computed getter

```c
struct Rect {
    float width
    float height

    ~mutating float area {
        return self.width * self.height
    }
}
```

Without explicit `get`/`set`, the property body is the getter. It has no stored field.
`~mutating` makes this read operation available on const/let struct receivers.

## Explicit getter and setter

```c
String name {
    get {
        return storedName
    }

    set {
        storedName = normalize(newValue)
    }
}
```

`newValue` is the setter's implicit input name. Its type is the property's value type.
A custom name syntax has not yet been selected.

A getter-only body exposes no setter. Stored properties are different: they have
write access unless immutability or an access modifier restricts it. Access control
and receiver mutation are independent dimensions.

## Protocol requirements

```c
protocol Named {
    String name { get }
}
```

A requirement has no executable body from which to infer behavior. `{ get }` or
`{ get set }` therefore declares the required interface explicitly.

## Source and runtime model

Declare shared properties in the primary interface; implement accessor bodies in
continuations. Implementation syntax for split accessors must preserve the distinction
between a stored field and a computed member and is part of G08.

A computed access is semantically a function invocation. It can have observable side
effects even though no parentheses are written. Inline optimization is possible, not
guaranteed. Turning a stored public field into a computed property can change binary
ABI and addressability even if source reads/writes look unchanged.

Compound mutation and nested value mutation need a defined getter/modify/setter
writeback path. Address-taking of a computed property cannot pretend an independent
temporary is its persistent backing storage. Throwing accessors and custom in-place
modify accessors remain outside the frozen surface until specified.
