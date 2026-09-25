# Implementation Gates and Open Surface Decisions

The broad design is ready to document and implement incrementally. This register
records what remains to make each affected feature precise. It does not ask for all
answers before M0/M1, and it does not classify every unanswered question as a benchmark.

A **gate** fixes semantics/compatibility before enabling its feature. A **checkpoint**
measures correct implementations afterward. See [Compiler/benchmarking.md](Compiler/benchmarking.md).

<a id="g01--c-grammar-collisions"></a>
## G01 - C grammar collisions

**Before:** broad parser enablement (M3-M6).

Specify tuple vs C comma expression interpretation, fixed-underlying C23 vs modern
raw enums, compact range preprocessing-number handling, generic angles, trailing
closures, and implicit tag-name/type conveniences. Contextual keywords alone do not
resolve these collisions. Preserve macro stringification/token-pasting and legacy
scalar comma expressions. Use an explicit rule or opt-in, not arbitrary lookahead
that changes a valid C program.

**Evidence:** C/gnu dialect matrix, macro cases, `-E` round trips, and AST/behavior
comparison. Modern raw-enum opt-in spelling remains a surface decision.

<a id="g02--labels-and-synthesized-construction"></a>
## G02 - Labels and synthesized construction

**Before:** final labels/initializer/payload API (M3-M6).

Ordinary one-name parameters remain positional; two names introduce an explicit label.
Decide whether matching external and local names can use a shorter declaration, and
whether a declared label can be omitted at a call. Do not infer either policy.
Generated memberwise calls can expose field names without literal `int x x` source
parameters; decide whether they also accept positional calls. Align enum payload
naming, callable labels, trailing closure label omission, and default arguments.

**Evidence:** declarations/implementations/callables/compound references all agree.

<a id="g03--optional-sugar"></a>
## G03 - Optional sugar

**Before:** optional type/literal implementation (M10).

Optional String behavior and `Optional<T>` are accepted directions. General `T?`
sugar for every value type remains provisional. Decide whether it is the language-wide
model or a narrower String facility. Preserve raw-pointer behavior and distinguish
explicit `Optional<T*>` from a nullable `T*`.
Choose checked extraction, chaining/defaulting, and any force/try-optional operators.
Nested optional absence must not collapse accidentally.

**Evidence:** String absence vs empty String, class absence, nested optionals, present
null pointer, and literal-context diagnostics.

<a id="g04--source-model-and-visibility"></a>
## G04 - Source model and visibility

**Before:** full separate implementation/access (M4/M15).

Specify no-module private/internal scope, top-level type defaults, continuation
recognition vs nested C tag shadowing, forward/incomplete class/protocol uses, duplicate
implementations, and the exact shared-header/inline body rule. New helpers must not
silently absorb a misspelled intended interface implementation. A public method body
cannot become visible in another TU without a declaration/artifact loading rule.

**Evidence:** separately compiled header/implementation pairs, duplicates, macro
interfaces, mismatched access/effects, and owner/foreign extensions.

<a id="g05--pointer-tagging-boundaries"></a>
## G05 - Pointer tagging boundaries

**Before:** enabling PointerTag (M17).

Specify supported targets/address spaces, packed/misaligned C-extension pointers,
provenance, casts/void pointers, equality/hash identity, null with tags, arithmetic and
indexing, atomics/volatile slots, aliases, foreign calls, and schema visibility across
TUs. Alignment capacity is a type property, not proof that arbitrary pointer bits
name a valid aligned allocation. More than one tag schema needs a coherence rule.

**Evidence:** each operation on tagged and untagged values, unsupported-target errors,
C calls with explicit clean pointers, and atomic update tests. Performance comes later.

<a id="g06--generic-witnesses-and-artifacts"></a>
## G06 - Generic witnesses and artifacts

**Before:** dynamic generic/static protocol calls and downstream specialization
(M15-M16).

A generic requirement through `any` needs shared generic invocation or another explicit
strategy; one monomorphized function pointer cannot serve arbitrary future types.
Specify metatype/static invocation and Self-opening restrictions. Public generic bodies
can depend on internal helpers; artifact contents/access and helper symbol availability
must account for those dependencies without making them public source API.

**Evidence:** separately compiled consumer types unknown to the provider, generic
requirements through erased values, private helper dependencies, overlapping defaults,
and conditional/retroactive conformance conflicts.

<a id="g07--ownership-and-failure-contracts"></a>
## G07 - Ownership and failure contracts

**Before:** relevant managed runtime feature (M7/M11-M13).

Choose manual-RC declaration spelling, API-visible +0/+1 return/transfer rules, mixed
ARC/MRC field/capture behavior, resource-owning value-copy policy, shared/weak capture
syntax, and escaping restrictions. Specify refcount synchronization/thread-transfer,
weak promotion/destruction order, overflow/resurrection policy, OOM, constructor
failure, default/delegation state, and C non-local control flow across managed scopes.

These are correctness decisions. Atomic vs non-atomic performance cannot decide
whether data races are acceptable. A two-word closure cannot blindly ARC-release a
stack context without an ownership distinction.

**Evidence:** self-assignment, aliasing, every partial-init failure point, weak races,
stack/heap context copies, C callback retention, and declared failure behavior.

<a id="g08--writeback-and-mutable-views"></a>
## G08 - Writeback and mutable views

**Before:** writable properties/subscripts/COW APIs (M14).

Specify getter/setter effects, read-only receivers, nested value mutation, in-place
modify access, mutable address-taking, setter visibility, and single-evaluation rules.
An unrestricted mutable Span can invalidate COW value independence if sharing changes
while the span is live. Use a scoped exclusivity contract or another explicit scheme.
The library must not call unrestricted raw exposure safe merely because uniqueness
was tested once.

**Evidence:** `items[index()] += rhs`, setter counts, aliasing getters, private(set)
address escape, COW clone-before/after-view cases, and failure during writeback.

<a id="g09--module-build-contract"></a>
## G09 - Module/build contract

**Before:** module artifacts and cross-module APIs (M2/M15).

Define ownership of macro-expanded declarations, guards/conditional #module, preprocessed
input, build root assignment versus mapped headers, source qualification for same-named
module types, cache fingerprints, deterministic parallel artifact assembly, and dynamic
library conformance registration. Imported C headers must retain C linkage.

An ordinary multi-file compiler invocation does not share all ASTs automatically.
A trusted build establishes module ownership; it is not a security boundary against
hostile native code. Exact CLI/format/mangling encodings are implementation decisions
but must be versioned before distribution compatibility is claimed.

**Evidence:** separate producer/consumer builds, stale/missing artifacts, macro variants,
foreign C includes, incremental rebuilds, and coherent specialization deduplication.

<a id="g10--text-and-container-surface"></a>
## G10 - Text and container surface

**Before:** public library API (M18).

Resolve Array/List/StaticArray naming. The existing fixed C array is available, but it
is not a first-class copyable StaticArray value. Define String indexing/Character,
normalization/equality/hash, invalid UTF-8, shared-substring retention, default literal
inference, and C-string pointer lifetime/termination/embedded-NUL behavior.

A scoped const-char callback is a safe baseline proposal, not a final acceptance of
implicit casting. No StringView type is included. General integer aliases and Any as
an opaque pointer/value remain possible designs, not language primitives.

<a id="g11--collections-and-algorithms"></a>
## G11 - Collections and algorithms

**Before:** full iterator and container surface (M10/M18).

Specify iteration binding mutability, reference iteration, repeated exhaustion,
invalidation, count/index complexity, checked-bounds failure, reversed/stepped ranges,
allocation failure, and throwing callback effects. Determine sorting/order laws,
dictionary duplicate key/setter semantics, hash stability context, and mutable keys.
Filter/map eagerness and result ownership must be explicit.

**Evidence:** end/empty cases, max-bound integer ranges, mutation during iteration,
optional dictionary values, floating-point ordering, and allocation failure.

<a id="g12--enum-and-optionset-details"></a>
## G12 - Enum and OptionSet details

**Before:** enum API/layout release (M6/M16).

Specify recursive payload indirection, modern switch fallthrough/exhaustiveness,
raw-value construction, unknown OptionSet bits, composite cases, backing-type defaults,
complement universe, multi-flag membership naming, and public schema evolution.
A raw nullable pointer does not provide an unused null niche. Enum replacement must
preserve an aliased incoming payload before destroying the old payload.

**Evidence:** C compatibility, active payload copy/destroy, invalid tags, over-alignment,
recursive representation, and stable generated C exports.

## Deferred, not forgotten

Class inheritance, general reflection/comptime, a package manager, generic value
parameters, a general allocator framework, and a high-level I/O layer remain future
work. The current gates do not require implementing those features first.
