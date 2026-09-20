# AST Strategy

Reuse Clang semantic entities when their meaning matches. Add new nodes/types when
Cx ownership, control flow, access, or source fidelity cannot be represented faithfully.
Names below are conceptual, not committed C++ class names.

## Reuse

- `var`/`let`: ordinary variable declarations with independent deduced types and source
  spelling; `let` adds top-level const/binding semantics.
- Ordinary C records/functions/pointers: existing semantics remain existing entities.
- `null`: typed null-literal machinery where compatible, retaining Cx spelling.
- Parameters/calls: ordinary semantic structures augmented with labels/source ranges.

## Dedicated semantics

Callable types retain parameter labels and effects. Closure expressions retain captures
and lexical bodies. Tuple types/expressions/patterns retain positional structure.
Try/throw/do-catch/defer and for-in/ranges retain their semantic constructs until
lowering. Optional construction/injection/projection must remain distinct from raw
null pointer operations.

Protocol declarations, associated types, conformances, conditional requirements, and
extensions need explicit semantic representations. Classes are not secretly C++
value records. Methods can be FunctionDecl-like entities with associated owner and
implicit `self`, without adding a source-visible parameter.

## New source-model metadata

Record primary type identity separately from continuation blocks and extensions.
Retain stored/computed distinction, original field order/default expressions, declared
member access, setter access, receiver mutation, generated-vs-user init status, and
module ownership.

An implementation links to the interface declaration it satisfies. Reopening does
not create another nominal type or append hidden fields. Private helpers remain
associated declarations with their access contract.

## Synthesized entities

Generated init/copy/destroy/accessor/OptionSet/witness declarations carry source origins
and synthesis reasons for diagnostics. A user-declared init suppresses constructor
synthesis but not required member destruction. Generated C export thunks refer to
native entries without replacing their Cx source type.

## Canonical types

Canonicalization reflects semantics, not merely equal machine size. Callable labels
and effects can distinguish static types. Tuple labels may be source sugar with
structurally compatible positional types. C tag namespaces and typedef shadowing
must remain correct when adding convenient type names.

## Integration obligations

Every added node/type is reviewed for visitors, AST dump, serialization/PCH, imports,
structural equivalence, dependency flags, diagnostics/source ranges, CodeGen, and
Clang tooling. Code generation success alone is insufficient.

An AST may expose a high-level expression plus a semantic expansion when useful;
there is no rule that "all lowering must wait for CodeGen" if an earlier representation
preserves semantics/source information and improves reuse.
