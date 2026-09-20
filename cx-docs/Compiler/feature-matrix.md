# Feature and Documentation Traceability

This table maps accepted design areas to implementation work. No row claims current
implementation status in the user's fork.

| Feature | Source contract | Main compiler responsibilities | Required interactions |
| --- | --- | --- | --- |
| `var` / `let` | language/variables.md | Parser/Sema, source types | Multiple declarators, typedefs, C decay, const |
| Labels / references | language/functions.md | Parser, lookup, callable type, mangling | C callbacks, complex declarators, defaults |
| Cx source model | language/source-model.md | Reopening, ownership/access, serialization | Separate TUs, no added fields, API mismatch |
| Properties/subscripts | language/properties.md, subscripts.md | Accessors, mutation paths, writeback | COW, compound ops, `~mutating`, errors |
| Operators | language/operators.md | Associated lookup and normal ranking | Single evaluation, builtin C semantics, set laws |
| Optional/null | language/nullability.md | Contextual literals, type sugar, extraction | Raw pointer null, String, weak load |
| Tuples/enums | language/tuples.md, enums.md | Types, patterns, value operations | C comma/C23 enums, active payload, niches |
| Protocols/generics | language/protocols.md, generics.md | Constraints, conformances, specialization | Defaults, retroactive coherence, generic witnesses |
| Classes/ARC | language/classes.md, memory.md | Init/lifetime, runtime calls | Weak race, MRC, raw resources, partial failure |
| Closures | language/closures.md | Capture typing, environment, adapters | Escaping, labels/effects, C ABI function pointer |
| Errors | language/errors.md | Effects, cleanup edges, C bridge | Throwing init/callable, `any Error`, ownership |
| Modules | language/modules.md | PP ownership, linkage, artifact builder | Foreign C headers, macro origin, cache keys |
| PointerTag | language/pointer-tagging.md | Protocol capability, target validation, masks | Provenance, aliases, atomics, C boundary |
| Library algorithms | stdlib/README.md | Literal/iterator hooks, specialization | Bounds, closure effects, allocation, COW |

Use this matrix with [OPEN-ISSUES](../OPEN-ISSUES.md), not as a substitute for detailed
feature tests. A per-feature ticket should identify the exact gate and documents it
updates so future revisions do not restore superseded syntax accidentally.
