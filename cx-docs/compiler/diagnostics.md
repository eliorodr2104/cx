# Diagnostics

Cx errors should explain the violated contract, show the smallest useful source range,
point to relevant declarations, and suggest valid remedies. The goal is explanatory
rustc-style usefulness using Clang's structured diagnostic infrastructure.

## Structure

```text
error: no overload of 'resize' accepts argument label 'height'
note: the visible declaration is 'resize(width:)'
help: check the intended parameter label or call a different overload
```

Do not replace useful Cx meaning with a generic parser complaint. Conversely, preserve
precise existing Clang diagnostics when they already describe the issue correctly.

## Fix safety

A syntactically valid edit is not necessarily the developer's intent. Changing a public
field/type to nullable or adding a missing public method alters API contracts. Offer
such edits as explicit user-selected actions, not automatically applied "safe fixes".

Compiler invocation never rewrites user headers. A public implementation missing a
declaration produces an error, related interface location, and a proposed declaration
edit. Macros/multiple possible headers may prevent a reliable automatic edit.

## Important diagnostic families

- Wrong/missing argument labels and ambiguous compound references.
- Missing requirements, wrong associated types, and conflicting conformances.
- Internal/private access and forbidden setter/address paths.
- Mismatched interface/implementation, new stored fields in continuations.
- Optional/raw-pointer absence confusion and invalid extraction.
- Escaping errors, unreachable catches, partial initialization, and cleanup misuse.
- Capturing callable to C callback conversion and hidden-context ABI mismatch.
- PointerTag capacity, unsupported targets, and foreign-boundary misuse.
- Missing/stale/incompatible generic/module artifacts and bridge type mappings.

Explain relevant candidates and rejection reasons rather than dump every declaration.
Generated entities should point back to the fields/requirements that caused synthesis.

## Stable structure, evolving presentation

Use semantic IDs independent of message text and of Clang's internal numeric IDs.
A public Cx code namespace is planned but individual numbers are not fabricated here.
Export severity, locations, ranges, related notes, and candidate/fix information to
editor tooling. Terminal rendering remains separate from Sema.

Style lints, including a suggestion to add `~mutating`, should be opt-in or carefully
classified so new warnings do not break legacy C `-Werror` builds.

## Tests

Test diagnostics independently from program rejection. Check meaningful ranges,
related declarations, recovery, and fix applicability, including negative macro cases.
Do not freeze terminal color/spacing in every semantic test.
