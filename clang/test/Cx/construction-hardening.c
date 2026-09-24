// Generated construction and initializers: the cases M4.5 fixed.

// RUN: %clang -x cx -std=gnu17 -fsyntax-only -Xclang -verify %s
// RUN: %clang -x cx -std=gnu17 -S -emit-llvm -o - %s -DNO_ERRORS | FileCheck %s

#module Build

// A flexible array member has no storage in a value, and an unnamed
// bit-field is padding: construction writes neither.
typedef struct Buffer { int len; int data[]; } Buffer;
typedef struct Padded { int a; int : 4; int b; } Padded;
Buffer make_buffer(void) { return Buffer(len: 0); }
Padded make_padded(void) { return Padded(a: 1, b: 2); }
// CHECK-LABEL: define {{.*}}make_padded
// CHECK: store i32 1
// CHECK: store i32 2

// A construction may start a statement, or be the operand of sizeof, when
// nothing but an expression can begin with it.
typedef struct Point {
  int x;
  int y;
  void show() {}
  ~mutating int sum() { return x + y; }
} Point;
typedef struct Scale { int k; init(int f) { self.k = f; } } Scale;

void statements(void) {
  Point(x: 1, y: 2).show();
  (void)Point(x: 1, y: 2).sum();
  (void)Scale(3);
  (void)sizeof(Point(x: 1, y: 2));
  (void)sizeof(Scale(3));
}
_Static_assert(sizeof(Point(x: 1, y: 2)) == 2 * sizeof(int), "");

#ifndef NO_ERRORS
// `Point(p)` and `Point()` keep their C meaning: a declaration of `p`, and a
// function type, whose size GNU C defines as 1.
_Static_assert(sizeof(Point()) == 1, "");
void c_meaning(void) {
  Point (p);
  p.x = 1;
}

// Whether a type has initializers decides how every construction of it is
// built, so only the primary definition can declare one.
struct Late { int a; void f(); }; // expected-note {{primary definition of 'Late' is here}}
struct Late {
  init(int a v) { self.a = v; } // expected-error {{an initializer of 'Late' must be declared in its primary definition}}
  void f() {}
};

struct Early { int a; init(int a v); };
struct Early { init(int a v) { self.a = v; } };

union Either {
  int i;
  init(int v) { self.i = v; } // expected-error {{a union has no construction, so it cannot declare an initializer}}
};

// No method takes a volatile receiver.
volatile Point shared;
int use_volatile(void) {
  return shared.sum(); // expected-error {{cannot call Cx method 'sum' on a volatile value of type 'volatile Point'}}
}
#endif
