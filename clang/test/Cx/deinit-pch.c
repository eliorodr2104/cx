// Resource types made in a PCH keep their deinit, the implicit one included.
// RUN: rm -rf %t && mkdir -p %t
// RUN: %clang_cc1 -x cx-header -emit-pch -o %t/r.pch %S/Inputs/cx-resource.h
// RUN: %clang_cc1 -x cx -include-pch %t/r.pch -verify -emit-llvm -o - %s | FileCheck %s
// RUN: %clang_cc1 -x cx -include %S/Inputs/cx-resource.h -verify -emit-llvm -o - %s | FileCheck %s

// expected-no-diagnostics

#module Shared

// CHECK-LABEL: define {{.*}}use
// CHECK: call void @"{{.*}}Owner{{.*}}deinit
// CHECK: call void @"{{.*}}Handle{{.*}}deinit
int use(void) {
  Handle a = Handle(1)
  Owner o = Owner(h: Handle(2), n: 3)
  return a.id + o.n
}

