# Error ABI and Automatic C Export

## Native outcomes

`R f(...) throw(E)` has one active outcome: an R result or an E error. General `throw`
uses type-erased Error semantics. Source code does not receive a hidden `Result` value
merely because one experimental ABI resembles a tagged aggregate.

Native candidates are aggregate returns, hidden output/status storage, and target-aware
split returns. Their exact register/stack placement is selected at implementation
checkpoint B01. Typed errors need no *additional* heap box merely for error transport,
but their own payloads or general existential storage may allocate.

Only the active outcome is initialized/owned. An error path must not expose an
uninitialized success value to source code or consume an inactive result slot.

## Automatic `extern(C)` bridge

This design accepts throwing exports when all required C mappings exist:

```c
extern(C) int parseCode(const char* input) throw(ParseError)
```

The compiler may emit a native Cx entry plus a C-facing bridge and generated header.
Cx callers still use `try parseCode(...)`. C callers use the generated C signature.

For illustration only, if ParseError has a supported integer-code adapter:

```c
/* Generated C header shape; exact public bridge convention is not frozen. */
int parseCode(
    const char* input,
    int* outValue,
    int* outErrorCode
);
```

A chosen status result identifies which output is initialized. The final adapter
specification must define status values, output pointer validity, ownership/cleanup,
error-code mapping, naming, and language-version compatibility. It must not use
`bool` unconditionally for a C89-facing header.

## Representability

A Cx String, class, closure, or arbitrary payload enum does not become C-compatible
just because a C struct could describe some bytes. Generated bridges need explicit
supported value/ownership mappings or opaque handles plus lifecycle functions.
Unsupported signatures produce useful diagnostics, not unsafe guessed conversions.

The bridge may transport unknown errors only through a specified opaque/error-data
contract. Public generics need concrete exported wrappers or supported specialization
exports, not an untyped generic C symbol.

## Ownership and cleanup

Inputs are borrowed/transferred according to the bridge's documented contract.
On success initialize only the success output; on failure initialize only the error
output. Handle partial construction and existing output storage according to a
specified initialization-vs-assignment rule.

Exceptions/longjmp from arbitrary foreign code are not automatically Cx error-channel
propagation. Cx native cleanup semantics must not be falsely promised across unsupported
foreign non-local transfers.

## Files and symbols

C-facing headers are generated build outputs. They do not overwrite handwritten `.h`
files. The native and bridge entries have distinct consistent identities; taking a
C function pointer selects the C-compatible entry, not the native throwing one.

Cross-language tests compile an actual C consumer separately and link it with the
Cx-produced bridge. All ABI adapters are versioned before public stability.

## Resource types

A C caller could copy a struct with a `deinit`, or leave it without calling it. A
resource type is therefore exported as an opaque type reached only through pointers,
with generated functions to construct and destroy it.

