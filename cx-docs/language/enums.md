# Enums

Cx preserves C enums and adds three kinds of Cx enum: raw and simple enums with scoped
cases, payload enums, and option sets. A Cx enum is selected by an explicit spelling,
never by a syntax heuristic.

## C enums and Cx enums

```c
enum Color {
    RED,
    GREEN,
    BLUE
};

enum Code : unsigned char { OK = 0 };
```

An enum whose body does not use `case` is a C enum. It keeps C enumerator lookup,
representation, and conversions, including the C23 fixed-underlying-type form above.

An enum whose body uses `case` is a Cx enum. `case` cannot appear in a C enum body, so
the spelling changes no valid C program. Mixing `case` clauses with C enumerators in one
body is an error.

```c
enum Direction { case north, south, east, west }

enum Status: uint16_t {
    case ok = 200, created
    case notFound = 404
}
```

A body holds one or more `case` clauses. A clause lists case names separated by commas,
and clauses are separated by a line break or `;`.

## Raw and simple enums

A Cx enum with an integer type after `:` is a raw enum. Its values follow C's rules: the
first case is 0 unless given, and each later case is one more than the previous. Every
value must fit the backing type, and two cases may not share a value.

A Cx enum without a backing type is a simple enum. Its cases take no `= value`, it has no
`rawValue`, and the compiler stores it in the smallest unsigned integer that holds every
case.

Cases are scoped. They are written `Status.ok`, or `.ok` where the expected type is known:
an initializer, an assignment, a `return`, an argument, the right operand of `==` or `!=`,
both branches of a `?:` whose value is expected, an element of a braced array of the enum,
and a `case` label of a Cx switch. A struct field in braces and a tuple literal element
follow in M6.1; until then they take `Type.case`. A bare `ok` is an error.

A raw or simple enum does not convert implicitly to or from an integer or another enum,
and C casts in either direction are errors. `s.rawValue` yields a raw enum's value as its
backing type. Checked construction from an integer arrives with Optional. Only `==` and
`!=` apply; ordering, arithmetic, increments, and bitwise operators do not.

A raw enum has the size and ABI of its backing type. Cases are constants and have no
symbols.

## Payload enums

```c
enum Token {
    case integer(int)
    case location(int line, int column)
    case end
}
```

A Cx enum with at least one parenthesized case is a payload enum. A payload is written as
a tuple type. A single-element payload is that element's type, since a one-element tuple
does not exist. A payload enum cannot have a backing type and has no `rawValue`.

```c
Token a = .location(10, 4)
Token b = Token.location(line: 10, column: 4)
Token c = .end
```

Construction follows tuple literal rules: labels are optional, but labels that are written
must match the payload's, and each element converts as initialization would. A case
without a payload is written without parentheses.

A payload enum is stored as a struct holding a tag, which is a simple enum with one case
per case, followed by a union of the payloads. Only one payload is alive at a time. Its
layout, alignment, and ABI are those of that struct. The tag and the union are not
accessible from source; a payload is read by matching. A zero-initialized payload enum
holds its first case with a zeroed payload.

Payloads may hold any tuple element type whose copy is a plain copy. A payload that
contains its own enum by value requires indirection and is not yet supported; a pointer to
it is allowed. Payload enums compare through matching until protocol-based equality
exists.

Generic payload enums are planned with generics:

```c
enum Either<L, R> {
    case left(L)
    case right(R)
}
```

## Pattern matching

```c
switch (token) {
    case .integer(value):
        consumeInteger(value)
    case .location(line, _):
        consumeLine(line)
    case .end:
        finish()
}
```

A switch whose condition has a Cx enum type is a Cx switch. Every other switch keeps C's
rules.

A label names a case as `.name` or `Type.name`. For a payload case it may bind the
payload elements by position, with a new name or `_` for each element, or omit the list
to ignore the payload. Bindings are `const` copies scoped to their case body.
`case .a, .b:` matches several cases when none of them binds.

Each case body ends at the next label; there is no fallthrough, and `break` still leaves
the switch. Each body is its own scope. An empty case followed by another label is an
error that suggests combining them. Labels must be at the top level of the switch body,
and C integer labels cannot mix with Cx case labels.

A Cx switch must cover every case or include `default`. Missing and duplicate cases are
errors, and a `default` that no value can reach is a warning. A value that matches no
case at run time, which only memory reinterpretation can produce, stops the program
instead of continuing with undefined behavior.

Patterns with values, nested patterns, guards, and matching outside a switch are future
work.

## Option sets and capabilities

```c
enum Permission: OptionSet {
    case read, write, execute
}

enum ParseError: Error {
    case malformed
    case unexpectedToken(int)
}
```

`OptionSet` makes each case an independent bit, as specified in
[Option Sets](optionsets.md). Because the enum is already a Cx enum, `OptionSet` after `:`
cannot be read as a C23 backing type. Conformance to protocols such as `Error`, methods,
and extensions on enums arrive with protocols.

## Representation summary

| Kind | Storage |
| --- | --- |
| C enum | C layout and conversions |
| Raw enum | Its backing type |
| Simple enum | Smallest unsigned integer that holds every case |
| Payload enum | Struct of tag and union of payload tuples |
| Option set | Smallest unsigned integer with one bit per case |

Payload representation may later use validity niches only when they are impossible for a
live payload. A raw nullable pointer's null value is not an unused niche.
