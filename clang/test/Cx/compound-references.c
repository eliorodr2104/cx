// A compound name refers to a function without calling it, filtering an
// overload set by argument labels (M3).

// RUN: %clang -x cx -std=gnu17 -S -emit-llvm -o - %s | FileCheck %s

#module Draw

void move(int x value);
void move(float x value);
void move(int y value);
void plain(int a, int b);

// The compound name filters to the `x:` declarations; the pointer type then
// selects among what is left.
void (*as_int)(int) = &move(x:);
// CHECK-DAG: @"_Z16_Cx0$Draw$as_int" = global ptr @"_Z17_Cx0$Draw$move$x:i"
void (*as_float)(float) = &move(x:);
// CHECK-DAG: @"_Z18_Cx0$Draw$as_float" = global ptr @"_Z17_Cx0$Draw$move$x:f"

// A single match needs no further selection.
void (*only_y)(int) = &move(y:);
// CHECK-DAG: @"_Z16_Cx0$Draw$only_y" = global ptr @"_Z17_Cx0$Draw$move$y:i"

// `_` names a position that has no label.
void (*unlabeled)(int, int) = &plain(_:_:);
// CHECK-DAG: @"_Z19_Cx0$Draw$unlabeled" = global ptr @"_Z15_Cx0$Draw$plainii"

// A call is still a call, and empty parentheses are still a call.
int nothing(void);
int call_still_works(void) { move(x: 1); return nothing(); }
// CHECK: call {{.*}}@"_Z17_Cx0$Draw$move$x:i"
