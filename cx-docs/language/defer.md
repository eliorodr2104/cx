# Defer

`defer` always takes a block. It registers work for leaving the current scope.

```c
FILE* file = fopen(path, "rb");
if (file != NULL) {
    defer {
        fclose(file)
    }

    useFile(file)
}
```

## Registration and order

Registration happens only when execution reaches the statement. Multiple registrations
in a scope execute in reverse registration order. A defer in a loop body applies
to that iteration's scope, not one shared final cleanup.

```c
defer { logFirst() }
defer { logSecond() }
```

On leaving that scope, `logSecond()` precedes `logFirst()`.

## Exits

Normal fallthrough, `return`, loop exits that leave the scope, and Cx error propagation
must run relevant active cleanups exactly once. The return value/error must be
preserved before cleanup can invalidate its sources.

Managed-local destruction and defer ordering share a cleanup stack; do not adopt
an independent rule that destroys all locals before running a defer that uses them.
Jumping into a Cx cleanup scope and C `goto`/`setjmp`/`longjmp` interaction must be
specified in G07. Pure C control flow is not changed by adding Cx support.

## Failure inside cleanup

A deferred block must not introduce an unhandled replacement error during cleanup.
Potentially throwing operations need local handling under the initial conservative
contract. Non-local `return`/`break`/`continue` from the deferred body are not a
shortcut to bypass the enclosing cleanup protocol.

For manual RC, a defer is the normal way to balance an acquired +1 even when later
operations throw. Cx does not automatically release a manual reference merely
because it appears in a scope containing `try`.
