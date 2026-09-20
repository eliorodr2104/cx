# 0009 - Error Channels and Generated C Bridges

## Status

Accepted semantics; native and bridge machine conventions are versioned implementation work.

## Context

Error handling should preserve the successful return type and support typed protocol-conforming errors, without requiring Result as every function return.

## Decision

Use `throw`, `throw(E)`, `try`, and do/catch with cleanup. General throw erases only where required. Deinit is non-throwing. Allow `extern(C)` throwing definitions when a supported automatic bridge and C header can be generated for the whole signature.

## Consequences

C callers use the generated status/output interface, not the native throwing ABI. Unsupported type/ownership mapping is an error. Result remains ordinary stored outcome data. Compiling does not rewrite handwritten headers. Handler order and partial initialization remain semantic contracts.

## Implementation gates

G07 for cleanup and bridge ownership; B01 for native representation after implementation. The exact C export status/signature format is not invented as a permanently frozen ABI.

## Related contracts

[Errors](../language/errors.md), [Error ABI/C export](../abi/errors-and-c-export.md).
