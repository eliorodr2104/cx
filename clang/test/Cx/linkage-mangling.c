// A function owned by a Cx module has Cx linkage: its symbol carries the
// owning module and the parameter types (M3). The encoding is versioned and
// experimental; see cx-docs/abi/linkage-and-mangling.md.

// RUN: %clang -x cx -std=gnu17 -S -emit-llvm -o - %s | FileCheck %s

// The same file is untouched when compiled as plain C.
// RUN: %clang -x c -std=gnu17 -S -emit-llvm -o - %s -DNO_MODULE \
// RUN:   | FileCheck --check-prefix=PLAINC %s

// Cx mode alone changes nothing: ownership does.
// RUN: %clang -x cx -std=gnu17 -S -emit-llvm -o - %s -DNO_MODULE \
// RUN:   | FileCheck --check-prefix=PLAINC %s

#ifndef NO_MODULE
#module Geometry
#endif

// The module, the base name and the parameter types are all part of identity.
int rect_area(int w, int h) { return w * h; }
// CHECK-DAG: define {{.*}}@"_Z23_Cx0$Geometry$rect_areaii"
// PLAINC-DAG: define {{.*}}@rect_area(

// Overloadable-style distinctness: a different parameter type is a different
// symbol, which is what makes M3 overloads possible at all.
long scale(long v) { return v; }
// CHECK-DAG: define {{.*}}@"_Z19_Cx0$Geometry$scalel"
// PLAINC-DAG: define {{.*}}@scale(

// A translation-unit-local function is still mangled, with the ordinary
// internal-linkage marker. It stays TU-local either way.
static int helper(int v) { return v; }
int use_helper(void) { return helper(1); }
// CHECK-DAG: define internal {{.*}}@"_ZL20_Cx0$Geometry$helperi"
// PLAINC-DAG: define internal {{.*}}@helper(

// The program entry point keeps its C name whatever owns its file.
int main(void) { return rect_area(2, 3) + use_helper() + (int)scale(1); }
// CHECK-DAG: define {{.*}}@main(
// PLAINC-DAG: define {{.*}}@main(
