// A module-owned entity has a mangled name carrying its labels and parameter
// types, so several entities can share a base name (M3).

// RUN: %clang -x cx -std=gnu17 -fsyntax-only -Xclang -verify %s
// RUN: %clang -x cx -std=gnu17 -S -emit-llvm -o - %s | FileCheck %s

// expected-no-diagnostics

#module Draw

// Overloads by parameter type.
void show(int value);
void show(double value);
// CHECK-DAG: declare {{.*}}@"_Z14_Cx0$Draw$showi"
// CHECK-DAG: declare {{.*}}@"_Z14_Cx0$Draw$showd"

// Overloads by argument label, including two that differ only in their label.
void move(int x value);
void move(float x value);
void move(int y value);
// CHECK-DAG: declare {{.*}}@"_Z17_Cx0$Draw$move$x:i"
// CHECK-DAG: declare {{.*}}@"_Z17_Cx0$Draw$move$x:f"
// CHECK-DAG: declare {{.*}}@"_Z17_Cx0$Draw$move$y:i"

// Ranking is per argument and uses C conversions only: for a short argument,
// the C promotion to int is preferred to a conversion to long.
void pick(int v);
void pick(long v);

void use(short s) {
  show(10);
  show(10.0);
  move(x: 1);
  move(x: 1.0f);
  move(y: 2);
  pick(s);
  // CHECK-DAG: call {{.*}}@"_Z14_Cx0$Draw$picki"
}
