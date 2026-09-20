# String and StaticString

`String` is a library value type, not a replacement for C pointers or literals.
It owns or retains its UTF-8 storage and has value semantics. COW, small-string
storage, and shared substrings are implementation options.

```c
String name = "Mario"
StaticString diagnostic = "Invalid argument"
const char* legacyName = "Mario";
```

`StaticString` represents immutable static-lifetime text without requiring a heap
buffer or ARC allocation. No public `StringView` type is part of this design.

## Optional text

`String?` with `nil` is the desired optional-string surface. Empty String and absent
String are different states. The wider `T?` proposal is documented separately in
[Nullability](nullability.md).

## Unicode model

Storage is UTF-8. Byte/code-unit views and grapheme iteration must use explicit units.
Do not call both byte count and character count simply an O(1) "length". An integer
subscript is not automatically an O(1) character lookup. The precise String index,
Character, normalization/equality, and slicing contracts remain G10.

## C interoperability

Easy access to `const char*` is a goal, but ownership and termination remain explicit.
A String may have embedded NULs or a shared substring ending before its buffer ends.
Obtaining a C representation can therefore require temporary materialization.

The preferred baseline API is a scoped C-string callback. `.cString`, casts, or
implicit C-parameter conversion remain provisional until their lifetime is defined.
No conversion to mutable `char*` is supplied by pretending read-only storage is
writable. C code retaining a pointer needs a separate owned-copy/lifetime contract.

See [String library design](../stdlib/string.md).
