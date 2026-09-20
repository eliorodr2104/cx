# Callable ABI

## Materialized representation

The baseline general Cx callable consists of an invocation entry and environment
reference. The environment's concrete generated type is known by its invocation
implementation even though the enclosing callable erases it.

```text
callable
    invoke entry
    environment reference
```

The conceptual invocation takes a hidden context before explicit arguments. Target
lowering may place hidden parameters differently as long as all callers agree.
Throwing invocation also follows the native error ABI; it does not require an extra
word in the stored callable solely because it throws.

## Uniformity

One callable type has one ABI at storage and external call boundaries. A noncapturing
value has an empty context in that representation; it does not change the variable's
size at runtime. Devirtualization/inlining can eliminate a known pair entirely, use
SSA values for captures, or reduce internal code to a direct function.

Inlining is an optimization, not compulsory language semantics. Runtime-selected
callbacks may need indirect calls; generated bodies can be too large or recursive.
Branch/basic-block lowering for fully known local closures is an optimization case,
not a replacement for first-class callable semantics.

## Environment ownership

Heap environments use compiler-generated managed storage with suitable destruction
of captures. Stack environments are possible only under a proven nonescaping/lifetime
contract. Copy/destroy code cannot blindly ARC-release a stack address.

The owned-vs-borrowed invocation/storage protocol must be chosen before this ABI is
implemented. Conservative heap materialization is correct where escape cannot be
proved; optimization can remove it later.

## Capture semantics

Value captures copy values, ARC references acquire required ownership, raw pointers
copy addresses without lifetime extension. Shared mutable capture boxes, weak capture
spelling, and manual-RC capture ownership remain G07. Thread transfer follows the
chosen ownership/concurrency contract rather than accidental pointer sharing.

## C ABI adaptation

A raw C callback cannot accept an extra hidden argument. Converting a known
noncapturing/non-throwing callable requires a real C-compatible entry or adapter.
Casting `R (*)(Context*, Args...)` to `R (*)(Args...)` is not a valid conversion.

An arbitrary value of an erased callable type is not statically known to be
noncapturing. The compiler may accept conversion only with adequate evidence or a
future explicit checked contract; it must not discard state. Capturing callbacks
can use external C APIs that separately accept a context pointer, with an explicit
lifetime bridge.

## Measurement

B02 compares materialized, known, nonescaping, escaping, and runtime-selected calls.
Count capture storage and ownership operations, not only the two-word callable header.
