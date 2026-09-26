# M7c: `deinit`, resource types and raw memory

Status: **Completed** (implemented and verified in this checkout).

Baseline: the pinned checkout recorded in the [M0 record](M0.md).

Third slice of roadmap row M7: the `deinit` and resource types of
[`language/initializers.md`](../../language/initializers.md#deinit).

```c
struct File {
  FILE* handle
  init(char* path) { handle = fopen(path, "r") }
  deinit() { fclose(handle) }
}

File a = File("a.txt")
File b = a                  // error: 'File' has a deinit, so it cannot be copied
a = File("b.txt")           // opens b.txt, closes a.txt
File *p = malloc(sizeof(File))
p->init("c.txt")
p->deinit()
free(p)
}                           // closes b.txt
```

## The rule

A type with a `deinit`, of its own or through a field, is a resource type. A
resource value has one owner: an existing one is never copied, and a new one,
a construction or a call's result, moves to where it is stored. Every value
that is stored is destroyed exactly once.

## Semantics

- **`deinit()`** takes no parameters and declares no return type, is not
  `~mutating`, is declared in the primary definition, and is not a union's.
- **Destruction** runs the body, then destroys each field that has a `deinit`,
  last declared first. A struct with such a field and no `deinit` of its own
  receives an implicit one.
- **Copies** are errors: initialization or assignment from an existing value,
  `return a`, `*p` read as a value, the value of an assignment, a field of a
  generated construction given an existing value.
- **Locals** need an initializer and are destroyed when their scope is left, on
  the cleanup stack `defer` uses; a jump past one is an error.
- **Assignment** builds the new value, destroys the old one, then stores the
  new one. In an initializer, a resource field of `self` is initialized once on
  each path, like a `const` field, and nothing is destroyed.
- **A discarded value** is destroyed at once; reading a field of a new value is
  an error.
- **Arrays** are given every element and destroyed from the last to the first.
  A braced list may not leave a resource member out.
- **Raw memory**: `p->init(...)` constructs a new value in `*p` without destroying
  what was there; `p->deinit()` destroys `*p`. Nothing checks their pairing.
- **Nowhere else**: static storage, parameters, variadic arguments, union
  members, tuple elements and enum payloads.

## Representation

The AST keeps the source. Sema diagnoses; CodeGen lowers with `pushDestroy`,
the same cleanup stack as `defer` and C++ destructors: a resource local pushes
its destruction, a `deinit` pushes its fields' before its body, an assignment
emits the new value into a temporary before destroying the old one, and an
ignored new value is emitted into a temporary and destroyed. `p->init(...)` is
built like a construction, `({ T *__cx_place = p; *__cx_place = T(...); })`,
and printed back as written. An implicit `deinit` has the linkage of a method
defined in its type's body.

## Permitted baseline C modes

Every C/GNU standard, as in M0.

## Implementation

| Area | Change |
| --- | --- |
| `clang/lib/Sema/SemaCx.cpp` | `deinit` declarations, resource types, implicit `deinit`, storage checks, `p->init` |
| `clang/lib/Sema/SemaExpr.cpp`, `SemaExprMember.cpp`, `SemaInit.cpp`, `SemaDecl.cpp` | copies, variadic arguments, fields of new values, left-out members, variables and parameters |
| `clang/lib/Sema/JumpDiagnostics.cpp` | jumps past a resource local |
| `clang/lib/Sema/SemaCxInit.cpp` | resource fields are initialized once |
| `clang/lib/CodeGen/CGCx.cpp` and hooks in `CGDecl.cpp`, `CGExpr.cpp`, `CGExprAgg.cpp`, `CodeGenFunction.cpp` | destruction, assignment, discarded values |
| `clang/lib/AST/ASTContext.cpp` | an implicit method is discardable ODR |
| `clang/lib/AST/StmtPrinter.cpp` | `p->init(...)` |
| `clang/lib/Parse/ParseDecl.cpp`, `ParseExpr.cpp` | `deinit(`, `p->init(` |

## Evidence

- The full Clang lit suite passes, with the known deadlocking
  `Index/crash-recovery-modules.m` filtered out.
- The IR of `deinit.c` is identical after `-E` and after `-ast-print`.
- A program that exercises every path prints its opens and closes in the
  expected order (see below).

## Tests

- **`deinit.c`**, as GNU17 and GNU89, then as IR, after `-E` and after
  `-ast-print`: body then fields, an implicit `deinit`, locals and `defer`,
  replacement, discarded values, arrays, an early return, raw memory.
- **`deinit-errors.c`**: every diagnostic.
- **`deinit-pch.c`**: a resource type and an implicit `deinit` from a PCH.

## Known limitations

- ~~**No moves** of a local: `return a` and passing `a` are copies (M7.2).~~
  Supplied by [M7.2](M7.2.md).
- **Raw memory is unchecked**, as `malloc` and `free` are.
- **C callers** will see a resource type as opaque once C export exists (M15).

## Runnable demonstration

```sh
clangx demo.c -o demo && ./demo
# open 1  open 2  close 1  open 3  close 3  open 4  close 4
# open 10 open 11 pair 10 close 11 close 10 ...
```
