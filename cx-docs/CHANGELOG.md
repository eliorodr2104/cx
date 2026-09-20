# Consolidation Changelog

## Input baselines

This revision uses the available `language-updated.zip` (25 Markdown files) and
`Cx-Compiler.zip` (13 Markdown files), then incorporates the later design discussion.
The original `language.zip` is not substituted for the later updated baseline.
No files on the user's Mac were inspected or changed.

## Updated existing areas

All existing language and Compiler filenames are retained. The language name is Cx.
The function syntax remains type-first with optional explicitly declared labels,
compound references, labeled/effectful callable types, and `self` receivers.

Variable inference now documents independent multiple declarators and top-level let
const semantics. Source/type continuations, access control, receiver mutation,
properties/subscripts, constructor suppression, cleanup, POP, constrained generics,
modules, and operator capabilities are linked across the documentation set.

The error model uses Error + throw/try/do-catch, not Result as a compulsory return.
Automatic throwing C-export bridges are now supported by design under explicit
representability/ownership rules. Callable labels/effects and the uniform materialized
entry/context ABI are no longer omitted.

## New areas

Added numbered design rationale records, ABI contracts, runtime ownership/weak/cleanup
notes, and the standard-library surface. Compiler includes a dependency-correct
roadmap, feature matrix, module artifact integration, and implementation benchmark
checkpoints. Root decision/gate/source/validation registers make status explicit.

## Technical corrections

- C23 fixed-underlying enums cannot automatically select new Cx semantics.
- Tuple/comma and range/preprocessing collisions are real parser gates.
- Custom initializer statements cannot be silently reordered to field declaration order.
- Private(set), subscripts, and compound assignment need writeback/address rules.
- One unrestricted `any P` type cannot change layout with runtime payload category.
- A boxed struct existential must preserve value independence, including on mutation.
- Weak promotion must stop before teardown exposes a deinitializing object.
- Raw nullable pointers do not provide an extra null niche for Optional<T*>.
- A C function callback needs an ABI-correct entry, not a hidden-context pointer cast.
- Public generic serialization can require internal implementation dependencies.
- Several `.c` inputs do not automatically share all semantic state.
- Imported C headers cannot inherit the consumer's Cx linkage by accident.
- COW mutable spans and C-string pointers require lifetime/exclusivity contracts.
- Correctness gates are separated from measurements of completed implementations.

## Not silently finalized

General T? sugar, dynamic-array naming, same-name label convenience, manual-RC spelling,
String cast API, exact artifact/mangling/layout encodings, and other feature-specific
contracts remain explicitly classified. Existing accepted design is not replaced by
unapproved syntax merely to make the document set look complete.

## Delivery

The combined archive contains `cx-docs/`. A separate unified diff compares the normalized
38-file baseline with the consolidated tree, including newly added documents. Its paths
are relative to the `cx-docs` directory. Review/check the patch against local changes
before applying it. Archive/file integrity and scope are documented in VALIDATION.
