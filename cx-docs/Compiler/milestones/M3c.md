# M3c — Overload lookup and compound references

Status: **Completed** (implemented and verified in this checkout).

Baseline: the pinned checkout recorded in the [M0 record](M0.md).

Third and last slice of roadmap row M3, after [M3a](M3a.md) and [M3b](M3b.md).
With this, row M3 is complete.

## Source contract

```c
#module Draw

void show(int value);
void show(double value);        // overloads by parameter type

void move(int x value);
void move(float x value);
void move(int y value);         // and by argument label

void (*callback)(int) = &move(x:);   // compound name, not a call
void (*both)(int, int) = &plain(_:_:);
```

- A module-owned entity has a mangled name carrying its labels and parameter
  types, so **several entities can share a base name**. An unowned entity has
  one C symbol and therefore cannot be overloaded.
- Lookup uses name, labels, arity and types. Resolution ranks per argument with
  C conversions only: exact match, then C promotion, then C conversion. For a
  `short` argument, the promotion to `int` beats a conversion to `long`.
- If neither candidate is uniformly preferable the call is ambiguous. Nothing
  is selected by declaration order.
- A candidate whose labels are not the ones written at the call is not
  applicable, whatever its parameter types would allow, and the diagnostic says
  which label made it inapplicable.
- `move(x:)` is a **compound name**, not a call: it names the function without
  calling it. It filters the overload set to the declarations with exactly
  those labels; the target type then selects among what is left, and without
  sufficient context the reference is ambiguous. `_` names a position with no
  label. `move(x: 10)` is still a call and `nothing()` is still a call.

## Consequence for M3b's redeclaration rule

[M3b](M3b.md) rejects a redeclaration that disagrees with its previous
declaration about labels. That rule now applies exactly where it is needed and
not where it would be wrong:

- In an **unowned** file there is one C symbol per name, so two declarations
  with the same types are the same entity and must agree about labels. The
  error still fires.
- In an **owned** file they are two entities with two symbols, so they are
  legitimately overloads and no error is possible.

Detecting a *definition that was meant* to implement an existing declaration
but drifted in its labels is a near-miss diagnostic, which belongs to
[G04](../../OPEN-ISSUES.md#g04--source-model-and-visibility) and M4.

## Permitted baseline C modes

Every C/GNU standard, as in M0.

## Implementation

| Area | Change |
| --- | --- |
| `clang/lib/Sema/SemaCx.cpp` | owned functions also get `OverloadableAttr`; `HasDifferentCxArgumentLabels`; `CxCandidateAcceptsCallLabels`; `BuildCxCompoundNameRef` |
| `clang/lib/Sema/SemaDecl.cpp` | `AddCxLinkage` moved ahead of redeclaration merging and now reads the lookup result |
| `clang/lib/Sema/SemaOverload.cpp` | differing labels make two declarations overloads; a candidate with the wrong labels is not viable; the note explaining why |
| `clang/include/clang/Sema/Overload.h` | `ovl_fail_cx_argument_label` |
| `clang/include/clang/Sema/Sema.h` | `CxCallArgumentLabels` and its `CxCallLabelScope` |
| `clang/lib/Parse/ParseDecl.cpp`, `ParseExpr.cpp` | `isCxCompoundNameSuffix`; compound-name parsing; call labels published to overload resolution |

Overload sets, per-argument ranking with C conversions only, ambiguity
detection and address-of-overload selection are Clang's existing `overloadable`
machinery, which already means in C exactly what `language/overloads.md`
specifies. Only the label dimension is new.

Two things had to move to make that reuse work. `AddCxLinkage` now runs
**before** redeclaration merging, because Clang checks that every declaration
of one entity agrees about `overloadable` during the merge; it reads the lookup
result instead of a redeclaration chain, which keeps the "first declaration
decides identity" rule from M3a intact. And the call's labels are published to
overload resolution through a scope object for the duration of one
`ActOnCallExpr`, because a candidate's viability now depends on them.

## AST and Sema changes

No new AST node. A compound name produces the same `DeclRefExpr` or
`UnresolvedLookupExpr` that an ordinary function name produces, over a filtered
candidate set, so address-of-overload selection needed no change at all.

## Target and runtime requirements

None.

## Tests

`clang/test/Cx/`:

- `overloads.c` — overloads by type and by label, including two that differ
  only in their label, each with its own symbol; `short` promoting to `int`
  rather than converting to `long`.
- `overloads-diags.c` — a label making candidates inapplicable, with a note
  naming the expected label; an ambiguous call reported as ambiguous.
- `compound-references.c` — `&move(x:)` selected by two different target types,
  a single match, `_` for unlabeled positions, and calls still parsing as calls.
- `compound-references-diags.c` — an unknown label, a wrong arity, a compound
  name on a non-function, and an ambiguous reference without context.
- `labels-redecl.c`, `labels-redecl-unowned.c` — the label-agreement rule in
  the file kind where it applies.

Verification run on this checkout: `clang/test` 48930 passed, 30 expectedly
failed, 0 unexpected failures.

## C collisions

- A compound name occupies `f(name:)`, which is not a C expression, and is
  distinguished from a labeled call by a bounded lookahead: every position must
  be an identifier followed by `:`, and `f()` stays an ordinary call.
- Only module-owned functions become overloadable, so an unowned C entity is
  still a single symbol with C redefinition rules. A name that exists in an
  unowned header keeps its C identity even when redeclared inside a module,
  which `linkage-c-identity.c` pins.

## Known limitations

- **No generics**, so the concrete-versus-generic preference rules in
  `language/overloads.md` are not implemented. They arrive with M9.
- **Callable types carry no labels**, so label-erasure conversions between a
  labeled callable and an unlabeled interface do not exist yet. M12.
- **No `throw` effects**, so a throwing entry cannot yet be rejected where a
  non-throwing C function pointer is expected. M13.
- **The overload set is per translation unit.** Cross-module overloading needs
  declarations to be visible, which is still `#include`; artifacts are M15.
- ~~**Near-miss diagnostics are missing.**~~ Closed for members by
  [M4.2](M4.2.md): a continuation definition that nearly matches a declared,
  unimplemented member is reported rather than silently becoming a second
  one.
- **`enable_if`-style constraints do not apply.** Cx has no constraints yet.

## Runnable demonstration

```sh
clangx -S -emit-llvm -o - draw.c | grep -E '^@|declare'
# declare void @"_Z17_Cx0$Draw$move$x:i"(i32)
# declare void @"_Z17_Cx0$Draw$move$x:f"(float)
# declare void @"_Z17_Cx0$Draw$move$y:i"(i32)
# @callback = global ptr @"_Z17_Cx0$Draw$move$x:i"
```

```
move(1);
   error: no matching function for call to 'move'
   note: candidate function not viable: expects argument label 'x:' for argument 1
   note: candidate function not viable: expects argument label 'y:' for argument 1
```

## Benchmark checkpoint

Not applicable. Overload resolution is a frontend activity; the selected call
lowers as any direct call.
