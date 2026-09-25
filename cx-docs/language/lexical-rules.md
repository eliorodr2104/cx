# Lexical and Statement Rules

Cx keeps the selected C dialect's preprocessing and lexical behavior. Its extensions
must preserve valid C before introducing an alternate interpretation.

## Contextual words

`var`, `let`, `class`, `protocol`, `extension`, `self`, `Self`, `init`, `deinit`,
`null`, `nil`, `try`, `throw`, `catch`, `in`, `where`, `type`, `any`, access modifiers,
`operator`, `subscript`, and `mutating` are interpreted in their Cx contexts.
Their recognition must not invalidate ordinary C identifiers or macros.
Existing C keywords, such as `do` and `static`, retain their C meanings outside
explicit Cx productions.

## Optional semicolons

Both forms are intended:

```c
var count = 0;
var limit = 10
```

A line ending is a candidate boundary, not an unconditional terminator.
Continuation takes precedence when required to preserve C:

```c
foo
(bar);

return
    compute();
```

The first remains one call and the second returns `compute()`'s result. Where C
cannot validly continue, the line break ends the statement: after a value that
cannot be called or indexed, a line beginning with `(` or `[` starts a new
statement, and so does a line beginning with `*`, `&`, `-` or `+` that assigns,
such as `*p = 3`, or with `++`/`--` before an operand. C `for`
headers keep explicit semicolon separators. An explicit semicolon is the escape
hatch when the programmer wants a boundary that continuation rules would not infer.

A complete grammar for macro-origin tokens, `do/while`, labels, trailing closures,
and preprocessing round trips is a milestone gate, not an already proved property.

## Declaration and call labels

```c
void resize(
    int width  newWidth,
    int height newHeight
)

resize(
    width:  800,
    height: 600
)
```

The declaration is type-first; the call uses colons. A single parameter name does
not implicitly require a call label. Cx callable *type* syntax can separately use
`int (value: int)`; it is not the syntax for an ordinary function declaration.

## Positional notation and ranges

Tuple access uses `value.$0`; closure shorthand uses `$0`. Both require contextual
recognition, including when a selected compiler extension permits `$` in identifiers.

The range spellings are ASCII `...` and `..<`, not the Unicode ellipsis. The compact
forms `0...5` and `0..<5` are intended, but preserving C preprocessing-number and
macro-stringification behavior needs a specified token strategy.

## Module directive and formatting

`#module Name` is a directive, not a macro or a file-detection signature. See
[Modules](modules.md). No automatic source rewriting occurs during compilation.

Use braces on the same line and align related multiline parameters/arguments.
Whitespace alignment is stylistic. It does not introduce declarations or labels.
