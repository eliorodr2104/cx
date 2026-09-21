// A redeclaration never changes an entity's identity (M3). Which file owns the
// definition does not matter; the first declaration decides.

// RUN: %clang -x cx -std=gnu17 -I %S/Inputs -S -emit-llvm -o - %s \
// RUN:   | FileCheck --implicit-check-not='Geometry$imported_c_entity' %s

#module Geometry

// Declared in an ordinary C header, so it is still that C entity even though
// it is defined inside a module. Renaming it here would leave the original C
// symbol unresolved for every C caller.
#include "cx-c-api.h"
int imported_c_entity(int v) { return v; }
// CHECK-DAG: define {{.*}}@imported_c_entity(

// Declared in a module-owned header, so it keeps Cx linkage here.
#include "cx-geom.h"
int rect_area(int w, int h) { return w * h; }
// CHECK-DAG: define {{.*}}@"_Z23_Cx0$Geometry$rect_areaii"

// A new function in this owned file gets Cx linkage.
int fresh_entity(void) { return 0; }
// CHECK-DAG: define {{.*}}@"_Z26_Cx0$Geometry$fresh_entityv"
