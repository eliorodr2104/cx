# Initialization and Cleanup

## Automatic vs explicit initialization

Automatic field defaults/memberwise construction use declaration order. Explicit
custom initializer statements retain their written execution order; source effects
must not be reordered to match field layout.

Track not-started, initialized, and fully constructed states. Reject reads of fields
without a value and escape/use of an incompletely initialized self. The rules,
including delegation, are in [Definite initialization](../language/initializers.md#definite-initialization);
defaults read earlier fields as described in
[Initialization order](../language/initializers.md#initialization-order-do-not-reorder-the-program).

## Failure

A failed initializer cleans only successfully initialized fields in reverse relevant
initialization order and releases allocated storage. It does not run normal user
`deinit` for a value that never completed construction. If a default/capture allocation
can fail, the chosen OOM/error policy must make that failure path well-defined.

## Normal destruction

For a fully initialized value/object: invoke a custom non-throwing deinit if present,
then automatic member destruction in reverse declaration order, then storage release
where owned. User deinit does not replace managed field cleanup.

Raw C pointer members are not implicitly freed. A raw resource-owning copyable struct
needs an explicit copy/ownership contract; otherwise two value copies can double-close
or double-free the same handle.

## Scope cleanup stack

Maintain one ordered cleanup model for managed locals, active defers, temporary
values, return/error transfer, and loop exits. A deferred body may access earlier
still-live locals, so a blanket "destroy all locals, then defer" rule is wrong.

Prepare/preserve an outgoing value or error before cleanup that can invalidate its
sources. Execute only registered/reached cleanups, exactly once. Re-entrant cleanup
and errors handled locally inside defer/deinit must not skip remaining work.

## Non-local control transfer

C `goto`, `setjmp`, `longjmp`, foreign exceptions, process termination, and asynchronous
signals are not automatically equivalent to Cx error propagation. Define restrictions
or supported cleanup behavior for Cx-owned scopes, without changing unmodified C.
The policy is G07; do not promise unconditional cleanup for every machine-level exit.
