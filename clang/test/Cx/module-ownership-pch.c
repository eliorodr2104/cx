// Module ownership travels with a serialized artifact, so a type read back
// from a PCH keeps the owner its source gave it (M4.2).

// RUN: %clang -x cx-header -std=gnu17 -o %t.pch %S/Inputs/cx-counter.h
// RUN: %clang -x cx -std=gnu17 -fcx-module=Counters -include-pch %t.pch \
// RUN:   -S -emit-llvm -o - %s | FileCheck %s

// Without the PCH the same source is a plain include, and has to agree.
// RUN: %clang -x cx -std=gnu17 -I %S/Inputs -S -emit-llvm -o - \
// RUN:   -DINCLUDE_HEADER %s | FileCheck %s

#ifdef INCLUDE_HEADER
#module Counters
#include "cx-counter.h"
#endif

// The record came from a file owned by Counters, so this block continues it
// rather than redefining it.
struct Counter {
  void increment() { self.value += 1; }
  ~mutating int current() { return value; }
};

// CHECK-DAG: define {{.*}}@"_ZN7Counter23_Cx0$Counters$incrementEP7Counter"
// CHECK-DAG: define {{.*}}@"_ZN7Counter21_Cx0$Counters$currentEPK7Counter"
