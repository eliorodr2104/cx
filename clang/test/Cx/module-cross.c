// An entity belongs to the module that declares it first. Another module
// declaring the same entity is an error rather than a silent merge whose
// symbol depends on include order; a different overload is its own entity.

// RUN: %clang -x cx -std=gnu17 -I %S/Inputs -fsyntax-only -Xclang -verify %s
// RUN: %clang -x cx -std=gnu17 -I %S/Inputs -S -emit-llvm -o - %s -DOVERLOAD \
// RUN:   | FileCheck %s

#ifdef OVERLOAD
#include "cx-mod-a.h"
#include "cx-mod-b-only.h"
int use(void) { return get(1) + get(1.0); }
// CHECK-DAG: call {{.*}}@"_Z{{[0-9]+}}_Cx0$Alpha$geti"(
// CHECK-DAG: call {{.*}}@"_Z{{[0-9]+}}_Cx0$Beta$getd"(
#else
#include "cx-mod-a.h" // expected-note@cx-mod-a.h:2 {{previous declaration is here}} expected-note@cx-mod-a.h:3 {{previous declaration is here}}
#include "cx-mod-b.h"
#endif
