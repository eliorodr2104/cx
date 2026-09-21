// Generated memberwise construction (M4). With no user-declared initializer,
// a struct is constructed from one labelled value per stored field.

// RUN: %clang -x cx -std=gnu17 -fsyntax-only -Xclang -verify %s
// RUN: %clang -x cx -std=gnu17 -S -emit-llvm -o - %s | FileCheck %s

// A type name in expression position is not valid C.
// RUN: not %clang -x c -std=gnu17 -fsyntax-only %s 2>&1 \
// RUN:   | FileCheck --check-prefix=PLAINC %s
// PLAINC: unexpected type name 'Size'

// expected-no-diagnostics

#module Shapes

typedef struct Size { int width; int height; } Size;

int make(void) {
  Size s = Size(width: 80, height: 40);
  return s.width + s.height;
}
// The fields are initialized in declaration order, here constant-folded.
// CHECK-DAG: @"__const._Z16_Cx0$Shapes$makev.s" = {{.*}}%struct.Size { i32 80, i32 40 }
// CHECK-DAG: define {{.*}}@"_Z16_Cx0$Shapes$makev"

// An ordinary call is still a call.
int callee(int v);
int call(void) { return callee(1); }
