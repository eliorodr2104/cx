# Milestone Records

One record per implemented roadmap milestone, in the format required by the
[roadmap](../implementation-roadmap.md): source contract; permitted baseline C modes;
AST/Sema changes; target/runtime requirements; positive/negative tests; C collisions;
known limitations; benchmark checkpoint if relevant; and a runnable demonstration.

A milestone without a record here is not started. A lettered suffix (M3a) is
one slice of a roadmap row that is being implemented in stages; a dotted number
(M1.1) is an extra milestone inserted between two rows. Hardening passes
(M4.1 onward) fix gaps and bugs found in completed features rather than adding new
ones.

## Features

- [M0: Pinned baseline, driver mode, `-x cx`, language options](M0.md): **Completed**
- [M1: `var`, `let`, contextual `null`](M1.md): **Completed**
- [M1.1: Separate `auto`, `__auto_type`, `var` and `let` in the AST](M1.1.md): **Completed**
- [M2: Module ownership and source identity skeleton](M2.md): **Completed**
- [M3a: Cx linkage and experimental mangling](M3a.md): **Completed**
- [M3b: Argument labels](M3b.md): **Completed**
- [M3c: Overload lookup and compound references](M3c.md): **Completed**
- [M4a: Struct methods, `self` and `~mutating`](M4a.md): **Completed**
- [M4b: Continuations](M4b.md): **Completed**
- [M4c: Access control](M4c.md): **Completed**
- [M4d: Generated memberwise construction](M4d.md): **Completed**
- [M4e: Declaration-site field defaults](M4e.md): **Completed**
- [M4f: Custom initializers](M4f.md): **Completed**
- [M5a: Implicit tag names](M5a.md): **Completed**
- [M5b: Optional semicolons](M5b.md): **Completed**
- [M6a: Tuples](M6a.md): **Completed**
- [M6b: Raw and simple enums](M6b.md): **Completed**
- [M6c: Payload enums](M6c.md): **Completed**
- [M6d: Pattern matching](M6d.md): **Completed**

## Hardening passes

- [M4.1: Close the access and method-call holes](M4.1.md): **Completed**
- [M4.2: Close the M2–M4 leftovers](M4.2.md): **Completed**
- [M4.3: Cx in code completion, and two method bugs](M4.3.md): **Completed**
- [M4.4: No crash on M0–M4 input; methods look up their receiver first](M4.4.md): **Completed**
- [M4.5: Access control, field defaults and construction agree with C initialization](M4.5.md): **Completed**
- [M4.6: Linkage and mangling that link, and don't collide](M4.6.md): **Completed**
- [M4.7: Tooling and the preprocessor keep Cx meaning](M4.7.md): **Completed**
