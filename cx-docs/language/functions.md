# Functions and Argument Labels

## Ordinary declarations

Cx preserves C's return-type-first functions and type-first parameters:

```c
int add(
    int a,
    int b
) {
    return a + b
}

var total = add(10, 20)
```

No `func`, `fn`, or return-type arrow is introduced for these declarations.

## Explicit labels

A labeled parameter extends a complete C parameter declarator with a local name:

```c
void resize(
    int width  newWidth,
    int height newHeight
) {
    applySize(newWidth, newHeight)
}

resize(
    width:  800,
    height: 600
)
```

The declarator name supplies the external label; the extra identifier supplies
the name visible inside the function. For simple parameters this reads as
`Type externalLabel localName`.

Ordinary one-name parameters stay positional:

```c
void resize(int width, int height)
resize(800, 600)
```

Labels are optional *when declaring an API*. When a parameter explicitly has a
label, its direct calls require that label. Omitting a label in the declaration
does not silently create one at the call site.

There is no requirement to repeat a name as `int x x`. The exact same-name
shorthand/optional-call-label policy remains [G02](../OPEN-ISSUES.md#g02--labels-and-synthesized-construction).
Do not implement a second interpretation by accident.

## Complex C declarators

The extension attaches after the complete declarator, not after a guessed type:

```c
void install(
    void (*callback)(int) handler
)
```

Here `callback` is the external label and `handler` is local. Attributes, arrays,
and nested declarators require parser regression tests.

## Defaults and identity

Default argument values belong to the visible declaration. Implementations do not
repeat or change them. The final evaluation and redeclaration rules must be fixed
before default arguments are enabled across translation units.

Overload lookup uses name, labels, arity, types, and applicable constraints. Local
parameter names do not distinguish overloads. Return type alone or `throw` alone
does not introduce another overload.

## Function references

```c
void (*callback)(int) = &move(x:)
```

`move(x:)` is a compound-name reference, not a call. `move(x: 10)` is a call.
Unlabeled positions use `_`, for example `move(_:mode:)`.

## Effects and first-class values

```c
int parse(String text input) throw(ParseError)
int (text: String) throw(ParseError) parser
```

The first is a function declaration. The second declares a Cx callable variable.
See [Overloads](overloads.md), [Closures](closures.md), and [Errors](errors.md).

## Separate implementation

Interface declarations and definitions must agree on their source-level contract.
The local names may differ; labels, types, access, effects, and receiver requirements
must match as specified in [Source Model](source-model.md).
