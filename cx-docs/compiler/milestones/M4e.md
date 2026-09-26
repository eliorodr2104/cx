# M4e: Declaration-site field defaults

Status: **Completed** (implemented and verified in this checkout).

Baseline: the pinned checkout recorded in the [M0 record](M0.md).

Fifth slice of roadmap row M4. It supplies the half of `language/initializers.md`
that [M4d](M4d.md) recorded as missing: *"Fields without declaration defaults
require construction values. Defaulted fields use their declared defaults; an
empty call is available when nothing is required."*

## Why it mattered

Without defaults, generated construction required **every** field, so a struct
with a `private` field had no usable construction anywhere outside its own
implementation, and no custom `init` existed to take its place. The type was
a dead end, not a design.

Defaults resolve that cleanly: a field with one is **not part of the
construction surface**, so its access does not bound where the type can be
built.

## Source contract

```c
#module Shapes

typedef struct Guarded {
    int shown;
    private int hidden = 7;
} Guarded;

Guarded r = Guarded(shown: 10);      // hidden takes its default
```

- A stored field may carry `= expression`. The default is type-checked where
  it is written, not at every construction that relies on it.
- A field with a default may be **skipped** in a construction, or given a
  value. A field without one must be given.
- **An empty call is available when nothing is required**, so a struct whose
  fields all have defaults is constructed with `T()`.
- A defaulted field is not exposed by the generated surface, so a `private`
  field with a default does not make the type unconstructible. Naming it
  anyway is still writing it, and still needs write access.
- Values are still matched in declaration order under the field's own name.

## Both spellings agree

```c
typedef struct P { int a; int b = 5; } P;

P x = { 1 };        // b == 5
P y = P(a: 1);      // b == 5
```

C initialization of a record with Cx field defaults uses them too. This is a
deliberate deviation from `language/structs.md`'s "aggregate initialization
retains C behavior": that sentence is about **existing C structs**, and a
struct with field defaults is not one. Leaving `{ 1 }` to zero-fill while
`P(a: 1)` gave 5 would mean two spellings of one initialization disagreeing,
which is worse than the deviation.

## Permitted baseline C modes

Every C/GNU standard, as in M0. `= ...` on a struct member is a syntax error
in C, so the form takes a position C cannot use, and a C struct without
defaults is untouched.

## Implementation

| Area | Change |
| --- | --- |
| `clang/lib/Parse/ParseDecl.cpp` | `= expression` after a member declarator; the field is created able to hold a default |
| `clang/lib/Sema/SemaDecl.cpp` | `ActOnField` takes `HasDefault`; the union variant-member rule applies only to C++ records |
| `clang/lib/Sema/SemaCx.cpp` | `AddCxFieldDefault`; construction skips defaulted fields and checks access only on the fields the call writes |
| `clang/lib/Sema/SemaInit.cpp` | a C record's field default is used directly, not through the C++ default-member-initializer path |

The default is stored in the field's existing in-class-initializer slot rather
than in a side table, so it serializes, dumps and imports like any other part
of the declaration.

Three C++-only assumptions had to be relaxed, all of the form "only a C++
class can have a default member initializer": the union variant-member check
cast the record to `CXXRecordDecl`; the aggregate-initialization path built a
`CXXDefaultInitExpr`, which sets up a `this` scope that does not exist here;
and [M4.1](M4.1.md)'s access check on record initialization examined every
field rather than the ones a call actually writes.

## AST and Sema changes

No new node and no new storage. A defaulted field is an ordinary `FieldDecl`
whose in-class initializer is set.

## Tests

`clang/test/Cx/field-defaults.c`: a private field with a default leaving the
type constructible; skipping some defaulted fields and giving others; an empty
call; C brace initialization agreeing with the generated call; a field without
a default still required; the private field still unwritable when named. Plain
C rejects the syntax.

Verification run on this checkout: `clang/test` 48945 passed, 30 expectedly
failed, 0 unexpected failures.

## Known limitations

- ~~**No custom `init`.**~~ Supplied by [M4f](M4f.md): `init(...)` in the
  type, `Type(...)` selecting it, and the rule that declaring one suppresses
  the generated surface.
- ~~**Only an expression as a default.**~~ [M7b](M7b.md) adds braced defaults,
  `int counts[4] = {}`.
- ~~**A default cannot mention another field.**~~ Supplied by
  [M7.1](M7.1.md): it may read an earlier field that holds a value.
- **Defaults are not used for an uninitialized declaration.** `Guarded g;`
  still leaves C's indeterminate storage, as `structs.md` requires.
- **No ordering guarantee beyond declaration order**, which is all the
  generated surface has; the custom-initializer statement-order rule in
  `initializers.md` belongs with `init`.

## Runnable demonstration

```sh
clangx demo.c -o demo && ./demo     # 10 1 2
```

## Benchmark checkpoint

Not applicable.
