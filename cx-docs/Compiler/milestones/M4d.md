# M4d — Generated memberwise construction

Status: **Completed** (implemented and verified in this checkout).

Baseline: the pinned checkout recorded in the [M0 record](M0.md).

Last slice of roadmap row M4, after [M4a](M4a.md), [M4b](M4b.md) and
[M4c](M4c.md). With this, row M4 is complete.

## Source contract

```c
#module Shapes
typedef struct Size { int width; int height; } Size;

Size size = Size(width: 80, height: 40);
```

- With no user-declared initializer, a struct is constructed from **one
  labelled value per stored field, in declaration order**, under the field's
  own name. The labels are compiler-provided construction metadata; nobody
  writes `int width width`.
- The result is a value of the struct type, initialized exactly as the
  equivalent C initializer would be, so the usual C conversions and
  diagnostics apply per field.
- **Construction writes every field, so the generated surface is only
  available where each field it exposes may be written.** A struct with a
  `private` field therefore has no usable construction outside the type
  itself, which is what "a synthesized initializer must not expose
  inaccessible state as a public construction API" means in practice.
- A union has no generated construction.
- Missing values, values in the wrong order, values without labels and extra
  values are each diagnosed, with a note at the field concerned.

## Deliberately not implemented

- **Positional values.** Whether the generated surface also accepts them is
  [G02](../../OPEN-ISSUES.md#g02--labels-and-synthesized-construction), so the
  labels are required and `Size(1, 2)` is rejected rather than silently
  accepted in a way a later decision would have to take back.

## Permitted baseline C modes

Every C/GNU standard, as in M0. A type name followed by `(` is not a C
expression, so the form takes a position C cannot use; ordinary calls,
compound literals and C initializers are untouched.

## Implementation

| Area | Change |
| --- | --- |
| `clang/lib/Parse/ParseExpr.cpp` | a type name followed by `(` starts a construction, in both the identifier and the annotated-type paths |
| `clang/lib/Parse/ParseDecl.cpp` | `ParseCxConstructionExpression` |
| `clang/lib/Sema/SemaCx.cpp` | `getCxConstructionType`; `ActOnCxConstruction` |

The construction lowers to an ordinary initialization of the record, built
through `ActOnInitList` and `BuildCompoundLiteralExpr`, so no new AST node and
no new lowering path exist. The access check is M4c's write check, applied per
field, rather than a second rule that could disagree with it.

`getCxConstructionType` accepts any record, including a union, so that the
union case reaches `ActOnCxConstruction` and gets a diagnostic explaining the
rule instead of falling back to C's "unexpected type name".

## AST and Sema changes

None beyond the two entry points. The value is a compound literal of the
record type.

## Tests

`clang/test/Cx/`:

- `construction.c` — a two-field struct constructed and read back, the fields
  initialized in declaration order; an ordinary call still a call; plain C
  rejecting the form.
- `construction-diags.c` — a missing field, positional values, values in the
  wrong order, an extra value, and a union.
- `construction-access.c` — a struct with a private field not constructible at
  file scope even inside the owning module, constructible from the type's own
  implementation, a wholly public struct constructible anywhere, and reading
  still governed by read access.

Verification run on this checkout: `clang/test` 48942 passed, 30 expectedly
failed, 0 unexpected failures.

## C collisions

- A type name in expression position is invalid C, and the construction path
  is only entered when the name resolves to a record type, so `f(1)` where `f`
  is a function is untouched.
- Compound literals, aggregate initialization and uninitialized storage keep
  their C behavior. Cx construction does not retroactively run anything for a
  legacy C declaration.

## Known limitations

- ~~**No declaration-site field defaults.**~~ Supplied by [M4e](M4e.md): a
  defaulted field may be skipped, an empty call is available when nothing is
  required, and a defaulted field is not part of the construction surface.
- **No custom `init`**, so "declaring any custom initializer suppresses
  synthesis" cannot be exercised. When `init` arrives, the suppression rule
  has to be added with it.
- **Only a typedef'd or annotated type name.** The bare tag-name convenience
  (`Size` for `struct Size` without a typedef) is
  [G01](../../OPEN-ISSUES.md#g01--c-grammar-collisions).
- **Nested and array fields are constructed only from an expression**, not
  from a nested generated call, because there is no contextual type for an
  inner `(...)` yet.
- **No classes**, so construction never produces an owned reference. M11.

## Runnable demonstration

```c
#module Shapes
typedef struct Size { int width; int height; } Size;
int main(void) {
  Size s = Size(width: 80, height: 40);
  return (s.width == 80 && s.height == 40) ? 0 : 1;
}
```

```sh
clangx demo.c -o demo && ./demo; echo $status   # 0
```

## Benchmark checkpoint

Not applicable. The construction is an ordinary record initialization and is
constant-folded where C would fold it.
