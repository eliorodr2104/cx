// A custom `init` is the complete construction interface of its type: it
// replaces the generated memberwise surface, takes argument labels and
// overloads like any other associated function, and sees `self` while it
// builds the value (M4f).

// RUN: %clang -x cx -std=gnu17 -fsyntax-only -Xclang -verify %s
// RUN: %clang -x cx -std=gnu17 -S -emit-llvm -o - %s | FileCheck %s

// expected-no-diagnostics

#module Shapes

typedef struct Rect {
  float width;
  float height;

  init(float width newWidth, float height newHeight) {
    self.width = newWidth;
    self.height = newHeight;
  }

  // Overloads differ by their labels, exactly as methods do.
  init(float side) {
    width = side;
    height = side;
  }

  ~mutating float area() { return width * height; }
} Rect;

float use(void) {
  Rect r = Rect(width: 3, height: 4);
  Rect s = Rect(5);
  return r.area() + s.area();
}
// CHECK-LABEL: define {{.*}}@"_Z15_Cx0$Shapes$usev"
// CHECK: call {{.*}}@"_ZN4Rect{{[0-9]+}}_Cx0$Shapes$init$width:height:EP4Rectff"
// CHECK: call {{.*}}@"_ZN4Rect{{[0-9]+}}_Cx0$Shapes$initEP4Rectf"

// A field default still participates: the object starts out holding it and
// the initializer only writes what it chooses to.
typedef struct Counter {
  int value = 7;
  int step;

  init(int step s) { step = s; }
} Counter;

int counted(void) {
  Counter c = Counter(step: 2);
  return c.value + c.step;
}
// CHECK-LABEL: define {{.*}}@"_Z19_Cx0$Shapes$countedv"

// An initializer may be declared with the type and implemented in a
// continuation, like any other associated function.
typedef struct Pair {
  int a;
  int b;
  init(int a x, int b y);
} Pair;

struct Pair {
  init(int a x, int b y) { self.a = x; self.b = y; }
};

int paired(void) {
  Pair p = Pair(a: 1, b: 2);
  return p.a + p.b;
}
// CHECK-LABEL: define {{.*}}@"_ZN4Pair{{[0-9]+}}_Cx0$Shapes$init$a:b:EP4Pairii"

// An initializer bounds construction by itself, not by the fields it writes:
// a private field stays hidden while the type stays constructible.
typedef struct Opaque {
  private int raw;

  internal init(int raw r) { raw = r; }
  ~mutating int get() { return raw; }
} Opaque;

int opaque(void) {
  Opaque o = Opaque(raw: 5);
  return o.get();
}
