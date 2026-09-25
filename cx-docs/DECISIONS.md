# Decision Register

Last reviewed: 2026-09-25.

This register separates accepted product direction from provisional design and from
contracts that must be completed at a specific implementation milestone. Use the
[milestone records](compiler/milestones/README.md) for current implementation status.

## Status vocabulary

- **Accepted:** established product direction.
- **Baseline:** an initial representation/strategy, not a stable universal ABI.
- **Proposed:** a later suggestion not unambiguously approved in its complete form.
- **Gate:** a semantic/compatibility decision required before implementing that feature.
- **Checkpoint:** a measurement performed after correct implementations exist.
- **Deferred:** intentionally outside the first design/implementation scope.

## Consolidated decisions

| Area | Decision | Status |
| --- | --- | --- |
| Name | Cx; library/runtime project `libcx` | Accepted |
| Files | Ordinary `.c` and `.h`; no source migration requirement | Accepted |
| Compiler | Extend Clang C, lower to LLVM IR, no primary C transpiler | Accepted |
| Driver | Invocation selects Cx; `clangx` working name; explicit `-x cx` | Accepted direction / final name provisional |
| C versions | Overlay the selected supported C/GNU dialect | Accepted |
| Raw operations | Keep C pointer/allocator semantics; no required unsafe block | Accepted |
| Inference | `var` independently deduces multiple initialized declarators | Accepted |
| Immutability | `let` adds top-level const after inference | Accepted |
| Functions | Return type first; ordinary parameters type-first | Accepted |
| Labels | `Type externalLabel localName`; one-name parameters remain positional | Accepted; G02 covers convenience/synthesis |
| References | Compound names and target typing select overloaded addresses | Accepted |
| Callable interface | `R (label: T)` and unlabeled forms; optional `throw(E)` effect | Accepted |
| Methods | Associated functions, implicit `self`, no instance method storage | Accepted |
| Value receivers | Mutable by default, explicit `~mutating`; lint may suggest it | Accepted |
| Source model | Primary fields/API; reopened implementation; private new helpers | Accepted; G04 validation |
| Access | public/internal/private on struct/class members; setter restrictions | Accepted; G08 mutation paths |
| Modules | Optional `#module`, normal `#include`, owned declarations | Accepted; G09 provenance/build rules |
| Init | Auto when no custom init exists; any custom init suppresses generated constructors | Accepted |
| Deinit | Automatic member cleanup plus optional non-throwing user hook | Accepted |
| Classes | ARC by default; manual reference counting also supported | Accepted semantics; manual spelling pending |
| Properties | Implicit stored access; getter-only shorthand; explicit get/set | Accepted |
| Subscript/literals | Bracket API and collection literal support | Accepted direction; detailed rules gated |
| POP | Defaults/refinement/composition, associated types, `any`, constraints | Accepted |
| Generics | Checked constraints, type parameters, specialization, `.c` bodies allowed | Accepted; artifact/generic witness gates |
| Operators | Explicit associated operations; normal overload engine | Accepted; laws and compound-writeback gates |
| Errors | Error protocol, throw/throw(E), try/do-catch; Result remains data | Accepted |
| C error export | Automatic bridge/header when full C mapping is supported | Accepted direction; bridge ABI gated |
| Cx enums | A body using `case` selects a Cx enum; other enums stay C | Accepted |
| Raw/simple enums | Scoped cases, no implicit integer conversion, `rawValue` out only | Accepted |
| Payload enums | Tuple payloads, tuple label rules, tag plus union layout | Accepted; recursion G12 |
| Enum switch | No fallthrough, exhaustive or `default`, trap on invalid value | Accepted |
| OptionSet | One bit per case, set literals, set algebra, `contains` | Accepted; raw import G12 |
| PointerTag | Implicit OptionSet, low bits from alignment, no wrapper | Accepted direction; G05 boundaries |
| Library | Optional, Result, Span, text, dynamic containers, ranges, protocols, Set/Dictionary | Accepted direction |
| Optional text | String should support nil-capable optional use | Accepted direction |
| General `T?` | Make `?` sugar for Optional on arbitrary non-raw value types | Proposed; not part of the accepted design |
| Dynamic array name | Array vs List vs first-class StaticArray naming | Proposed / unresolved |
| C string conversion | Easy const-char interface; exact cast/temporary lifetime | Proposed / G10 |
| Callables ABI | Entry + context, optimization can eliminate materialization | Baseline |
| Class ABI | Ordinary-target reference + inline refcount/metadata header | Baseline |
| Existential ABI | Uniform static-type layout; inline values or correct boxing | Baseline / buffer size checkpoint |
| Error/enum ABI | Typed outcomes/active payload; exact encoding measured/implemented | Baseline / checkpoints |
| Inheritance/comptime | Future limited OOP, reflection/comptime, package/allocator frameworks | Deferred |

## Superseded or corrected claims

Generated and custom initializers no longer coexist automatically. Result is not the
fundamental error return. General callable labels are not erased from their static
types. The preprocessor now intentionally handles `#module`.

A class vs struct payload cannot change the stored size of the same unrestricted
`any P` type. Raw nullable pointers have no extra null niche. Weak invalidation must
prevent promotion before teardown, not only afterward. C23 typed-enum syntax is not
unique to Cx. Public generic artifacts may need private implementation dependencies.
Mutable COW views and C-string casts need lifetime contracts, not merely optimization.

These corrections preserve the intended design while removing claims that were too
strong or technically incomplete. Their gates are listed in [OPEN-ISSUES](OPEN-ISSUES.md).
