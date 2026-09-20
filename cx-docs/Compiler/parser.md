# Parser Strategy and Compatibility Gates

## Rule

Preserve a valid selected-C interpretation. Recognize Cx locally with contextual
lookup and bounded lookahead where possible. Do not repeatedly parse whole translation
units as C, fail, rewind, and parse them again as Cx.

This policy is not itself a complete grammar. The conflicts below must have concrete
rules/tests before implementation.

## Additive declarations

`var`/`let` keep complete C declarators and parse independent initializers. A labeled
parameter has a second name after the complete C declarator. Compound names such as
`move(x:)` are references, while `move(x: value)` has argument expressions.

Continuation blocks are recognized against an already known complete type in the
appropriate scope/owner context. Their parsing must not treat a normal C tag shadowing
in a nested scope as accidental reopening. Access/effect modifiers remain contextual.

## Semicolon elision

Use syntactic completeness, continuation validity, and source boundary information.
Preserve `foo\n(bar);`, multiline arithmetic, C do/while, labels, and declarations
with multiple declarators. Elision must not insert separators in C for headers.

Macro-generated tokens, end-of-file, closing braces, trailing closures, and preprocessed
input all need an explicit newline/boundary policy. Errors should suggest a semicolon
when that is the clear escape hatch, not reinterpret valid C.

## Tuple/comma collision

`(a, b)` is a C comma expression. Tuple interpretation can be introduced by unambiguous
Cx syntax, a tuple-expected context, or a specifically defined new declaration context.
It cannot replace scalar-context legacy comma expressions globally. Labeled tuples
provide an unambiguous form, but the unlabeled inference rule remains G01.

## C23 raw enums

A fixed underlying type after `:` is already a C23 facility. It cannot alone select
scoped/no-implicit-conversion Cx semantics. Payload cases or protocol conformance are
Cx additions; modern raw enum opt-in still needs a rule.

## Numeric tokens and ranges

The compact `0...5` can arrive as a preprocessing number, not separate integer and
range tokens. The preprocessor has token-pasting/stringification obligations. Do not
solve compact ranges by blindly splitting every numeric-looking token and changing
macro behavior. Specify Cx-aware parser/token normalization or another conservative
solution and test round trips.

## Trailing closures and generic references

A call followed by a block can already be C in some contexts. Callable expectation
and Cx-specific syntax must guide trailing-closure parsing without swallowing a
legacy block. Likewise, `f < x > (y)` is not automatically a generic application.
Consult declaration context and preserve C relational expressions.

## Source ownership and diagnostics

Preserve spelling and expansion locations independently. Access/module ownership must
not be inferred solely from whichever physical spelling token the lexer happened to
return for a macro. Avoid cascading parse errors when the actual problem is a label,
unmatched continuation, or ownership declaration.

The exact grammar is a feature gate, not a claim that "contextual keywords solve all
compatibility." See [G01](../OPEN-ISSUES.md#g01--c-grammar-collisions).
