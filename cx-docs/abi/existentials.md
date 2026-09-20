# Existentials and Witness ABI

## One static existential type, one layout

An unrestricted `any Drawable` can hold a class reference or a struct value. It must
have one materialized layout and calling convention for all those values.
The earlier idea of a two-word layout whenever the runtime payload is a class and a
larger layout whenever it is a struct is not a valid fixed ABI for that same type.

The baseline general representation uses inline value storage, type/value-operation
metadata, and witness/conformance references. A class reference can occupy the inline
storage. A statically *class-constrained* existential could have a specialized smaller
ABI after its source contract is defined. Optimization of an unmaterialized known
existential is a separate matter.

## Inline and boxed storage

Inline eligibility depends on size and alignment. Otherwise, storage refers to an
out-of-line value box. Metadata supplies projection, copy/assignment, destruction,
and storage management as needed.

Boxing must preserve the payload's semantics. Copying an existential containing a
struct creates another logical value; merely retaining one writable shared box would
turn it into reference semantics. Use eager copy or valid COW/detachment. A class
payload instead copies a strong reference to the same object.

## Witness data

Witnesses implement protocol requirements. A property can require getter/setter
entries. Refinement and associated-type/conformance metadata are additional entries
or descriptors as needed. Tables need not contain extension-only methods.

A conformer's own matching implementation is preferred to an applicable default.
Conformance construction resolves that choice once under the declared rules. It does
not select whichever module happened to load first.

Witness entries may be ABI adapters rather than the direct address of a concrete
method: the erased receiver/result shape may differ from its natural concrete ABI.

## Generic and static requirements

A generic protocol requirement cannot be implemented solely as one monomorphization
when future callers can choose new types. Supporting such calls through `any` requires
a shared generic entry with metadata/witness arguments, controlled instantiation,
or an explicit restriction on that existential operation.

Static requirements and `Self`-dependent inputs similarly need an appropriate
metatype/opened-existential contract. Accepting their declarations does not prove
all erased calls are valid. This is G06, not a buffer-size benchmark.

## Composition and associated types

A composition initially carries one suitable conformance reference per needed
protocol, with deduplication/refinement handled consistently. An existential with
`Element == String` need not add storage just for that equality; it validates the
conformance when constructed and retains the static constraint.

## Measurement

B04 chooses inline-buffer capacity after correctness. Generic constrained calls need
not materialize existentials; shared generic implementations can still use witnesses.
