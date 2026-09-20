# Dictionary and Set

Dictionary maps keys to values; Set stores unique elements. Both are intended owning
value-semantic library containers, with COW where it preserves their contracts.
Hash-based implementations require equality/hash protocols with consistent laws.

```c
Dictionary<String, int> counts = ["one": 1, "two": 2]
Set<int> numbers = [1, 2, 3]
```

Concrete API names and literal edge cases remain provisional.

## Dictionary behavior

Lookup must distinguish an absent key from a present value that itself can represent
absence. An Optional<Value> lookup provides this distinction; collapsing an optional
stored value into "remove the key" needs an explicit separate setter contract.

Define duplicate literal keys, insert-vs-update results, removal results, ordering,
iteration invalidation, and failure effects before API release. Mutating keys in ways
that change their hash/equality while stored must not silently corrupt the table.
Class-reference keys need identity or stable semantic equality rules.

## Set algebra

Both Set and OptionSet should provide consistent names for membership, insertion,
removal, union, intersection, difference, symmetric difference, subset/superset, and
disjointness.

```c
var common = a & b
var combined = a | b
var onlyA = a - b
var different = a ^ b
```

Mutable variants and compound operators require normal receiver/writeback rules.
Set complement is not automatically meaningful without a defined finite universe;
OptionSet complement is a separate valid-bit policy.

## Existing C dictionary as a case study

The user has described an existing fast C dictionary using tagging, builtins, and
complex structures. Its source, build, and license have not been inspected for this
pack, and no speed claim has been verified.

The first acceptance step is unchanged C compiled under both baseline Clang C and
Cx mode, with identical defined behavior. Next, add opt-in Cx wrappers/methods and
protocols while keeping the C core buildable as pure C. Only then compare a deeper
rewrite or integration into libcx.

Pointer-tagging assumptions, target macros, allocation contracts, hash equality,
reallocation, and sanitizer behavior need correctness tests before benchmark B08.
Confirm distribution rights before vendoring code into the library.
