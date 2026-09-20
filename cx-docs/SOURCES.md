# External Technical References

Consulted during consolidation on 2026-09-20. These primary references support facts
about C/Clang/LLVM and examples of existing API constraints. They do not define Cx,
and an online version is not assumed to match the user's local fork. Record the
actual checkout SHA before using implementation-specific APIs.

## R1 - Clang internals

[Clang CFE Internals Manual](https://clang.llvm.org/docs/InternalsManual.html)

Reference for Parser/Sema collaboration, AST/type/source information, diagnostics,
CompilerInvocation, preprocessing, and serialization responsibilities.

## R2 - Clang AST

[Introduction to the Clang AST](https://clang.llvm.org/docs/IntroductionToTheClangAST.html)

Reference for source-aware AST representation. Reuse decisions in Cx remain project
choices, not claims that every proposed node already exists.

## R3 - Driver and invocation

[Driver Design and Internals](https://clang.llvm.org/docs/DriverInternals.html)

[Clang command guide](https://clang.llvm.org/docs/CommandGuide/clang.html)

Reference for input-language selection, compilation actions, and the separation of
driver/frontend behavior. Cx flags described in this pack are proposed fork interfaces.

## R4 - C23 enum compatibility

[Clang C language status](https://clang.llvm.org/c_status.html)

[WG14 C23 issue 1018](https://www.open-std.org/jtc1/sc22/wg14/issues/c23/issue1018.html)

C23 fixed-underlying enums are an existing C facility, so a colon/base type is not
an unambiguous signal for stronger Cx-only enum semantics.

## R5 - Preprocessing tokens

[GCC preprocessor tokenization](https://gcc.gnu.org/onlinedocs/cpp/Tokenization.html)

[GCC preprocessing output](https://gcc.gnu.org/onlinedocs/cpp/Preprocessor-Output.html)

References for preprocessing-number token boundaries, macro token behavior, and
source-line information. Cx ranges/module ownership need deliberate compatibility work.

## R6 - LLVM IR and pointer representations

[LLVM Language Reference Manual](https://llvm.org/docs/LangRef.html)

Reference for aggregates, calling conventions, linkage, pointer operations, and
non-integral/external-state pointer constraints. Integer masking is not universally
valid for every target pointer representation.

## R7 - Opaque pointers

[LLVM Opaque Pointers](https://llvm.org/docs/OpaquePointers.html)

Reference for IR pointer syntax. Type-erasing an environment pointer is not inherently
a runtime type check, but pointer validity/alignment rules still apply.

## R8 - ARC

[Objective-C ARC specification](https://clang.llvm.org/docs/AutomaticReferenceCounting.html)

A reference implementation/specification to study for ownership, weak operations,
and cleanup. It is not a claim that Objective-C runtime semantics or optimization
passes can be reused unchanged for Cx.

## R9 - Existing Clang modules

[Clang Modules](https://clang.llvm.org/docs/Modules.html)

Reference for module build/configuration issues. Cx `#module` is the project's own
ownership design, not automatically the same feature or flag space.

## R10 - Test infrastructure

[LLVM Testing Infrastructure Guide](https://llvm.org/docs/TestingGuide.html)

Reference for regression/unit/whole-program testing and lit-driven workflows.

## R11 - FileCheck

[FileCheck manual](https://llvm.org/docs/CommandGuide/FileCheck.html)

Reference for selective output/IR property checks rather than brittle complete snapshots.

## R12 - Scoped mutable buffer access

[Swift mutable buffer access documentation](https://developer.apple.com/documentation/swift/arrayslice/withunsafemutablebufferpointer(_:))

An existing API example with scoped pointer lifetime and restricted container access.
It illustrates why Cx mutable COW views need a full lifetime/exclusivity contract,
not merely a pointer-returning convenience method.

## Project sources

The Cx design authority is the user's decisions in the conversation and the attached
`language-updated.zip` and `Cx-Compiler.zip` baselines. This pack contains newly written
project documentation, not a reproduction of these external manuals.
