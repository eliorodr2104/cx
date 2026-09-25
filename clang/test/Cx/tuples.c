// Tuple types, literals, access, destructuring and conversions.
// RUN: %clang -x cx -target arm64-apple-macosx -fsyntax-only -Xclang -verify %s
// RUN: %clang -x cx -std=gnu89 -target arm64-apple-macosx -fsyntax-only -Xclang -verify %s
// RUN: %clang -x cx -std=c23 -target arm64-apple-macosx -fsyntax-only -Xclang -verify %s
// RUN: %clang -x cx -target arm64-apple-macosx -S -emit-llvm -o - %s | FileCheck %s
// RUN: %clang -x cx -target arm64-apple-macosx -E %s -o %t.i
// RUN: %clang -x cx-cpp-output -target arm64-apple-macosx -S -emit-llvm -o - %t.i | FileCheck %s
// RUN: %clang -x cx -target arm64-apple-macosx -Xclang -ast-print -fsyntax-only %s > %t.print.c
// RUN: %clang -x cx -target arm64-apple-macosx -S -emit-llvm -o - %t.print.c | FileCheck %s
// expected-no-diagnostics

#module Geo

// CHECK: %"struct.(int, float)" = type { i32, float }

(int, float) pair;
(int id, float score) user;
static (char, (int, int)) nested;
typedef (int x, int y) Point;
struct Box { (int, int) corner; };

// A tuple is returned, and passed, as the struct it is stored as.
// CHECK-LABEL: define {{.*}} @"_Z{{[0-9]+}}_Cx0$Geo$divideii"
(int, float) divide(int a, int b) {
  // CHECK: sitofp
  // CHECK: fdiv
  return (a / b, (float)a / b);
}

// CHECK-LABEL: define {{.*}} @"_Z{{[0-9]+}}_Cx0$Geo$takeu6$TupleIiiE"
int take((int x, int y) p) { return p.x + p.$1; }

// Every tuple of the same element types is the same type; labels are sugar.
(int, float) relabel((int n, float f) t) { return t; }

int use(void) {
  // Unlabelled, where a tuple is expected: each element converts.
  (int, float) t = (1, 2.5);
  var u = (id: 42, score: 1.5f);
  int a = t.$0;
  float b = u.score + u.$1;
  u.id = 7;
  user = u;
  pair = t;
  t = (3, 4.0);
  pair = relabel(user);

  var (q, r) = divide(7, 2);
  var (_, s) = u;
  let (i, j) = (1, 2);
  // CHECK: call {{.*}} @"_Z{{[0-9]+}}_Cx0$Geo$takeu6$TupleIiiE"
  take((1, 2));
  take((x: 3, y: 4));
  Point p = (x: 1, y: 2);
  p.x += p.$1;
  (int, float) *ptr = &t;
  ptr->$0 = ptr->$0 + 1;
  struct Box box = {{1, 2}};
  nested.$1.$0 = box.corner.$1;
  var two = (1, (2, 3));
  return a + (int)b + q + (int)r + (int)s + i + j + two.$1.$1 +
         (int)sizeof((int, float));
}

// A line that begins with a tuple type starts a declaration.
int lines(void) {
  var first = 1
  (int, float) next = (first, 2.0)
  return next.$0
}

// So does one after a struct or enum closed without ';', declared or defined.
// CHECK-LABEL: define {{.*}} @"_Z{{[0-9]+}}_Cx0$Geo$swapu6$TupleIiiE"
struct Pair { int a; }
(int, int) swap((int, int) p) { return (p.$1, p.$0) }
struct Pair
(int, int) *pairs;
// CHECK-LABEL: define {{.*}} @"_Z{{[0-9]+}}_Cx0$Geo$sidesv"
enum Side { Left, Right }
(int, int) sides(void) { return (Left, Right) }

// A parenthesized name is still C's declarator.
struct Wrapped { int v; }
(wrapped);
int unwrap(void) { return wrapped.v + pairs->$0; }
