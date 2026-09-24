// A block naming an already complete owned type is a continuation that
// implements its members, not a second definition (M4).

// RUN: %clang -x cx -std=gnu17 -I %S/Inputs -S -emit-llvm -o - \
// RUN:   %S/Inputs/cx-counter-impl.c | FileCheck --check-prefix=IMPL %s
// RUN: %clang -x cx -std=gnu17 -I %S/Inputs -S -emit-llvm -o - %s \
// RUN:   | FileCheck %s
// C23 accepts a compatible redefinition of a tag, which a continuation always
// looks like; in an owned file the continuation wins.
// RUN: %clang -x cx -std=c23 -I %S/Inputs -S -emit-llvm -o - \
// RUN:   %S/Inputs/cx-counter-impl.c | FileCheck --check-prefix=IMPL %s
// RUN: %clang -x cx -std=gnu23 -I %S/Inputs -S -emit-llvm -o - %s \
// RUN:   | FileCheck %s

#include "cx-counter.h"

int use(void) {
  struct Counter c = { 40 };
  c.increment();
  return c.current();
}

// The declaration in the header and the definition in the continuation are one
// entity, so there is exactly one symbol and the consumer references it.
// IMPL-DAG: define {{.*}}@"_ZN7Counter23_Cx0$Counters$incrementEP7Counter"
// IMPL-DAG: define {{.*}}@"_ZN7Counter21_Cx0$Counters$currentEPK7Counter"
// IMPL-DAG: define {{.*}}@"_ZN7Counter29_Cx0$Counters$resetLocalStateEP7Counter"
// CHECK-DAG: declare {{.*}}@"_ZN7Counter23_Cx0$Counters$incrementEP7Counter"
// CHECK-DAG: declare {{.*}}@"_ZN7Counter21_Cx0$Counters$currentEPK7Counter"
