# M4a — Struct methods, `self` and `~mutating`

Status: **Completed** (implemented and verified in this checkout).

Baseline: the pinned checkout recorded in the [M0 record](M0.md).

First slice of roadmap row M4. Continuations and access control are
[M4b](M4b.md); generated memberwise construction is [M4c](M4c.md). M4 is not
complete until all three land.

## Source contract

```c
#module Counters

struct Counter {
    int value;

    void increment() {
        self.value += 1;      // `.` on the implicit receiver
    }

    ~mutating int current() {
        return value;         // an unqualified field of the receiver
    }
};

struct Counter c = { 0 };
c.increment();
c.current();
```

- A function declarator inside a struct declares a **method**: an associated
  function whose first parameter is the implicit `self` receiver. It adds no
  stored field, no per-instance method pointer and no vtable. A `_Static_assert`
  in the tests pins that the layout is unchanged.
- `self` is the receiver; there is no `this`. `self.field` is permitted, and it
  is the implicit receiver only — this does **not** establish a general `.`
  shorthand for raw pointers, as `language/pointers.md` requires.
- An unqualified name inside a method body resolves against the receiver when a
  local parameter or binding does not shadow it.
- Instance methods are mutating by default. `~mutating` gives `self` a const
  pointee: the method may be called on a `const` value, and the compiler
  enforces the promise by rejecting writes through `self`.
- `c.method(args)` is a call of the associated function with the receiver's
  address. Nothing about the receiver is copied.
- A method is bound to its receiver, so a bare method reference is an error; it
  never decays to a plain function pointer.
- A method always carries a Cx name, because it cannot be a C entity. The
  record is the enclosing qualified name, so a method in an unowned file is
  still distinct from a same-named method of another type.

## Permitted baseline C modes

Every C/GNU standard, as in M0. A function member is a hard error in C
(`field 'increment' declared as a function`), so the syntax slot is free.

## Implementation

| Area | Change |
| --- | --- |
| `clang/include/clang/Basic/Attr.td` | `CxMethod`; `CxLinkage`'s module becomes optional |
| `clang/lib/Parse/ParseDecl.cpp` | `~mutating`; a function declarator in a struct becomes a method; method bodies cached and replayed; `ParseCxMethodBody` |
| `clang/lib/Sema/SemaCx.cpp` | `ActOnCxMethodDeclarator`, `LookupCxMethod`, `isCxSelfReference`, `getCurrentCxMethod`, `BuildCxMethodCall`, `BuildCxImplicitSelfMemberRef` |
| `clang/lib/Sema/SemaExprMember.cpp` | member access finds a method; `.` permitted on the receiver |
| `clang/lib/Sema/SemaExpr.cpp` | a call whose callee is a method; an unqualified name resolved against the receiver; a bare method reference rejected |
| `clang/lib/AST/Decl.cpp` | a method's linkage is a file-scope function's, and `isDeclExternC` accepts a Cx record member |
| `clang/lib/AST/ItaniumMangle.cpp` | the module section may be empty |

A method body is **cached and replayed** after the record is complete, the same
way C++ defers an inline member function body. Without that, a method could not
use a field declared after it, and the doc's own example would not compile.

Two upstream assumptions had to be relaxed, both of the form "a declaration
inside a record can only be C++". `isDeclExternC` asserted it; a Cx method is
not extern "C" either, so the assertion now admits Cx. And linkage for a record
member is computed the way a C++ class member's is, which produced internal
linkage in C; a Cx method is an associated function, so it takes the linkage a
file-scope function would have. Without that, a method declared in a header
could never link to a definition elsewhere, which M4b needs.

## AST and Sema changes

No new AST node. A method is a `FunctionDecl` in its `RecordDecl`; a method call
is an ordinary `CallExpr` whose first argument is the receiver's address, which
is what "methods are associated functions" means. The intermediate member
reference is a `MemberExpr` that only a call consumes.

## Target and runtime requirements

None.

## Tests

`clang/test/Cx/`:

- `struct-methods.c` — declaration and definition, `self.field`, an unqualified
  field, a method using a field declared after it, the call passing the
  receiver's address, the symbols, a `_Static_assert` that the layout is
  unchanged, and plain C rejecting the member.
- `struct-methods-mutating.c` — `~mutating` called on a constant value, a
  mutating method rejected there with a note suggesting `~mutating`, and a
  `~mutating` body rejected for writing through `self`.
- `struct-methods-diags.c` — a bare method reference and an assignment of one
  to a function pointer; `.` still rejected on an ordinary pointer.

Verification run on this checkout: `clang/test` 48933 passed, 30 expectedly
failed, 0 unexpected failures.

## C collisions

- A function member is invalid C, and so is `~mutating` before a member
  declaration, so both take positions C cannot use.
- `.` on a pointer keeps its C diagnostic everywhere except the implicit
  receiver, which `struct-methods-diags.c` pins.
- An existing C struct, aggregate initialization and uninitialized storage are
  untouched: a struct with methods is still initialized with `{ 0 }` here.

## Known limitations

- **No continuations yet.** A method must be defined where it is declared;
  reopening a type in another file is M4b, and until then a declaration without
  a body has no way to acquire one.
- **No access control.** Every method is public; `private`, `internal` and
  `private(set)` are M4b. The access specifier is set to public so that linkage
  can be computed at all.
- **No generated construction.** `Counter(value: 0)` does not exist; M4c.
- **No `deinit`, no copy or destruction hooks.** Value operations are M7.
- **Methods on classes do not exist**, because classes do not exist yet (M11).
- A method written `(void)` crashed the compiler as shipped here, and an
  unqualified name resolved against the receiver only for a field and not for
  a sibling method; both fixed in [M4.3](M4.3.md).
- **A method cannot be overloaded on the receiver's mutability**: `~mutating`
  and mutating versions of one name are two declarations of the same method,
  not an overload pair.
- Methods with argument labels, and overloaded methods, did not work as
  shipped here; [M4.1](M4.1.md) fixes both and changes the symbols of
  labelled methods.
- ~~**The diagnostic for writing through a `~mutating` receiver** is C's
  generic const message naming `self`.~~ Replaced in [M4.2](M4.2.md) by a
  message about the promise, with a note offering to drop `~mutating`.

## Runnable demonstration

```c
#module Counters
struct Counter {
  int value;
  void increment() { self.value += 1; }
  ~mutating int current() { return value; }
};
int main(void) {
  struct Counter c = { 40 };
  c.increment();
  c.increment();
  return c.current() == 42 ? 0 : 1;
}
```

```sh
clangx demo.c -o demo && ./demo; echo $status   # 0
```

## Benchmark checkpoint

Not applicable here, but the invariant a later checkpoint would measure is
already enforced by a `_Static_assert`: methods add no instance storage.
