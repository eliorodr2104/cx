# Enum and Tuple ABI

## C first

Ordinary C enums, including supported fixed-underlying C enums, keep the selected C
representation and conversion behavior. A modern Cx raw enum needs an unambiguous
source opt-in before stronger semantics can be assigned to it.

A modern raw enum uses its declared backing integer. OptionSet stores a bitmask with
a valid-bit policy. Standalone flag representation and pointer-tag bit placement
must not be confused.

## Payload enums

The general model is a discriminant and storage large/aligned enough for the largest
payload. Exactly one case's payload is alive. Copy/destruction dispatch only to that
active payload's value operations.

For reassignment, preserve the incoming value/ownership before destroying the old
payload if aliasing or self-assignment is possible. A simplistic "destroy old, then
read new" sequence is incorrect when the new expression depends on the old value.
The temporary new state must be cleaned on failure.

## Niches

A niche is a representation unavailable to every valid live value of the payload.
A non-nil class reference may provide an empty representation for an extra enum case.
A raw `T*` already permits null, so null cannot encode an additional absent case while
also representing a present null pointer.

Do not assume that alignment bits, integer holes, or pointer integer casts are
universally available niches. The optimizer must preserve C-visible values and all
supported target representations.

No source payload `rawValue` reveals an unstable discriminant unless explicitly
specified. Public layout evolution needs ABI versioning/resilience rules.

## Tuples

Tuple fields preserve declared element order with target alignment/padding. Labels
have no runtime storage. Structurally compatible label changes do not reorder fields.
Managed element copy/destruction is synthesized in the same value model as structs.

Layout similarity alone does not establish C ABI interchangeability with a handwritten
struct. Cross-language parameters/returns need the explicit export layout/adapter.

## Deferred layout choices

Tag widths/offsets, optimized niches, recursion/indirection, and public resilient
layouts are fixed with the representation implementation. B07 compares correct
representations; it does not decide whether null is a valid raw-pointer value.
