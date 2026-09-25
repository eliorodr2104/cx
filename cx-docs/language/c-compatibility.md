# C Compatibility

## Contract

For the same supported Clang revision, target, C dialect, macro configuration,
and relevant compiler options, valid C source must retain its defined meaning
when compiled as Cx. The compatibility suite also covers selected Clang/GNU
extensions; accepting arbitrary invalid C after recovery is not the language contract.

Cx overlays the selected dialect, rather than silently upgrading it. For example,
C11 `auto` keeps C11 meaning even if Cx `var` reuses a newer deduction implementation.

```c
#include <stdio.h>

int main(void) {
    int value = 42;
    printf("%d\n", value);
    return 0;
}
```

## Preserved facilities

C preprocessing, headers, identifiers and namespaces, declarations, pointers,
arrays, unions, enums, atomics, volatile access, selected builtins, and ordinary
C linkage remain available. Existing `NULL` is not removed or redefined by Cx.
Code using only C does not acquire mandatory Cx-runtime dependencies.

New words are contextual where needed:

```c
typedef int var;
var value = 42;
int class = 1;
int null = 2;
```

Macro expansion occurs before contextual interpretation. Cx must not reinterpret
these declarations merely because their names have Cx meanings elsewhere.

## Additions are not blanket reinterpretations

`int width` is an unlabeled C parameter. `int width newWidth` explicitly adds a
label. Semicolon elision may accept new text but must preserve existing C parsing.

There are compatibility problems that keyword handling alone cannot solve:

- `(a, b)` is already a C comma expression.
- Fixed-underlying-type enum syntax is already supported by C23.
- Adjacent digits and periods participate in preprocessing-number tokenization.
- An ordinary C tag name need not be available as an ordinary type identifier.
- Trailing closures can overlap with a call followed by a compound statement.

The [parser contract](../compiler/parser.md) and [G01](../OPEN-ISSUES.md#g01--c-grammar-collisions)
require explicit disambiguation tests before those extensions are enabled.

## Opt-in modules

`#module` is a Cx addition that explicitly supplies ownership and Cx linkage defaults.
It is not inserted into unmodified C files. Imported C declarations retain C
linkage; including a C header from a module must not rename its functions.
See [Modules](modules.md).

## Diagnostics and boundaries

Different explanatory diagnostics are allowed, but new style warnings must not
break otherwise successful C builds using `-Werror` by default. Undefined behavior
is not assigned a new promised result. Implementation-defined C choices continue
to come from the selected Clang target and configuration.

Cx source containing Cx features need not compile with an ordinary C compiler.
Generated C-facing declarations are covered by [C export](../abi/errors-and-c-export.md).
