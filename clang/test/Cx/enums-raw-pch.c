// Cx enums made in a PCH keep their cases, backing types and scoping.
// RUN: rm -rf %t && mkdir -p %t
// RUN: %clang_cc1 -x cx-header -emit-pch -o %t/enum.pch %S/Inputs/cx-enum.h
// RUN: %clang_cc1 -x cx -include-pch %t/enum.pch -verify -emit-llvm -o - %s | FileCheck %s
// RUN: %clang_cc1 -x cx -include %S/Inputs/cx-enum.h -verify -emit-llvm -o - %s | FileCheck %s
// expected-no-diagnostics

#module Shared

// CHECK-LABEL: define {{.*}}levelv
// CHECK: store i8 2
unsigned char level(void) { enum Level l = .high; return l.rawValue; }
// CHECK-LABEL: define {{.*}}modev
// CHECK: ret i8 1
enum Mode mode(void) { return Mode.slow; }
