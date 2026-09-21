# M4c — Access control

Status: **Completed** (implemented and verified in this checkout).

Baseline: the pinned checkout recorded in the [M0 record](M0.md).

Third slice of roadmap row M4, after [M4a](M4a.md) and [M4b](M4b.md).
Generated memberwise construction is [M4d](M4d.md); M4 is not complete until
it lands.

## Source contract

```c
#module Users

struct User {
    int name;
    private int token;
    private(set) int id;
    internal int generation;
    internal private(set) int cacheVersion;
};
```

| Access | Who may use the member |
| --- | --- |
| `public` | anyone with a visible declaration |
| `internal` | code in the owning Cx module |
| `private` | the type's own implementation |

- A bare specifier sets read access, and write access follows it. A `(set)`
  specifier sets write access alone, so `private(set)` keeps the declared read
  access and restricts writing.
- **Write access cannot be broader than read access**; the combination is
  rejected.
- Primary members are **public** by default. A member introduced only in a
  continuation is an implementation detail, so it is **private** by default.
  An implementation keeps the access of the declaration it implements.
- "The type's own implementation" is what is lexically inside the type: a
  method body or a continuation.
- Access uses the **declaration's ownership**, not the module of whoever
  included the header.
- The specifiers are contextual. A member, or anything else, may still be
  named `private` or `internal`.

## Mutation means more than `=`

The write check sits on `CheckForModifiableLvalue`, the single point every
*expression* modification reaches, so `private(set)` cannot be sidestepped.
Initialization reaches fields another way and was missed here;
[M4.1](M4.1.md) closes it.

```c
u->id = 5;   u->id += 1;   u->id++;   --u->id;    // all rejected
int *p = &u->id;                                   // rejected too
int v = u->id;                                     // reading is public
const int *c = &u->id;   // allowed when the object is const
```

Taking a **mutable** pointer to a write-restricted member is rejected, because
restricting `=` while handing out an unrestricted pointer to the same field
would not enforce the intended API. The rule is deliberately conservative: it
rejects `&u->id` even where the result is immediately made `const`, since the
expression itself produces a writable pointer. The precise address-exposure
rule is [G08](../../OPEN-ISSUES.md#g08--writeback-and-mutable-views).

## Permitted baseline C modes

Every C/GNU standard, as in M0. An identifier before a member's type is not
valid C, so the specifiers take a position C cannot use, and a member actually
named `private` keeps working because the specifier is only recognised when a
type or `(set)` follows.

## Implementation

| Area | Change |
| --- | --- |
| `clang/include/clang/Basic/Attr.td` | `CxAccess`, carrying the read and write levels |
| `clang/lib/Parse/ParseDecl.cpp` | `ParseCxAccessSpecifiers`, used by the primary body and by a continuation |
| `clang/lib/Sema/SemaCx.cpp` | `AddCxAccess` with the defaults and the breadth rule; `CheckCxMemberAccess` |
| `clang/lib/Sema/SemaExprMember.cpp` | read access on a field and on a method |
| `clang/lib/Sema/SemaExpr.cpp` | write access on every modification and on mutable address-taking |

Three hooks cover every use because each is a funnel Clang already has:
`BuildFieldReferenceExpr` for reading a field, `CheckForModifiableLvalue` for
every modification, and `CheckAddressOfOperand` for handing out a pointer.

## AST and Sema changes

One implicit attribute per member. No new node, and no change to layout,
linkage or mangling: access controls source use, not identity, exactly as
`abi/linkage-and-mangling.md` requires.

## Tests

`clang/test/Cx/`:

- `access.c` with `Inputs/cx-user.h` and `Inputs/cx-user-impl.c` — the type's
  own implementation reaching every member; the owning module reaching public
  and internal but not private; another module reaching only public.
- `access-write.c` — `=`, `+=`, `++`, `--` and mutable address-taking all
  rejected for a `private(set)` member, while reading and a const address are
  allowed.
- `access-diags.c` — write broader than read, a repeated specifier, and
  members named `private` and `internal`.

Verification run on this checkout: `clang/test` 48939 passed, 30 expectedly
failed, 0 unexpected failures.

## C collisions

- The specifiers are recognised only when followed by a type or by `(set)`, so
  `int private;` and `n.private` keep their C meaning, which
  `access-diags.c` pins.
- No C program can contain a member with restricted access, because the
  specifiers do not exist in C, so no C code changes behavior.

## Known limitations

- **Owner-module extensions do not exist.** `private` currently means "inside
  the type", which is what a continuation gives. When extensions arrive, an
  owner-module extension must also be admitted.
- **The address-exposure rule is conservative**, as described above; G08.
- **Nested writeback is not covered.** Properties and subscripts do not exist
  yet, so the writeback cases G08 lists cannot be checked. M14.
- ~~**A mutating method call on a restricted stored member is not checked.**~~
  Closed by the construction work in [M4d](M4d.md)–[M4f](M4f.md): a mutating
  call takes the member's address, so `o->in.bump()` on a `private(set)`
  member is rejected where the write is.
- **No hidden-visibility story.** Access is a source rule only; an internal or
  private member still has an ordinary external symbol, as
  `abi/linkage-and-mangling.md` anticipates.
- **The unnamed-module case is untouched**: with no module, `internal` cannot
  be satisfied by anything but the type itself. G04.

## Runnable demonstration

```
u->token       error: private member 'token' is not permitted here
u->generation  error: internal member 'generation' is not permitted here
u->id += 1     error: writing to private member 'id' is not permitted here
&u->id         error: writing to private member 'id' is not permitted here
u->id          ok
```

## Benchmark checkpoint

Not applicable.
