# String and StaticString Library Design

## String

String is a UTF-8, value-semantic library type. It owns or retains the storage required
for its lifetime. COW, small-string optimization, and shared substring ranges are
allowed implementations, not visible ownership differences between String values.

No public StringView abstraction is planned. A String substring can retain shared
backing storage while behaving as an independent value. That can retain a large
source buffer; copying/shrinking policy should be explicit and measured, not hidden
behind a promise that every substring is always allocation-free.

## StaticString

StaticString represents immutable static-lifetime text, normally a literal. It does
not require ARC/heap ownership solely to keep its backing bytes alive. Pointer plus
byte count is a plausible representation; exact layout/termination remain ABI choices.
A static byte sequence can still contain an embedded NUL.

## Units and Unicode

Expose byte/code-unit operations separately from grapheme-oriented Character iteration.
A code unit, scalar, and user-perceived character need not be the same unit. Do not
promise constant-time ordinal character indexing on variable-length UTF-8.

The final index type, grapheme implementation, normalization/equality/hash policy,
substring boundary validation, invalid-UTF-8 handling, and operation complexity form
G10. `Character` may require more storage than one byte or one scalar. Equality and
hashing must agree on the selected normalization semantics.

## C conversion

A C string requires readable NUL-terminated bytes for the receiver's required lifetime.
String storage can be shared, offset, not terminated at the substring boundary, or
contain embedded zeros. Therefore a cast cannot universally be zero-copy and cannot
promise ownership transfer.

The recommended baseline is a scoped operation such as:

```c
text.withCString { pointer in
    legacyPrint(pointer)
}
```

Here the pointer is read-only and valid for the callback's documented duration; a
C callee must not retain it. The name is an API draft. Allocation failure and encoding
policy must be documented. An escaping C copy needs a separate owning-copy/free contract.

`.cString`, an explicit cast, or a special C-argument conversion remain proposals,
not settled lifetime-safe APIs. Mutable `char*` conversion is not implicit.
For variadic C APIs such as printf, do not infer a String conversion from format text
without an explicitly supported bridge.

## Literals and optional text

An expected String or StaticString can select a literal construction hook. Ordinary
C string literals retain their C behavior. `String?` distinguishes no String from an
empty String; wider optional sugar is G03. Context-free literal inference is not yet
fixed by choosing an example in documentation.
