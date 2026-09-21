// Module-owned data carries its module in its symbol, the same way a function
// does, so two modules can hold a global of the same name (M4.2).

// RUN: %clang -x cx -std=gnu17 -I %S/Inputs -S -emit-llvm -o - %s \
// RUN:   | FileCheck %s

#module Shapes

#include "cx-foreign.h"

// An externally visible file-scope variable in an owned file is a Cx entity.
int counter = 0;
// CHECK-DAG: @"_Z19_Cx0$Shapes$counter" = global i32 0

// Internal linkage is already distinct from every other translation unit, so
// it keeps its plain name.
static int local = 1;
// CHECK-DAG: @local = internal global i32 1

// A variable first declared in an unowned header is a C entity and stays one,
// even when it is defined here.
int foreign_global = 2;
// CHECK-DAG: @foreign_global = global i32 2

int read(void) { return counter + local + foreign_global; }
