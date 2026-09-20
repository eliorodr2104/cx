# Class Reference and Object ABI

## Baseline, not a universal hardware guarantee

On the initial conventional pointer targets, a class reference is one object pointer.
A nullable class reference can use the same reference width with an empty value.
Do not generalize "one machine word" into a guarantee for capability or other unusual
pointer representations.

The baseline object header has a strong ownership/control word and a type-metadata
reference, followed by fields in the specified layout order. Exact offsets, encoding,
and counter/weak-state sharing are implementation choices to validate before freezing
an ABI. The entire object includes its header; the reference does not.

## Metadata

Metadata provides size/alignment and the required destroy/deallocate operation, plus
runtime type identity. It is not automatically a vtable containing every declared
method. Optional reflection data is not mandatory baseline overhead.

A metadata address can identify a registered type within one runtime, but is not by
itself a stable serialized identity across processes, compiler versions, or dynamic
libraries. Module/type identity and registration/canonicalization must support those
boundaries explicitly.

## Dispatch and conformance

Ordinary known class methods use direct associated calls unless later language
semantics require dynamic dispatch. Protocol witnesses are separate conformance data;
objects do not gain one pointer per adopted protocol.

## Destruction

At the last strong release, transition the object to a state from which weak promotion
cannot acquire a new live reference. Invoke user deinit if construction completed,
destroy managed fields, and deallocate through the correct allocator/metadata entry.
A runtime function receiving an erased object can use metadata; optimized typed
release paths may specialize this work.

Weak bookkeeping is lazy/out-of-line where practical. The discovery mechanism and
header/control-word state are part of the runtime design; an out-of-line table does
not appear without a way to locate it.

## Stability and tests

Layout/version validation, cross-TU construction/destruction, partial failure,
over-alignment, zero-state references, and DSO identity are prerequisites to stable
interop. Benchmark B03 compares only race-correct ownership designs.

See [Runtime Ownership](../runtime/ownership.md).
