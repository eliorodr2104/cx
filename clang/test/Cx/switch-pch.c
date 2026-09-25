// A Cx switch on a payload enum made in a PCH.
// RUN: rm -rf %t && mkdir -p %t
// RUN: %clang_cc1 -x cx-header -emit-pch -o %t/p.pch %S/Inputs/cx-payload.h
// RUN: %clang_cc1 -x cx -include-pch %t/p.pch -verify -emit-llvm -o - %s | FileCheck %s
// RUN: %clang_cc1 -x cx -include %S/Inputs/cx-payload.h -verify -emit-llvm -o - %s | FileCheck %s
// expected-no-diagnostics

#module Shared

// CHECK-LABEL: define {{.*}}area
// CHECK: switch i32
double area(Shape s) {
  switch (s) {
    case .circle(r): return 3.0 * r * r;
    case .rect(w, h): return w * h;
    case .none: return 0;
  }
}
