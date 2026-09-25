# Semantic Analysis

Sema validates meaning and builds the semantic AST during parser interaction. It is
not a second independent Cx inference/lookup engine.

## Lookup and identity

Preserve C scopes, tags, typedef lookup, and macro-derived identifiers. Cx adds owner
modules, associated methods, protocols, continuation matching, and compound names.
Local bindings/parameters shadow implicit member lookup; `self.field` remains explicit.
The detailed ordering against enclosing/global declarations must be tested rather
than inherited from a casual prose example.

Labels filter candidates before conversion ranking. Use exact match, promotion, then
standard C conversions; compare candidates consistently per argument. Generics add
provable constraint specificity without making a converting concrete candidate beat
an exact generic unconditionally. Return type or throwing effect alone is not a new
overload.

## Inference and optionality

Deduce each `var` declarator independently. Apply `let` top-level const after value-type
deduction. Preserve pointee const and Cx labels/effects. Do not accidentally preserve
an initializer's top-level const as immutable `var` storage.

Raw pointers remain nullable C types. `null` and optional `nil` have separate conversion
rules. Class/String optionality is desired; fully generalized `T?` is G03. Optional
extraction must be checked or explicitly specified; it is not an implicit cast.

## Access and mutation

Resolve public/internal/private access against the owning type/module. Enforce setter
restrictions on mutation paths, not just assignment tokens. `~mutating` constrains
value receivers shallowly and is part of declarations; a lint does not change the
contract based on today's body.

Continuation blocks implement known interfaces, default new helpers to private, and
cannot add storage. Unknown explicit public/internal bodies are diagnostics with
suggested source edits, not commands to rewrite headers.

## Initialization and cleanup

Synthesize memberwise/default construction only when no user init is declared. Track
field states, defaults, successful construction, and failure cleanup. Execute custom
body statements in source order; never reorder side effects to enforce a field-order
slogan. Suppress full-object deinit on partial construction.

## Properties, subscripts, and operators

Record whether an access is a stored lvalue, getter value, setter path, or an explicit
writeback. Compound updates evaluate the base/index once. Enforce COW uniqueness and
receiver restrictions where required; do not mutate a discarded getter copy.

## Protocols and generics

Check explicit conformance against requirements: names, labels, types, associated
types, effects, mutability, and access. Prefer a matching concrete witness to a default.
Check overlapping defaults/conformances, including imported modules.

Generic bodies are checked under declared constraints; concrete representation checks
may remain deferred. Erased `any` values permit only operations supported by retained
type information. Generic/static/Self requirements require their own invocation ABI,
not an untyped call to an arbitrary witness address.

## Effects and C exports

`try` yields the success type and is required even within do/catch. Unhandled effects
must fit an enclosing handler or function declaration. Handlers are ordered and
unreachable catches are diagnosed. Deinit cannot propagate.

For `extern(C)` throwing exports, Sema validates representability/ownership and records
an adapter request. It does not pretend the native entry already has C calling rules.

## PointerTag

Validate tag capacity, concrete alignment, supported target/address space, unique
schema, and operation context. Capacity is not proof that every arbitrary raw pointer
is aligned/live. Cross-boundary rules are G05.
