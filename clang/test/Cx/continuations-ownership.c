// A continuation reopens a type of its own module. A type owned by another
// module, or by no module at all, keeps C's redefinition rule (M4).

// A type owned by another module cannot be reopened here.
// RUN: not %clang -x cx -std=gnu17 -I %S/Inputs -fsyntax-only %s \
// RUN:   -DOTHER_MODULE 2>&1 | FileCheck --check-prefix=OTHER %s
// OTHER: error: redefinition of 'Counter'

// A plain C header's types stay C types and are not reopenable.
// RUN: not %clang -x cx -std=gnu17 -I %S/Inputs -fsyntax-only %s \
// RUN:   -DFOREIGN 2>&1 | FileCheck --check-prefix=FOREIGN %s
// FOREIGN: error: redefinition of 'Foreign'

// A type defined in this owned file is this module's, so reopening it is an
// ordinary continuation.
// RUN: %clang -x cx -std=gnu17 -I %S/Inputs -fsyntax-only %s -DLOCAL

#module Other

#ifdef OTHER_MODULE
#include "cx-counter.h"
struct Counter { void increment() {} };
#endif

#ifdef FOREIGN
#include "cx-foreign.h"
struct Foreign { void m() {} };
#endif

#ifdef LOCAL
struct Local { int a; void m(); };
struct Local { void m() { self.a = 1; } };
#endif
