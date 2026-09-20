# Linkage, Names, and Symbol Visibility

## Identity rules

Ordinary unowned C entities retain C linkage. New explicitly module-owned entities
use Cx linkage by default. Methods, overloads, generics, and other Cx-specific identities
use Cx naming as required. `extern(C)` requests a C-compatible entry/bridge.

A declaration already imported from an ordinary C header is still that C entity.
Adding a definition in a module must not silently rename it and leave the original
C symbol unresolved. The implementation matches the existing identity or diagnoses
a conflict; explicit adapters are separate declarations.

## Semantic mangling input

Use canonical module/type ownership, base name, argument labels, parameter types,
generic signature/arguments, relevant receiver/effect/type information, and generated
entity kind. Local parameter names, absolute paths, line numbers, and build directories
are not public identity.

Return type or throwing effect alone does not permit overloads. They may still need
to appear in an ABI signature/fingerprint to detect incompatible declarations.
Changing a public type's effect/representation is not made compatible by leaving
its visible base name unchanged.

The binary encoding is versioned. One mangling framework serves functions, metadata,
conformances, witnesses, thunks, and specializations. Module identity is not a universal
package-version resolver.

## Access is not linkage

`public`, `internal`, and `private` control source access, not declaration identity.
An internal/private member called across translation units may need a linkable symbol.
Object-file hidden visibility does not enforce arbitrary Cx module membership after
linking, and a native module is not a security boundary.

The C `static` function remains translation-unit local. Module-owned internal symbols
can be hidden at an appropriate library boundary where the ABI/build supports it.

## C exports

One C external name cannot represent several incompatible overloads. A throwing export
has a C bridge distinct from its native Cx entry. The C bridge's generated declarations
and exact parameter mappings are part of its ABI. Generic exports need concrete
instantiations/wrappers under the approved contract.

## Specialization and coherence

Identical generic specializations need deterministic identities and correct target
linkage/deduplication. COMDAT/link-once availability differs by object format and is
not assumed universal. Conditional bodies/dependency versions must agree before two
copies are treated as interchangeable.

Conformance conflicts must be checked using program/module metadata. An ordinary
linker will not automatically prove protocol coherence merely because symbols were
mangled. Dynamic loading requires an additional policy before support is claimed.
