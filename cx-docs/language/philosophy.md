# Philosophy

Cx (pronounced "see-ex") is a conservative extension of C. The name is **Cx**,
not an all-capitals variant. `libcx` names the runtime/standard-library project,
not the language itself.

The goal is incremental adoption in ordinary `.c` files: existing C remains C,
and a programmer can introduce Cx features into the same translation unit.
The implementation extends Clang rather than translating generated C as its
primary compilation strategy.

## Principles

- Preserve defined C behavior, its preprocessor, raw pointers, ordinary declarations,
  object layout, and ABI under the selected C dialect and target.
- Keep function declarations return-type-first and parameters type-first.
- Prefer value types, protocols, composition, and extensions. Classes provide
  identity and ARC; class inheritance is deferred, not rejected forever.
- Provide automatic reference counting and an explicit manual-RC facility without
  changing the meaning of the C pointer declarator `*`.
- Do not require an `unsafe` language mode to access C's low-level facilities.
- Permit clear abstractions without requiring them to allocate, box, or dispatch
  dynamically when their semantics do not require it.
- Keep syntax, implementation strategy, and binary ABI separate. An optimization
  opportunity is not a promise that every build removes every cost.

## Scope

This is a design baseline, not a claim that the fork implements the language.
An accepted surface feature can still have an unresolved interaction with another
feature. Those interactions are recorded in [the decision register](../DECISIONS.md)
and [implementation gates](../OPEN-ISSUES.md), rather than hidden behind a claim of
complete C compatibility.

The standard library supplements libc. It does not replace `printf`, `malloc`,
C system headers, target builtins, or platform APIs.
