// The build can assign the primary source file's Cx module (M2). It never
// assigns one to a file that source includes.

// RUN: %clang -x cx -std=gnu17 -I %S/Inputs -fcx-module=Root -Xclang -ast-dump \
// RUN:   -fsyntax-only %s | FileCheck %s

// Without an assignment this file has no owner at all.
// RUN: %clang -x cx -std=gnu17 -I %S/Inputs -Xclang -ast-dump -fsyntax-only %s \
// RUN:   | FileCheck --check-prefix=NONE %s

// The driver forwards the assignment to the frontend.
// RUN: %clang -x cx -fcx-module=Assigned -### -c %s 2>&1 \
// RUN:   | FileCheck --check-prefix=DRIVER %s

// An explicit `#module` in the primary file must agree with the assignment.
// RUN: %clang -x cx -std=gnu17 -fcx-module=Geometry -fsyntax-only \
// RUN:   %S/Inputs/cx-owned.h
// RUN: not %clang -x cx -std=gnu17 -fcx-module=Other -fsyntax-only \
// RUN:   %S/Inputs/cx-owned.h 2>&1 | FileCheck --check-prefix=MISMATCH %s

#include "cx-unowned.h"
// The assignment stops at the primary file.
// CHECK: FunctionDecl {{.*}} col:{{[0-9]+}} plain_helper

int assigned_entity(void);
// CHECK: FunctionDecl {{.*}} cx-module Root assigned_entity
// NONE-NOT: cx-module

// DRIVER: "-cc1"
// DRIVER-SAME: "-fcx-module=Assigned"
// MISMATCH: error: module 'Geometry' declared here conflicts with module 'Other' assigned to this file by the build
