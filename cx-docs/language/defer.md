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
Pure C control flow is not changed by adding Cx support.

## Jumps

- A `goto` or `break` that leaves the scope runs its registered defers, like any
  other exit.
- A `goto` or `case` label may not jump into a deferred block, nor forward past
  a `defer` into the rest of its scope, where the defer would be active without
  having been registered.
- A clause of a Cx `switch` is its own scope, so a defer in one clause runs
  before the clause ends and never blocks the next `case`. In a C `switch`, a
  later `case` of the same block skips the defer and is an error.

## Non-local exits

`setjmp`, `longjmp` and their variants may not be called inside a deferred block.
A `longjmp` out of a scope with active defers does not run them, and neither do
`exit`, `abort` or a signal: these exits leave the program's scopes the way C
leaves them. Code that needs cleanup across them must not rely on `defer`.

## Spelling

`defer` is a keyword only at the start of a statement and before `{`. Elsewhere it
stays an ordinary name, so a C variable or function called `defer` keeps working.
`defer` followed by anything but a block is an error.

## Failure inside cleanup

A deferred block must not introduce an unhandled replacement error during cleanup.
Potentially throwing operations need local handling under the initial conservative
contract. Non-local `return`/`break`/`continue` from the deferred body are not a
shortcut to bypass the enclosing cleanup protocol.

For manual RC, a defer is the normal way to balance an acquired +1 even when later
operations throw. Cx does not automatically release a manual reference merely
because it appears in a scope containing `try`.
