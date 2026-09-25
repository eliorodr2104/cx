# Overload Resolution

## Candidate selection

Lookup first finds declarations visible through normal scope/member/module rules.
Cx does not add C++ argument-dependent lookup or user-defined implicit conversion
chains.

Resolution proceeds by label/arity applicability, type compatibility, and ranking:

1. Exact type match.
2. Applicable C promotion.
3. Applicable standard C conversion.

This is a per-argument comparison, not a summed numeric score. If neither candidate
is uniformly preferable, diagnose ambiguity rather than choose by source order.
Ordinary non-overloaded C calls preserve C conversions.

```c
void show(int value)
void show(double value)

show(10)     // int
show(10.0)   // double
```

For a `short` argument, `int` promotion is preferred to conversion to `long` when
that promotion is the target's C rule.

## Compound references

```c
void move(int x value)
void move(float x value)
void move(int y value)

void (*callback)(int) = &move(x:)
```

The compound name filters to the `x:` declarations. The pointer type then selects
the `int` overload. Without sufficient context, `&move(x:)` is ambiguous.
Function-address selection must match an ABI-compatible function signature;
ordinary argument conversions do not license a mismatched function-pointer cast.

## Generics

A concrete exact candidate is preferred to an otherwise equivalent generic one.
Among equally applicable generics, a provably more constrained candidate may win.
An exact generic candidate must not lose merely because a concrete candidate exists
that needs a worse conversion. The detailed partial-order algorithm is a semantic
implementation gate, with explicit ambiguity tests.

## Callable labels and effects

Cx callable types may retain labels and `throw(E)`. A matching labeled callable can
be projected to an unlabeled interface under the accepted label-erasure direction.
Implicit renaming from `value:` to `other:` is not permitted.

C function pointers retain no Cx argument-label interface. A throwing entry cannot
be used as a non-throwing C function pointer by ignoring its error channel.

## Diagnostics

Explain which label, type, effect, or constraint made the relevant candidates
inapplicable. Show source declarations and a bounded set of useful alternatives.
No selection depends on link order, declaration order, or an arbitrary "closest"
score. See [Diagnostics](../compiler/diagnostics.md).
