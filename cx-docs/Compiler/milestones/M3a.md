# M3a — Cx linkage and experimental mangling

Status: **Completed** (implemented and verified in this checkout).

Baseline: the pinned checkout recorded in the [M0 record](M0.md).

First slice of roadmap row M3, covering its stated acceptance: "minimal
deterministic experimental mangling and C-linkage checks". Labels and call-site
labels are [M3b](M3b.md); overload lookup and compound references are
[M3c](M3c.md). M3 is not complete until all three are.

## Source contract

Module ownership from [M2](M2.md) is what decides linkage.

- A function declared in a file owned by a Cx module has **Cx linkage**: its
  symbol is a mangled Cx name carrying the owning module and the parameter
  types, not its plain C name.
- A function in an unowned file keeps **C linkage**. Cx mode alone changes
  nothing; ownership does.
- **A redeclaration never changes an entity's identity.** A function first
  declared in an ordinary C header is still that C entity even when it is
  defined inside a module, and one first declared in a module-owned header
  keeps its module even when defined in an unowned file. The first declaration
  decides; the file holding the definition does not.
- `main` keeps its C name whatever owns its file.
- A `static` function is mangled too, with the ordinary internal-linkage
  marker. Mangling does not change its scope: it stays translation-unit local.

## Mangling

```
<cx-name> ::= <length> _Cx <version> $ <module> $ <base-name>
```

written as a single Itanium `<source-name>`, so the enclosing function mangling
supplies the parameter types and the symbol stays one well-formed token:

```c
#module Geometry
int  rect_area(int w, int h);   // _Z23_Cx0$Geometry$rect_areaii
long scale(long v);             // _Z19_Cx0$Geometry$scalel
static int helper(int v);       // _ZL20_Cx0$Geometry$helperi
```

The `_Cx0` version prefix is deliberate. The encoding is experimental and is
not a distribution promise; it exists so that distinct Cx entities have
distinct deterministic symbols, which is what M3b's labels and M3c's overloads
need in order to mean anything.

Identity inputs used today: module, base name, parameter types. Local parameter
names, paths, line numbers and build directories are not identity, as required.
Argument labels join this encoding in M3b.

## Permitted baseline C modes

Every C/GNU standard, as in M0.

## Implementation

| Area | Change |
| --- | --- |
| `clang/include/clang/Basic/Attr.td` | `CxLinkage`, an implicit-only inheritable attribute carrying the owning module |
| `clang/lib/Sema/SemaCx.cpp`, `clang/lib/Sema/SemaDecl.cpp` | `Sema::AddCxLinkage`, called from `CheckFunctionDeclaration` after redeclaration merging |
| `clang/lib/AST/ItaniumMangle.cpp` | `shouldMangleCXXName` accepts Cx entities; `CXXNameMangler::mangleCxName` writes the composed source name |

The marker is an attribute rather than a recomputation from the ownership map
for two reasons. It has to survive serialization, because M2 does not serialize
ownership and a declaration loaded from a PCH would otherwise silently lose its
identity. And it has to be *decided once*, at the first declaration, rather than
re-derived per declaration from whichever file that declaration sits in —
which is exactly the C-identity rule above.

`AddCxLinkage` runs after merging so that `getPreviousDecl()` is meaningful. An
entity with a previous declaration copies that declaration's answer and never
consults its own file; only a genuinely new declaration asks the ownership map.

## AST and Sema changes

One implicit attribute. No new AST node, no new type, no change to overload or
lookup behavior — that is M3c.

## Target and runtime requirements

None. The mangled name is an ordinary symbol; `$` is valid in ELF and Mach-O
symbol names.

## Tests

`clang/test/Cx/`:

- `linkage-mangling.c` — module, base name and parameter types in the symbol;
  a `static` function mangled with the internal marker; `main` unmangled; the
  same file untouched both as plain C and in Cx mode without `#module`.
- `linkage-c-identity.c` — an entity declared in an ordinary C header keeps its
  C symbol when defined inside a module, checked with `--implicit-check-not` so
  the Cx name cannot appear anywhere in the output; an entity declared in an
  owned header keeps its Cx symbol; a new function in the owned file gets one.
- `linkage-separate-tu.c` — producer and consumer compiled separately agree on
  the symbol, and an unowned implementation file inherits the header's identity.

Verification run on this checkout: `clang/test` 48921 passed, 30 expectedly
failed, 0 unexpected failures.

## C collisions

- A C program cannot observe the rename of an entity it declared, because an
  entity declared in a C header keeps its C symbol. That is the whole point of
  the redeclaration rule, and `linkage-c-identity.c` pins it.
- System and third-party headers are unowned, so everything they declare keeps
  C linkage. Verified through `printf`.
- `__asm__("name")` still wins: `getMangledName` honors `AsmLabelAttr` before
  asking the mangler.

## Known limitations

- **`extern(C)` does not exist yet.** There is no way to ask for a C-facing
  entry from an owned file other than declaring the entity in an unowned
  header first. The spelling belongs with the error and bridge work (M13).
- ~~**Variables are not covered.**~~ Closed by [M4.2](M4.2.md): an
  externally visible file-scope variable in an owned file carries its module
  in its symbol, decided by its first declaration.
- **The encoding is not demangler-friendly.** The symbol starts with `_Z`, so
  ordinary C++ demanglers will try and fail on it rather than print something
  useful. A Cx-aware tool is M19.
- ~~**Labels are not in the identity yet.**~~ Closed by [M3b](M3b.md): two
  declarations differing only in labels mangle differently.
- ~~**No overload sets.**~~ Closed by [M3c](M3c.md). Here two functions with
  the same base name in one owned file were still a C redefinition error;
  distinct symbols were the precondition, not overloading itself.
- **Cross-module coherence is unchecked.** Two modules with the same name in
  one program produce the same symbols; module identity versioning is G09.

## Runnable demonstration

```sh
clangx -S -emit-llvm -o - owned.c | grep '^define'
# define i32 @"_Z23_Cx0$Geometry$rect_areaii"(...)
# define i32 @imported_c_entity(...)            <- declared in a C header
# define i32 @main()
```

```sh
clangx -c geom.c -o geom.o && clangx -c user.c -o user.o
clangx geom.o user.o -o prog && ./prog      # 42
```

## Benchmark checkpoint

Not applicable. Mangling changes symbol names, not generated code.
