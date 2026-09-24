// A field default applies wherever C would zero-fill the field: in a nested
// record, in the elements an array initializer leaves out, around a
// designator, in an anonymous member, and in a type with an initializer
// (M4.5).

// RUN: %clang -x cx -std=gnu17 -fsyntax-only -Xclang -verify %s
// RUN: %clang -x cx -std=gnu17 -S -emit-llvm -o - %s | FileCheck %s
// RUN: %clang -x cx -std=gnu17 -O1 -S -emit-llvm -o - %s \
// RUN:   | FileCheck --check-prefix=OPT %s

#module Defaults

typedef struct In { int q = 4; } In;
typedef struct Mid { int x = 1; In in; } Mid;
typedef struct Pair { int a; int b = 2; } Pair;
typedef struct Wrap { int z; Pair p; } Wrap;
typedef struct Anon { struct { int s = 5; }; union { int u = 6; float f; }; } Anon;

Mid empty = {};
// CHECK: @"_Z{{[0-9]+}}_Cx0$Defaults$empty" = global %struct.Mid { i32 1, %struct.In { i32 4 } }

Pair elements[3] = {{1}};
// CHECK: @"_Z{{[0-9]+}}_Cx0$Defaults$elements" = global [3 x %struct.Pair] [%struct.Pair { i32 1, i32 2 }, %struct.Pair { i32 0, i32 2 }, %struct.Pair { i32 0, i32 2 }]

Pair designated[4] = {[3] = {1}};
// CHECK: @"_Z{{[0-9]+}}_Cx0$Defaults$designated" = global [4 x %struct.Pair] [%struct.Pair { i32 0, i32 2 }, %struct.Pair { i32 0, i32 2 }, %struct.Pair { i32 0, i32 2 }, %struct.Pair { i32 1, i32 2 }]

Wrap around = {.z = 1};
// CHECK: @"_Z{{[0-9]+}}_Cx0$Defaults$around" = global %struct.Wrap { i32 1, %struct.Pair { i32 0, i32 2 } }

Anon anon = {};
// CHECK: @"_Z{{[0-9]+}}_Cx0$Defaults$anon" = global %struct.Anon { %struct.anon { i32 5 }, %union.anon { i32 6 } }

// An initializer runs over an object that already holds every default,
// including those of its members.
typedef struct Rd { int d = 7; } Rd;
typedef struct Q {
  int n;
  Rd r;
  init(int n v) { self.n = v; }
} Q;

int init_nested(void) { Q q = Q(n: 3); return q.r.d; }
// OPT-LABEL: define {{.*}}init_nested
// OPT: ret i32 7

// A bit-field default is converted, and warned about, once.
typedef struct Bits { int small : 2 = 3; int g; } Bits; // expected-warning {{implicit truncation from 'int' to bit-field changes value from 3 to -1}}
Bits built(void) { return Bits(g: 1); }
