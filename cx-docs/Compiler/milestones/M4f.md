# M4f: Custom initializers

Status: **Completed** (implemented and verified in this checkout).

Baseline: the pinned checkout recorded in the [M0 record](M0.md).

Sixth slice of roadmap row M4, and the part of `language/initializers.md` that
[M4e](M4e.md) recorded as the last one missing: `init(...)` in the type,
`Type(...)` selecting it, and the rule that declaring one replaces the
generated surface.

## Source contract

```c
#module Shapes

typedef struct Rect {
    float width;
    float height;

    init(float width newWidth, float height newHeight) {
        self.width  = newWidth;
        self.height = newHeight;
    }

    init(float side) { width = side; height = side; }
} Rect;

Rect r = Rect(width: 3, height: 4);
Rect s = Rect(5);
```

- An initializer is written `init(...)`, with **no return type**. Writing one
  is an error: an initializer produces its own type.
- It takes argument labels, overloads, access specifiers and a body exactly
  like a method because it *is* one. See below.
- `self` is available throughout, as is the unqualified spelling of a field.
- It cannot be `~mutating`: it establishes the value it builds.
- **Declaring any initializer suppresses the generated memberwise surface.**
  The initializers are the complete construction interface, as
  `initializers.md` requires.
- **Field defaults continue to participate**: the object starts out holding
  them, and the initializer writes only what it chooses to.
- Construction is bounded by the initializer it selects, not by the fields
  that initializer writes. A `private` field stays hidden while an `internal`
  or public `init` keeps the type constructible.
- An initializer may be declared with the type and defined in a continuation.

## Why an initializer is a method

An initializer is a `FunctionDecl` named `init` in the record, carrying
`CxMethodAttr`, with the same implicit leading `self` receiver a mutating
method has. Nothing about labels, overload resolution, access, linkage,
mangling, continuations or body replay needed a second implementation: they
all key on the method representation that [M4a](M4a.md)–[M4d](M4d.md) already
built. Only two places distinguish an initializer by name: `LookupCxMethod`, so
`r.init(...)` is not a member call, and construction, so `Rect(...)` finds it.

The parser reaches the same conclusion by splicing a `void` return type in
front of `init(` and letting the ordinary member path run. `init (x);` where
`init` is a type name is left alone, so the one C spelling this position has
is preserved.

## How construction lowers

`Rect(width: 3, height: 4)` becomes a statement expression:

```c
({ Rect __cx_object = { /* field defaults */ };
   init(&__cx_object, 3, 4);
   __cx_object; })
```

C already has an expression form for "run these statements, then produce this
value", so this needs no new AST node and no new lowering. The object is an
ordinary local; the initializer is an ordinary call with the receiver as its
first argument, which is what lets overload resolution, the label filter and
the access check all be the existing ones.

The consequence is that construction through an initializer **runs code**, so
it cannot appear outside a function. That is diagnosed directly rather than
left to C's "initializer element is not a compile-time constant".

## Permitted baseline C modes

Every C/GNU standard, as in M0. `init(...)` with no return type is not a valid
C struct member, and a struct with no initializer is untouched: it keeps the
generated memberwise surface and C brace initialization.

## Implementation

| Area | Change |
| --- | --- |
| `clang/lib/Parse/ParseDecl.cpp` | `TryCxInitializerIntroducer` splices `void` before `init(`, in both the primary body and a continuation |
| `clang/lib/Parse/ParseExpr.cpp` | a construction expression continues into `ParsePostfixExpressionSuffix` |
| `clang/lib/Sema/SemaCx.cpp` | `isCxInitializer`; return-type and `~mutating` checks; `LookupCxMethod` skips `init`; `BuildCxInitConstruction`; `ActOnCxConstruction` dispatches to it |
| `clang/lib/Sema/SemaOverload.cpp` | the arity note counts written arguments, not the Cx receiver |

Two of these are fixes for M4 oversights rather than new work.
`clang/lib/Parse/ParseExpr.cpp`: `Size(width: 3, height: 4).height` was
rejected because a construction returned without taking its postfix suffix.
`clang/lib/Sema/SemaOverload.cpp`: the arity note counted the implicit
receiver, so a missing argument read as *"requires 2 arguments, but 1 was
provided"* when the source had written none.

## AST and Sema changes

No new node, no new attribute and no new storage. An initializer is a
`FunctionDecl` with `CxMethodAttr`; a construction through one is a
`StmtExpr`.

## Tests

`clang/test/Cx/init.c`: two initializers distinguished by their labels; `self`
and the unqualified field spelling; a field default surviving an initializer
that does not write it; an initializer declared with the type and defined in a
continuation; an `internal init` keeping a type with a `private` field
constructible; the mangled symbol of each.

`clang/test/Cx/init-diags.c`: the generated surface gone once an initializer
exists; a wrong label leaving no candidate; `r.init(...)` not being a member
call; a written return type; `~mutating init`; construction at file scope;
`init` still an ordinary identifier as a typedef, a member name and a
parameter type.

`clang/test/Cx/construction.c` gained the postfix-suffix case.

Verification run on this checkout: `clang/test` 48947 passed, 30 expectedly
failed, 0 unexpected failures.

## C collisions

`init (x);` inside a struct declares a field when `init` is a visible type
name, and that reading is kept. Every other `init(` at the start of a struct
member is implicit-int, which C99 removed and which a struct cannot hold
anyway, so the position was free.

## Known limitations

- **No definite-initialization analysis.** A field the initializer does not
  write reads as zero rather than being diagnosed. `initializers.md` asks for
  "every successful initializer must establish every required stored field"
  and explicitly defers the design; zero is the interim, and it is chosen over
  indeterminate storage rather than over a diagnostic.
- **No delegation.** `init(...)` inside an initializer body does not call
  another one; `initializers.md` defers that with the same design.
- **No `throw`ing initializers.** `init(...) throw(E)` belongs with M13.
- **No method call on a temporary.** `Rect(2).area()` is rejected, because the
  receiver is passed by address and a constructed value is a prvalue. This is
  not specific to initializers: `make().area()` on any function returning a
  struct is rejected the same way, and it belongs with the writeback and
  materialization work in G08/M14.
- **C brace initialization is unaffected.** `Rect r = { 3, 4 };` still works on
  a type with an initializer. Suppressing it would change C behavior for a C
  form, which `structs.md` does not ask for.

## Runnable demonstration

```sh
clangx demo.c -o demo && ./demo     # 12 25 42 9
```

## Benchmark checkpoint

Not applicable.
