# Operator Overloading

Cx operator implementations are explicit associated operations, preserving
return-type-first declarations. Binary operators do not insert function pointers
into instances.

```c
struct Vec2 {
    float x
    float y

    static Vec2 operator +(
        Vec2 lhs,
        Vec2 rhs
    )
}

struct Vec2 {
    static Vec2 operator +(
        Vec2 lhs,
        Vec2 rhs
    ) {
        return Vec2(
            x: lhs.x + rhs.x,
            y: lhs.y + rhs.y
        )
    }
}
```

## Supported direction

Arithmetic, bitwise, shift, equality, and ordering operators are candidates for
explicit overloads. Unary `+`, `-`, `!`, and `~` have arity-specific declarations.
Protocol requirements may declare operators using `Self` operands.

Assignment, member access, `sizeof`, the comma operator, conditional `?:`, and
short-circuit `&&`/`||` retain language semantics. Custom operator tokens/precedence
and increment/decrement customization are not part of the accepted initial surface.
C address-of/dereference are not silently overloaded as generic unary operators.

## Lookup

Use the same type/conversion/constraint ranking as ordinary Cx functions. Associated
operator declarations are found through operand types and visible conformances;
this is a narrowly defined operator rule, not C++ ADL over arbitrary free functions.
Builtin C operations remain unchanged when no explicit Cx operation is involved.

An overload should involve an associated Cx type; an unrelated extension cannot
silently redefine `int + int` for legacy C expressions.

## Compound assignment

The initial design derives `a += b` from `+` and assignment, but must evaluate the
left-hand storage expression exactly once:

```c
items[nextIndex()] += amount
```

must not call `nextIndex()` twice. Property/subscript targets need one location or
well-defined getter/setter writeback sequence. Captured aliases and failure during
the operation require the same assignment/ownership rules as other mutation.

## Synthesized operators

OptionSet and Set can supply bit/set algebra. Derived `!=` from `==` is reasonable;
ordering derivation needs explicit law requirements. Floating-point NaN/partial
orders must not be made incorrect by replacing `<=` with `!(>)` indiscriminately.

See [Option Sets](optionsets.md) and [Library Protocols](../stdlib/protocols.md).
