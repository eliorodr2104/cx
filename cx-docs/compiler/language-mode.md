# Language Mode and Driver Interface

The public working command is `clangx`. Its final installed spelling remains a naming
decision, not a reason to duplicate a compiler implementation.

## Planned invocation

```sh
clangx file.c
clang -x cx file.c
clangx -x c file.c
clangx -std=c11 file.c
clangx -std=gnu23 file.c
```

These are target interfaces for the modified compiler, not commands asserted to
work before the mode is implemented.

`clangx` defaults ordinary `.c` sources to Cx. Explicit `-x c` wins. Normal `clang`
continues interpreting `.c` as C. Object files, archives, and assembly inputs must
not be reclassified as Cx by a blanket driver flag.

Language options apply with the driver's normal per-input ordering rules. Test mixed
input kinds and resets such as `-x none`, rather than treating all inputs as identical.

## Independent C dialect

Use the selected C standard plus a Cx mode flag. Cx `var` can reuse newer deduction
machinery without making old-mode C `auto` acquire newer semantics. Preserve the
selected GNU/Clang extension behavior within the supported compatibility contract.

## Source identity

No magic byte header or mandatory source-identification marker is required.
`#module` is optional ownership within Cx mode; it is not the mechanism that turns
an ordinary compiler into a Cx compiler.

## Feature macros

A Cx availability macro and version macro are intended. `__CX__` remains a provisional
compiler-macro spelling, not the spelling of the language name. Its final encoding
must be documented with the driver feature. Existing C standard macros continue to
describe the selected C baseline.

Feature macros intentionally distinguish C from Cx. Differential tests must normalize
or classify that difference instead of claiming identical macro environments when
they are not identical.

## Private interface

Internal `-cc1` flags are not stable user APIs. The mode must round-trip through
CompilerInvocation, PCH/module compatibility, tooling, and compilation databases.
The final mode representation and flags are implementation choices.
