# M7b: Definite initialization and delegation

Status: **Completed** (implemented and verified in this checkout).

Baseline: the pinned checkout recorded in the [M0 record](M0.md).

Second slice of roadmap row M7: the rules of
[`language/initializers.md`](../../language/initializers.md#definite-initialization)
for custom initializers, which [M4f](M4f.md) left as zero-filled fields.

```c
struct Rect {
  float width
  float height

  init(float width w, float height h) { width = w; height = h }

  init(float side) {
    if (side < 0) side = 0
    self.init(width: side, height: side)
  }

  init(int side) {
    width = side
    return            // error: field 'height' is not initialized when the
  }                   //        initializer returns
}

Rect r = { 1, 2 }     // error: 'Rect' has an initializer, so it is
                      //        constructed with 'Rect(...)', not with braces
```

## The rule

Every path through an initializer initializes every field of `self` before it
leaves; no field is used before it is initialized, and `self` as a whole only
once it is complete. A value of a type with an initializer comes from running
one.

## Semantics

- **Initializing a field**: a declaration default, or an assignment of the
  whole field. `origin.x = 0` and `&field` on an uninitialized field are uses,
  so they are errors until the field is initialized.
- **Paths.** A field is initialized where every path reaching the point
  initializes it: both branches of an `if`, every clause a `switch` reaches, and
  not after a loop that alone assigns it. A path that ends in a `_Noreturn` call
  needs nothing.
- **`self`**: a method call, `self`, `&self` or `*self` needs every field.
- **`const` fields** are initialized by their one assignment on each path; a
  second, or a compound assignment or increment, is an error.
- **Arrays and aggregates** take braced defaults, `int counts[4] = {}`, since C
  cannot assign them whole. Their elements are then ordinary writes.
- **Anonymous members**: an anonymous struct's members are fields of their own;
  an anonymous union is initialized by any one of its members, after which any
  member may be read.
- **`defer`**: a deferred block uses `self` only where what it uses is
  initialized when the `defer` is registered, and initializes nothing.
- **Delegation**: `self.init(...)` selects an initializer like a construction
  does. A delegating initializer calls it exactly once on every path and touches
  `self` only after it.
- **Construction goes through `init`**: braces, written or elided, cannot build
  a type with an initializer, at any depth. Copies, declarations without an
  initializer, static storage and members a braced list leaves out stay C.

## Representation

The analysis runs when an initializer's body is complete, over Clang's CFG of
that body, like `-Wuninitialized`: a first walk labels each reference to `self`
(initialization, use, use of `self`, delegation), a forward fixed point computes
the fields initialized on every path and on some path plus the delegation state,
and a final walk reports each error once. Fields form a tree so anonymous
structs and unions are checked as described. Nothing is lowered differently: a
construction still zero-fills fields without defaults, which no initializer can
now expose, and a delegation is an ordinary call with `self` as the receiver.

`-ast-print` now writes a construction as `Type(label: value)` and a delegation
as `self->init(...)`, so its output compiles to the same code.

## Permitted baseline C modes

Every C/GNU standard, as in M0.

## Implementation

| Area | Change |
| --- | --- |
| `clang/lib/Sema/SemaCxInit.cpp` | the analysis |
| `clang/lib/Sema/SemaDecl.cpp` | runs it when an initializer's body is complete |
| `clang/lib/Sema/SemaCx.cpp` | `resolveCxInit`, shared by construction and delegation; `ActOnCxInitDelegation`; const fields of `self` |
| `clang/lib/Sema/SemaInit.cpp` | no braces for a type with an initializer |
| `clang/lib/Sema/SemaExpr.cpp` | a const field of `self` is assignable in an initializer |
| `clang/lib/Parse/ParseExpr.cpp`, `ParseDecl.cpp` | `self.init(...)`; braced field defaults |
| `clang/lib/AST/StmtPrinter.cpp` | constructions and delegations |

## Evidence

- The full Clang lit suite passes, with the known deadlocking
  `Index/crash-recovery-modules.m` filtered out.
- The IR of `init-definite.c` and `init-delegation.c` is identical after
  `-ast-print`.
- `code-completion.c` had an initializer that left a private field unset; it
  now sets it.

## Tests

- **`init-definite.c`**, as GNU17 and GNU89, then as IR, after `-E` and after
  `-ast-print`: braced defaults, branches, an early return, a Cx `switch`, a
  `_Noreturn` path, a loop, a const field on two branches, `defer` after an
  initialization and reassigning in a `defer`, anonymous struct and union.
- **`init-definite-errors.c`**: every diagnostic, including a partial write,
  `&field`, `*self`, const fields, `defer`, an array without a default, an
  anonymous union, braces written, nested, elided and in a compound literal,
  and static storage left zero.
- **`init-delegation.c`** and **`init-delegation-errors.c`**: delegation after
  computing an argument, on both branches, and its errors.

## Known limitations

- **Helpers cannot initialize fields.** The analysis sees one body; a field is
  initialized by assigning what a helper returns.
- **A `defer` in an initializer** uses only what is initialized where it is
  registered, which is stricter than where it runs.
- **Block literals and nested functions** inside an initializer are not
  analysed.
- ~~**A default reading another field** is still open (M7.1).~~ Supplied by
  [M7.1](M7.1.md).

## Runnable demonstration

```c
#module Shapes
int printf(const char*, ...)
typedef struct Rect {
  float width
  float height
  init(float width w, float height h) { width = w; height = h }
  init(float side) {
    if (side < 0) side = 0
    self.init(width: side, height: side)
  }
  ~mutating float area(void) { return width * height }
} Rect
int main(void) {
  Rect a = Rect(width: 3, height: 4)
  Rect b = Rect(-2.0f)
  Rect c = Rect(5.0f)
  printf("%g %g %g\n", a.area(), b.area(), c.area())
}
```

```sh
clangx demo.c -o demo && ./demo     # 12 0 25
```
