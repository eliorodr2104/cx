// A default that reads a field comes through a PCH like any other.
// RUN: rm -rf %t && mkdir -p %t
// RUN: %clang_cc1 -O1 -x cx-header -emit-pch -o %t/r.pch %S/Inputs/cx-reads.h
// RUN: %clang_cc1 -O1 -x cx -include-pch %t/r.pch -verify -emit-llvm -o - %s | FileCheck %s
// RUN: %clang_cc1 -O1 -x cx -include %S/Inputs/cx-reads.h -verify -emit-llvm -o - %s | FileCheck %s
// expected-no-diagnostics

#module Shared

// CHECK-LABEL: define {{.*}}use
// CHECK: ret i32 8
int use(void) {
  Size s = Size(width: 4)
  return s.height
}
