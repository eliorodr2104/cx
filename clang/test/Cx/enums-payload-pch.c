// A payload enum made in a PCH keeps its payloads and its value struct.
// RUN: rm -rf %t && mkdir -p %t
// RUN: %clang_cc1 -x cx-header -emit-pch -o %t/p.pch %S/Inputs/cx-payload.h
// RUN: %clang_cc1 -x cx -include-pch %t/p.pch -verify -emit-llvm -o - %s | FileCheck %s
// RUN: %clang_cc1 -x cx -include %S/Inputs/cx-payload.h -verify -emit-llvm -o - %s | FileCheck %s
// expected-no-diagnostics

#module Shared

// CHECK-LABEL: define {{.*}}squared
// CHECK: store i8 1,
Shape square(double s) { return .rect(w: s, h: s) }
_Static_assert(sizeof(Shape) == 24, "tag and two doubles");
