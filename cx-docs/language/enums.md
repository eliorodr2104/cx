# Enums

Cx preserves C enums and adds payload enums, protocol conformance, and option-set
capabilities. These categories must not be selected by an unreliable syntax heuristic.

## Existing C enums

```c
enum Color {
    RED,
    GREEN,
    BLUE
};
```

Keep C enumerator lookup, representation, and conversions for this declaration.
C23 also supports fixed-underlying-type declarations, for example
`enum Code : unsigned char { OK = 0 };`. A colon and integer base alone cannot mean
"scoped Cx enum with no implicit C conversions" without breaking the superset goal.

A distinct modern raw-enum contract is desired, but the unambiguous opt-in spelling
is G01. Do not silently choose `enum class` or reinterpret a valid C23 declaration.
See [C compatibility references](../SOURCES.md).

## Payload enums

```c
enum Token {
    integer(int),
    identifier(String),
    location(int, int),
    end
}
```

```c
Token token = .location(10, 4)
```

Each case constructs an enum value with only that case's payload alive. Payload
parameters can be positional. The exact relation between named payload elements
and function argument-label syntax remains G02; examples do not impose duplicated
names to obtain a labeled constructor.

Generic payload enums are allowed:

```c
enum Either<L, R> {
    left(L),
    right(R)
}
```

## Pattern matching

```c
switch (token) {
    case .integer(value):
        consumeInteger(value)
    case .identifier(name):
        consumeName(name)
    case .location(line, column):
        consumeLocation(line, column)
    case .end:
        finish()
}
```

A closed modern enum switch is exhaustive, or includes `default`. The precise
modern-case fallthrough policy must be specified before lowering; ordinary C
switches retain C fallthrough. New enum values do not expose arbitrary discriminant
integers through an automatic payload `rawValue` API.

## Capabilities and methods

```c
enum ParseError: Error {
    malformed,
    unexpectedToken(int)
}

enum Permission: OptionSet {
    read,
    write,
    execute
}
```

Enums can participate in methods, extensions, and conformance. They use value
semantics and synthesized active-payload copy/destruction. OptionSet cases instead
represent independent bits, as specified in [Option Sets](optionsets.md).

## Representation

Raw C enums retain C layout. Modern raw enums, once disambiguated, use their declared
backing integer. Payload enums have tagged-union semantics; representation may use
validity niches only when they are actually impossible for a live payload.
A raw nullable pointer's null value is not an unused niche.
