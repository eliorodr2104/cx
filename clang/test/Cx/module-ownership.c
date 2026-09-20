// Cx `#module` records which module owns a physical file (M2). Ownership is a
// property of the file, not preprocessor state that leaks across an include.

// RUN: %clang -x cx -std=gnu17 -I %S/Inputs -Xclang -ast-dump -fsyntax-only %s \
// RUN:   | FileCheck %s

// Nothing is owned when the same sources are compiled as plain C.
// RUN: %clang -x c -std=gnu17 -I %S/Inputs -Xclang -ast-dump -fsyntax-only \
// RUN:   %S/Inputs/cx-unowned.h | FileCheck --check-prefix=PLAINC %s

#module Application

#include "cx-owned.h"
#include "cx-unowned.h"

// A header that declares its own owner keeps it, even though the file that
// includes it belongs to a different module. The owner is printed before the
// name, so these patterns distinguish an owned declaration from an unowned one.
// CHECK: FunctionDecl {{.*}} cx-module Geometry used rect_area

// A plain C header declares no owner and does not inherit the includer's.
// CHECK: FunctionDecl {{.*}} col:{{[0-9]+}} used plain_helper

// After the includes, this file's declarations still belong to Application.
int app_run(void) { return rect_area(2, 3) + plain_helper(); }
// CHECK: FunctionDecl {{.*}} cx-module Application app_run

// PLAINC-NOT: cx-module
