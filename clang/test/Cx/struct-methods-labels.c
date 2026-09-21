// A method's implicit receiver is not a written argument, so labels line up
// with the parameters the source actually declares (M4.1).

// RUN: %clang -x cx -std=gnu17 -fsyntax-only -Xclang -verify %s
// RUN: %clang -x cx -std=gnu17 -S -emit-llvm -o - %s -DNO_ERRORS \
// RUN:   | FileCheck %s

#module Shapes

struct T {
  int v;

  void resize(int width newWidth, int height newHeight) {
    self.v = newWidth + newHeight;
  }

  // Methods overload on their labels, like any other Cx entity.
  int pick(int value v) { return v; }
  int pick(int other v) { return -v; }
};

int use(struct T *t) {
  t->resize(width: 1, height: 2);
  return t->pick(value: 1) + t->pick(other: 2);
}
// CHECK-DAG: define {{.*}}@"_ZN1T23_Cx0$Shapes$pick$value:EP1Ti"
// CHECK-DAG: define {{.*}}@"_ZN1T23_Cx0$Shapes$pick$other:EP1Ti"

#ifndef NO_ERRORS
void wrong(struct T *t) {
  t->resize(1, 2); // expected-error {{no matching function for call to 'resize'}}
  // expected-note@13 {{candidate function not viable: expects argument label 'width:' for argument 1}}
  t->pick(1); // expected-error {{no matching function for call to 'pick'}}
  // expected-note@18 {{candidate function not viable: expects argument label 'value:' for argument 1}}
  // expected-note@19 {{candidate function not viable: expects argument label 'other:' for argument 1}}
}
#else
// expected-no-diagnostics
#endif
