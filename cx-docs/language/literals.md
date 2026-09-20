# Literal Construction

Cx adds literal conveniences through compiler-known construction hooks rather than
arbitrary user-defined implicit conversion chains.

## Arrays, sets, and dictionaries

```c
Array<int> values = [1, 2, 3]
Set<int> unique = [1, 2, 3]
Dictionary<String, int> counts = ["one": 1, "two": 2]
```

A target type selects the appropriate accepted literal construction. Without a target,
the proposed default for a homogeneous bracket list is the dynamic array abstraction;
its final public name is still pending. Empty literals need contextual element types.
Numeric common-type rules, duplicate dictionary keys, and contextual set construction
must be recorded explicitly before implementation.

## Text

```c
String text = "hello"
StaticString diagnostic = "hello"
const char* cText = "hello";
```

The expected type can select a String/StaticString construction hook. Existing C
literal semantics remain unchanged in C contexts. A standalone `var text = "hello"`
does not yet settle the default between a C pointer and a Cx text value; G10 tracks it.

## Tuple and range forms

Labeled tuple expressions and range operators are covered in their own documents.
Unlabeled tuple syntax overlaps with the C comma expression; compact ranges overlap
with preprocessing numbers. Their supported contexts must be specified, not inferred
from examples that only happen to parse in another language.

## Numbers and names

Cx keeps C numeric spellings unless an explicit addition is approved. Do not infer
`float` from `10f` or invent a `d` suffix without a language rule. User-facing examples
prefer C type names, `size_t`, and standard fixed-width typedefs where included.
A universal `IntN`/`UIntN` naming policy is not finalized.
