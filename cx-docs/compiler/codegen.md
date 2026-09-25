# Code Generation

CodeGen lowers validated Cx meaning directly to ordinary LLVM IR. It does not redo
name lookup, label selection, or language-level type inference.

## Existing and direct lowering

`var`, `let`, resolved labels, and typed null literals generally reuse ordinary
lowering. Struct methods are direct associated functions with a hidden `self` address
or receiver as required. Extensions do not change instance layout. `~mutating` can
use an appropriately read-only receiver contract, but is not proof of no side effects.

## Values, properties, and iteration

Tuples/enums use target-aware aggregates and value operations. Properties/subscripts
lower to direct access or accessor calls with defined writeback. Preserve single
evaluation and error cleanup in compound assignments.

For-in uses the resolved iteration operations; specialization/inlining may remove
iterator/Optional materialization. Closed ranges must not overflow after the final
largest representable bound. Library bounds checks are retained unless proved redundant.

## Callable values

The general ABI is invocation entry plus context reference. A concrete closure body
knows its environment layout; a `void*` explanatory model is not a runtime type check.
Opaque LLVM pointer syntax does not remove alignment/provenance requirements.

Known calls may devirtualize/inline and eliminate storage. Materialized values still
obey one ABI for their static type. A context-free C callback conversion needs a real
C-compatible entry, not a function-pointer cast that omits the hidden context.
Nonescaping stack contexts must not flow into generic ARC retain/release as if heap
allocated. See [Callable ABI](../abi/callables.md).

## Unified cleanup

Normal exit, return, loop exits, and Cx errors share active cleanup infrastructure.
Preserve outgoing result/error ownership before cleanup. A defer can use live locals;
do not destroy every local before executing it. Partial init has initialized-field
cleanup, not full-object deinit.

## Errors and exports

Native error lowering remains an experiment between aggregate, hidden-output, and
target-aware approaches. Use explicit control flow, not accidental C++ exception
semantics. Do not mark all error branches cold without workload evidence.

`extern(C)` throwing definitions can generate a native entry, a C bridge, and C-facing
header declarations. Conversion/lifetime rules must be supported for all exposed types.
The bridge is an actual calling convention adaptation.

## Classes and existentials

Class baseline: one object reference and an inline counter/metadata header on supported
ordinary targets. Runtime metadata supplies destruction/value operations as needed.
Protocol witnesses are not stored once per method in every instance.

An unrestricted `any P` uses one uniform container layout independent of its payload.
Small values/class references can fit inline; larger values may box. Boxed value copies
must preserve value independence rather than introduce writable alias semantics.

## Pointer operations

PointerTag masks/or operations are allowed only on representations where the target
contract validates them. Preserve pointer provenance and C ABI boundaries; a round-trip
through integers is not universally sufficient.

## Optimization contract

Separate correctness from optimization tests. Ordinary LLVM IR is the target; Cx does
not require LLVM to understand source concepts. Runtime-assisted operations and shared
generic witnesses are permitted where semantics demand them. Measure, do not promise,
inlining, code size, allocation elimination, and deduplication.
