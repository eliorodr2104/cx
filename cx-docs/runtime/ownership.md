# Ownership Runtime

## State and operations

The runtime supports class/environment allocation, strong retain/release, destruction,
and weak storage/promotion. Compiler-generated value operations handle aggregates
containing managed fields.

The semantic lifecycle is construction, live ownership, transition to deinitializing,
field cleanup, and deallocation. Refcount zero is not a state from which weak loads
may resurrect a usable object. Overflow, invalid retain/release, and resurrection
policy must be specified and tested.

## Copies and assignment

ARC class copies share an object with appropriate ownership. Struct/tuple/enum copies
copy their values; ARC subobjects still share referenced objects. Out-of-line boxes
used to represent values need eager copy or correct COW, not arbitrary aliasing.

Assignment must handle self-assignment and aliases: preserve/acquire the incoming
value before releasing a source it may depend on. Constructors and failure paths
track exactly which owned fields exist.

## Manual RC

Manual-RC references start with an owned +1 from construction, increment with retain,
and relinquish ownership with release. A plain alias assignment does not retain.
Declaration syntax, return ownership annotations, capture policy, and mixed ARC/MRC
field behavior are required semantic decisions before this feature is enabled.

Do not force destruction while other owners remain. Do not infer ownership from a
function's name or hidden body unless the language explicitly chooses and documents
that convention.

## Synchronization

The thread-sharing contract comes before microbenchmarks. If ownership can cross
threads, concurrent retain/release/weak promotion need a correct synchronization
protocol. Non-atomic fast paths require a provable confinement or equivalent condition.
Data-race protection for object fields is separate from refcount correctness.

The proposal to defer all "atomicity" to benchmarking was too broad: representation
performance is measured, race semantics must be defined first.

## Runtime boundaries

Use target/allocator-aware allocation and destruction. OOM policy, failure of context
allocation, interaction with freestanding builds, and runtime availability are G07.
Pure C and trivial Cx code should not link an unused object/Unicode runtime.

Runtime entry names/layouts remain versioned implementation choices. Reusing concepts
from Objective-C ARC does not establish binary compatibility with its runtime.
