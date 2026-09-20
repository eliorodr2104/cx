# Error Handling

Cx has a dedicated error channel. The ordinary function return type describes
success, not a `Result<T, E>` wrapper.

## Error protocol and declarations

```c
protocol Error {
}

enum ParseError: Error {
    invalidInput,
    unexpectedToken(int)
}

int parse(String text input) throw(ParseError)
int loadNumber() throw
int constantNumber()
```

`throw(ParseError)` restricts escaping errors to that type. Plain `throw` permits
any conforming error, conceptually an `any Error` channel. A function without an
effect declaration cannot let a Cx error escape.

## Throw and try

```c
throw ParseError.invalidInput

var number = try parse(text: input)
```

`try` yields the success value, here `int`. It does not yield an `Error` and does
not implicitly construct a source-level `Result`.

Every potentially throwing call needs the appropriate `try` acknowledgment,
including calls inside `do/catch`. Being inside a handler does not remove `try`.
Only Error-conforming values can be thrown.

## Local handling

```c
void showNumber(String text input) {
    do {
        int number = try parse(text: input)
        printf("%d\n", number)
    } catch (.invalidInput) {
        printf("Invalid input\n")
    } catch (.unexpectedToken(token)) {
        printf("Unexpected token: %d\n", token)
    }
}
```

Typed handlers use `catch (ParseError error)`. `catch (error)` handles the remaining
error value; `catch` handles without binding. Handlers are considered in source
order. An earlier general handler must not be silently reordered behind a specific
one; unreachable-handler diagnostics should explain the problem.

A handler chain must handle every escaping error in a non-throwing function.
Unmatched errors may continue to an enclosing handler or the declared error channel.
An error thrown by a handler goes to its outer context, not the handler's siblings.

## Propagation and conversion

```c
int readNumber(String text input) throw(ParseError) {
    return try parse(text: input)
}
```

Typed-to-general propagation is allowed and erases the error only where needed.
Unrelated typed errors require explicit handling/wrapping; there is no arbitrary
user-defined implicit error conversion. A `void` success type is also valid.

## Cleanup and construction

Leaving scopes on an error runs the same registered cleanup machinery used by
other exits. Return/error payloads are preserved while cleanup runs. ARC ownership
is balanced; manual-RC ownership still requires explicit balancing, typically defer.

Throwing initializers clean partially constructed fields but do not invoke the
normal user destructor for a never-completed object. `deinit` cannot propagate.

## Callable effects

```c
int (text: String) throw(ParseError) parser
int (String) throw operation
```

Labels and error effects are part of the Cx callable contract. A throwing callable
cannot silently become a non-throwing C callback.

## C exports

The accepted design permits `extern(C)` on throwing definitions when the compiler
can generate an unambiguous C-compatible bridge. Cx calls the native throwing entry;
C callers use the generated status/out-parameter entry and generated C header.
The source header is never modified automatically. Every exposed type and ownership
transition must be representable or explicitly adapted.
See [Error ABI and C Export](../abi/errors-and-c-export.md).

## Representation and status

Explicit error control flow is the baseline. Aggregate return, hidden error output,
and target-aware split return are implementation candidates, measured at the error
milestone rather than decided by documentation. No claim of zero cost or always-cold
errors is made for every workload.

`Result<T, E>` remains an ordinary library value for stored/composed outcomes.
`try?`, force-try, multiple typed error lists, and rethrowing-effect inference are
not implicitly accepted by this document.
