// Inside a method body an unqualified name is resolved against the receiver:
// a field, and a sibling method too. A local binding shadows it, and a free
// function is still a free function (M4.3).

// RUN: %clang -x cx -std=gnu17 -fsyntax-only -Xclang -verify %s
// RUN: %clang -x cx -std=gnu17 -S -emit-llvm -o - %s | FileCheck %s

// expected-no-diagnostics

#module Shapes

void free_function(void);

typedef struct Counter {
  int value;

  void bump(int by n) { value += n; }

  // A sibling method, called without `self.` and with its label.
  void twice(void) { bump(by: 1); bump(by: 1); }

  // A free function is still a free function inside a method.
  void notify(void) { free_function(); }

  // A local shadows the member, as it does for a field.
  ~mutating int shadowed(void) {
    int value = 3;
    return value;
  }
} Counter;
// CHECK-DAG: define {{.*}}@"_ZN7Counter{{[0-9]+}}_Cx0$Shapes$twiceEP7Counter"
// CHECK-DAG: call {{.*}}@"_ZN7Counter{{[0-9]+}}_Cx0$Shapes$bump$by:EP7Counteri"
// CHECK-DAG: call void @"_Z25_Cx0$Shapes$free_functionv"()

int use(Counter *c) {
  c->twice();
  c->notify();
  return c->shadowed();
}
