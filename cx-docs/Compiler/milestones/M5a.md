# M5a — Implicit tag names

Status: **Planned** (designed, not implemented).

Baseline: the pinned checkout recorded in the [M0 record](M0.md).

First slice of roadmap row M5, and one of the six collisions
[G01](../../OPEN-ISSUES.md#g01--c-grammar-collisions) names outright:
*"implicit tag-name/type conveniences"*.

## What it is

```c
struct Size { int width; int height; };

Size s = Size(width: 80, height: 40);   // no typedef anywhere
void take(Size v);
Size make(void);
```

A tag declared with `struct`, `union` or `enum` is usable as a type name on
its own. The `typedef struct X { ... } X;` ceremony becomes optional, and
`struct X` keeps working everywhere it works today.

## Why it is not just a convenience

Two things in the language as it stands need it.

**A type cannot name itself.** Inside its own method or initializer, the
typedef does not exist yet — `typedef struct S { ... } S;` introduces `S` only
after the whole declaration, and method bodies are replayed before that:

```c
typedef struct S {
    int a;
    init(int a v) { self.a = v; }
    ~mutating int twice(void) { S o = S(a: a * 2); return o.a; }
    //                          ^ error: must use 'struct' tag to refer to type 'S'
} S;
```

**Construction needs a type name in expression position.** `Size(width: 80)`
is reached through `Sema::getCxConstructionType`, which asks `getTypeName`, so
today it works only for a typedef'd or annotated name. [M4d](M4d.md) recorded
this as a limitation and pointed at G01.

## The rule

The same one C++ has, which is why C++ compiles `struct stat` alongside
`int stat(...)` without ambiguity:

- A tag name is visible to ordinary lookup as well as tag lookup.
- **An ordinary declaration of the same name hides it.** A variable, function,
  parameter, enumerator or typedef named `stat` hides `struct stat`, which
  then needs its elaborated spelling. This is what keeps existing C headers
  working: every C program that reuses a tag name for something else keeps its
  meaning, and `struct X` never stops being available.

In Clang this is one line of mechanism — `LookupOrdinaryName` includes
`Decl::IDNS_Tag` in C++ and would in Cx — plus `LookupResult`'s existing
tag-hiding, which implements the second rule already. The work is not the
mechanism; it is the evidence G01 demands.

## Permitted baseline C modes

Every C/GNU standard, as in M0, and that is the point: the rule must hold from
C89 through C23 and the GNU dialects alike, because it changes name lookup
rather than syntax. A file that is not in Cx mode is untouched.

## What G01 demands before this ships

> Use an explicit rule or arbitrary lookahead that changes a valid C program.
> **Evidence:** C/gnu dialect matrix, macro cases, `-E` round trips, and
> AST/behavior comparison.

Concretely, before this is enabled:

1. **A regression corpus.** Real C headers compiled in Cx mode with and
   without the rule, with the ASTs compared. The system headers of the pinned
   SDK are the obvious first corpus, plus the `clang/test/C` tree.
2. **The tag-hiding rule tested per dialect**, C89 through C23 and each GNU
   variant, since implicit-int and the enum rules differ across them.
3. **The known C idioms named and tested**: `struct stat` with `stat()`,
   `struct timeval` with a local `timeval`, a tag and a typedef of the same
   name (`typedef struct X X;`), a tag and an enumerator, a tag shadowed in an
   inner scope.
4. **`-E` round trips**, because the rule must not depend on anything the
   preprocessor removes.

## Open questions for the design

- **Does the tag name get Cx linkage or module ownership of its own?** A type
  is not an entity with a symbol today, so probably nothing changes; M15 may
  disagree when module artifacts arrive.
- **Does a tag declared in an unowned C header become implicitly usable in an
  owned file?** The conservative answer is yes — visibility is not ownership —
  but it is worth stating rather than falling out.
- **`enum` tags.** The same rule, but enum constants are already in the
  ordinary namespace, so an enum tag and a constant of the same name need an
  explicit answer.
- **Anonymous and typedef-only tags** are unaffected.

## Known limitations of the plan

- It does not remove the other five G01 collisions, which still gate the rest
  of M5: tuple versus comma expression, raw enums, compact ranges, generic
  angles and trailing closures.
