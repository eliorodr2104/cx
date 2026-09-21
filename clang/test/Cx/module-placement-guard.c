// A header guard is not a declaration, so `#module` inside one is the
// documented form and stays accepted (M4.2).

// RUN: %clang -x cx -std=gnu17 -fsyntax-only -Xclang -verify %s
// RUN: %clang -x cx -std=gnu17 -S -emit-llvm -o - %s | FileCheck %s

// expected-no-diagnostics

#ifndef GUARDED_H
#define GUARDED_H

#module Guarded

int owned(void) { return 1; }
// CHECK: define {{.*}}@"_Z18_Cx0$Guarded$ownedv"

#endif
