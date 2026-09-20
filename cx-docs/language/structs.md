# Structs and Value Semantics

Cx structs are always values. Assignment, value parameters, and value returns have
copy semantics. Optimizations may eliminate copies but must preserve that behavior.

```c
struct Counter {
    int value

    void increment()
    ~mutating int current()
}
```

## Methods and `self`

Methods are associated functions. They add neither per-instance method pointers nor
an implicit vtable. The receiver is `self`, never `this`.

```c
struct Counter {
    void increment() {
        self.value += 1
    }

    ~mutating int current() {
        return value
    }
}
```

Unqualified fields are allowed when a local parameter/binding does not shadow them.
The example's second block implements the first; it does not add stored state.

## Receiver mutation

Instance methods are mutating by default. `~mutating` promises that the receiver's
stored value is not modified and allows calls on `let`/`const` values.

```c
let counter = Counter(value: 0)
counter.current()   // allowed
counter.increment() // error
```

This is shallow const-style protection, not purity or transitive deep immutability.
A pointer or class reference stored in a const value may still refer to mutable
external state under its own rules. A lint/fix-it can suggest `~mutating`; body
inspection must not silently change an externally declared method contract.

## Copy and cleanup

Trivial fields use ordinary C-compatible value operations. An ARC class field
copies its reference with the required ownership operation; it does not clone the
object. Copying a struct containing `User owner` produces another struct value that
can still refer to the same `User`.

Managed fields are destroyed in reverse declaration order. A custom `deinit()` is
a hook before automatic member destruction, not a replacement for that destruction.
Raw pointers are copied as addresses and are not implicitly freed.

Custom resource-owning structs with raw handles need an explicit copy policy before
being made freely copyable; automatic copying must not invent `dup`, deep copy, or
balanced manual-RC retains. See [G07](../OPEN-ISSUES.md#g07--ownership-and-failure-contracts).

## Layout and visibility

Stored fields, including `private` and `internal` fields, belong to the primary type
definition. Implementation/extension blocks do not add storage. Access modifiers
apply equally to structs and classes. A private field's layout is not secret.

Existing C structs, aggregate initialization, assignment, and uninitialized local
storage retain their C behavior; Cx construction syntax does not retroactively run
constructors for every legacy C declaration.
