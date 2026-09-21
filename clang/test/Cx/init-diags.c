// A custom initializer replaces the generated construction surface, and the
// rules it does not satisfy are reported where they are written (M4f).

// RUN: %clang -x cx -std=gnu17 -fsyntax-only -Xclang -verify %s

#module Shapes

typedef struct Point {
  int x;
  int y;

  init(int x nx) { x = nx; y = 0; } // expected-note 2 {{candidate function not viable}}
} Point;

void generated_is_gone(void) {
  // The memberwise surface is suppressed by the custom initializer, so the
  // field labels no longer name a way to build the type.
  Point p = Point(x: 1, y: 2); // expected-error {{no initializer of 'Point' (aka 'struct Point') accepts these values}}
  (void)p;
}

void wrong_label(void) {
  // Labels are part of an initializer's identity, so a wrong one leaves no
  // candidate rather than selecting one and complaining about the spelling.
  Point p = Point(nx: 1); // expected-error {{no initializer of 'Point' (aka 'struct Point') accepts these values}}
  (void)p;
}

void not_a_method(void) {
  Point p = Point(x: 1);
  p.init(x: 2); // expected-error {{no member named 'init' in 'struct Point'}}
}

// An initializer produces its own type and always establishes the value it
// builds.
typedef struct Bad {
  int a;
  int init(int a v); // expected-error {{a Cx initializer produces its own type, so it declares no return type}}
  ~mutating init(int a v); // expected-error {{an initializer establishes the value it builds, so it cannot be '~mutating'}}
} Bad;

// Running an initializer needs a function to run in.
typedef struct Point2 {
  int x;
  init(int x nx) { x = nx; }
} Point2;
Point2 global = Point2(x: 1); // expected-error {{construction of 'Point2' (aka 'struct Point2') runs an initializer, so it cannot appear outside a function}}

// `init` is still an ordinary identifier everywhere else.
typedef int init;
struct UsesTypedef { init (field); };
int takes_init(init v) { return v; }
