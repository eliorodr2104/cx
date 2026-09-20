# Modules and Header Ownership

`#module` adds optional ownership to the existing C source/header model. It does not
replace `#include` and does not add `import`.

```c
#module Geometry
#include "rect.h"
```

A physical source/header declares at most one owner module. The directive precedes
ordinary declarations; standard header guards/comments may surround it. Repeated or
conflicting ownership and conditional module identities require deterministic
validation.

## Ownership is per file, not a leaking preprocessor state

```c
// app.c
#module Application
#include <Geometry/rect.h>
```

If `rect.h` declares `#module Geometry`, its declarations belong to Geometry. After
the include, declarations in `app.c` still belong to Application. Plain external C
headers without Cx ownership stay C headers; they do not acquire Application linkage.

Declarations constructed by macros need an explicit expansion-site ownership policy.
`#line` changes diagnostics locations, not module ownership. See G09.

## Build-supplied identity

The build can supply the primary source's module and explicit header ownership
mappings. The flag spelling is not frozen and must not accidentally reuse an existing
Clang-module flag with different semantics.

A root file's explicit module must agree with its build assignment. An included
foreign header is allowed to belong to another module; that is not a conflicting
root assignment. Unowned includes must not inherit the root assignment blindly.

## What modules do

Modules establish internal access, owner-extension privileges, semantic identities,
Cx linkage defaults, and compiler artifact association. Module identity is stable
across paths/build directories and participates in Cx mangling.

Normal name lookup still needs declarations. Modules are not automatically a source
namespace syntax or dependency downloader. Qualification of same-named public types
from different modules remains G09.

## Linking and C

New module-owned functions can use Cx linkage. A redeclaration of an imported C
entity must preserve its existing C identity or produce a conflict diagnostic.
`extern(C)` exposes a C-facing entry where its contract can be represented.

A pure C project with no module directives does not need a manifest or Cx artifact.
An ordinary multi-input compiler invocation still normally compiles separate
translation units. Generic bodies/metadata shared across them require explicit
build scheduling or artifacts, not assumed omniscient lookup.

## Compiler artifacts

Versioned build artifacts can supply public generic bodies and conformance metadata
not textually in headers. They do not grant direct source access to internal helpers
whose implementation is needed transitively. See [Module ABI](../abi/modules-and-generics.md).
