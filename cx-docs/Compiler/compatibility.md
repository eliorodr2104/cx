# Compiler Compatibility Contract

## What is compared

Use the same pinned Clang baseline, target, selected C dialect, and equivalent relevant
flags. Valid C must remain valid and retain its defined/implementation-selected
meaning. Acceptance of invalid C after recovery is not automatically a supported
semantic contract. Deliberate Cx macro differences must be classified in testing.

No promise is made to reproduce a particular result of undefined behavior or bytewise
identical optimized assembly. New style warnings must not silently break C builds
using warnings-as-errors.

## ABI and headers

Ordinary unowned C declarations preserve C linkage, layout, and calling conventions.
`#module` is explicit Cx ownership/linkage opt-in for new entities. Existing imported
C entities retain their identity; system headers are not renamed by the consumer's
module setting.

Struct methods/access restrictions do not add fields. A Cx type containing ARC state
or nontrivial value operations is not a plain C aggregate merely because its bytes
can be described as a struct.

## Required conflict tests

Test identifiers/typedefs/macros named like Cx words; tag/ordinary namespace overlap;
comma expressions versus tuples; C23 fixed-underlying enums; generic angle brackets
versus relational operators; C compound blocks versus trailing closures; compact
range preprocessing tokens; and semicolon elision around C control flow.

A contextual-keyword approach is an implementation technique, not a proof that all
these distinct collisions are resolved.

## Modules and preprocessing

Header ownership must survive preprocessing/PCH/import. Macro expansion needs an
ownership rule distinct from display filenames. Build root ownership cannot turn
unmarked foreign headers into local internal declarations.

Module declarations are not a security boundary. Source-level access remains governed
by trusted build identity plus Sema; native code can still use raw memory operations.

## C exports

`extern(C)` may request automatic throwing bridges. Eligibility means a supported
ABI/lifetime mapping for the entire exposed signature. Unsupported managed payloads
are errors unless explicit adapters/handles exist. C-facing headers are generated
outputs, not edited source inputs. Generic C APIs require concrete wrappers/exports.

## Evidence

Differential compile/execute tests, cross-language link tests, ABI layout checks, and
header corpus tests provide evidence. They do not prove equivalence for all programs.
Use the user's C dictionary unchanged as a later real-world case once its actual
source/build/license are available; it has not been inspected in this documentation.
