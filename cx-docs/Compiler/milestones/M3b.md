# M3b: Argument labels

Status: **Completed** (implemented and verified in this checkout).

Baseline: the pinned checkout recorded in the [M0 record](M0.md).

Second slice of roadmap row M3, building on [M3a](M3a.md). Overload lookup and
compound references are [M3c](M3c.md); M3 is not complete until that lands.

## Source contract

```c
void resize(int width newWidth, int height newHeight) {
    applySize(newWidth, newHeight);   // the body sees the local name
}

resize(width: 800, height: 600);      // a direct call uses the label
```

- A labeled parameter is a **complete C parameter declarator followed by the
  local name**. The declarator's name is the external label; the extra
  identifier is the name visible inside the body. The extension attaches after
  the whole declarator, so `void (*callback)(int) handler` gives the label
  `callback` and the local name `handler`.
- An ordinary one-name parameter stays positional.
- When a parameter has a label, a direct call must write it. Omitting a label
  in the declaration does not create one at the call site, and writing one the
  declaration did not declare is an error.
- A redeclaration must keep the labels the previous declaration established.
  Local names may differ. [M3c](M3c.md) narrows this to unowned files, where
  one C symbol per name means the two declarations really are one entity; in an
  owned file they are two overloads.
- Labels are not available through a function pointer: a C function pointer
  carries no Cx argument-label interface.
- Labels join the mangled Cx identity from M3a. The label section is written
  only when at least one parameter has a label, with `_` for the unlabeled
  positions:

```c
#module UI
void labeled(int width w, int height h);  // _Z29_Cx0$UI$labeled$width:height:ii
void mixed(int v, int mode m);            // _Z21_Cx0$UI$mixed$_:mode:ii
void unlabeled(int v, int w);             // _Z17_Cx0$UI$unlabeledii
```

Labels are a source-level interface, so they work in an unowned file too; that
file's entities simply keep their C symbols, because linkage follows ownership
and not labelling.

## Deliberately not implemented

[G02](../../OPEN-ISSUES.md#g02--labels-and-synthesized-construction) leaves two
things open, and neither is implemented here:

- the convenience when the external and local names coincide: there is no
  `int x x` shorthand and no single-name form that creates a label;
- optionally omittable call labels: a declared label is always required.

## Permitted baseline C modes

Every C/GNU standard, as in M0. The syntax is not valid C in either position,
so no C program changes meaning: an identifier after a complete parameter
declarator is a syntax error in C, and `name :` at the start of a call argument
is not a C expression.

## Implementation

| Area | Change |
| --- | --- |
| `clang/include/clang/Basic/Attr.td` | `CxArgumentLabel`, an implicit-only attribute on a parameter |
| `clang/lib/Parse/ParseDecl.cpp` | `ParseParameterDeclarationClause` swaps the declarator name for the local name and records the label |
| `clang/lib/Parse/ParseExpr.cpp` | call arguments consume a leading `name :` |
| `clang/lib/Sema/SemaCx.cpp` | `AddCxArgumentLabel`, `CheckCxArgumentLabels`, `CheckCxArgumentLabelRedeclaration` |
| `clang/lib/Sema/SemaDecl.cpp` | the redeclaration check runs with `AddCxLinkage` |
| `clang/lib/AST/ItaniumMangle.cpp` | the label section of the Cx name |

The call-site labels are collected through the `ExpressionStarts` callback that
`ParseExpressionList` already invokes before each argument, rather than by
threading an output parameter through a function with many callers.

The redeclaration check is not a nicety. Labels are part of the mangled name,
so two declarations of one entity that disagreed about labels would quietly
produce two different symbols; rejecting the disagreement is what keeps the
mangling from turning a source-level slip into a link-time mystery.

## AST and Sema changes

One implicit attribute on `ParmVarDecl`. No new AST node: a labeled call is an
ordinary `CallExpr`, checked after it is built. Nothing about overload
resolution changes yet.

## Target and runtime requirements

None.

## Tests

`clang/test/Cx/`:

- `labels.c`: simple and complex labeled declarators, the local name in the
  body, the attribute in the AST, positional parameters left alone, labeled
  calls; plain C rejects the syntax.
- `labels-diags.c`: missing, wrong and extraneous labels, each with a note at
  the parameter; a label through a function pointer.
- `labels-redecl.c`: a label mismatch across declarations is rejected; an
  agreeing redeclaration with different local names is fine and produces one
  symbol.
- `labels-mangling.c`: labels in the symbol, `_` for unlabeled positions, a
  wholly unlabeled function's symbol unchanged from M3a, and an unowned file
  keeping its C symbols.

Verification run on this checkout: `clang/test` 48925 passed, 30 expectedly
failed, 0 unexpected failures.

## C collisions

- An identifier following a complete parameter declarator is invalid C, so the
  declaration form takes a position C cannot use.
- `name :` at the start of a call argument is likewise not a C expression. The
  conditional operator is not affected: `f(a ? b : c)` has `?` as its second
  token, not `:`.
- A call written with no labels to a function declared with none is not
  examined at all, so ordinary C calls are untouched.

## Known limitations

- **Default arguments are untouched.** Their evaluation and redeclaration rules
  are still unfixed, as `language/functions.md` requires before they are
  enabled across translation units.
- **Callable types carry no labels.** `int (text: String) parser` is M12; a
  labeled call is only checked against a direct callee.
- **Labels do not yet participate in overload resolution**, because there are
  no overload sets until M3c. Two functions differing only in labels are still
  a C redefinition error.
- **No compound references.** `&move(x:)` is M3c.
- **An unowned file's labels are unmangled**, so an entity there is still one C
  symbol. That is a consequence of linkage following ownership, not a separate
  rule, but it means labels alone cannot distinguish two entities.

## Runnable demonstration

```sh
clangx -S -emit-llvm -o - labeled.c | grep '^define'
# define void @"_Z29_Cx0$UI$labeled$width:height:ii"(i32 %newWidth, i32 %newHeight)
```

```
resize(800, 600);
        error: missing argument label 'width:' in call
        note: parameter declared here
```

## Benchmark checkpoint

Not applicable. Labels are a frontend interface; the call lowers unchanged.
