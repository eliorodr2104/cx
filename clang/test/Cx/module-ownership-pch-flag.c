// `-fcx-module=` assigns the primary source file. Building a PCH makes a header
// the primary file, and build systems pass it the flags of the sources it
// serves, so there the option does not apply: a C header keeps its C symbols.

// RUN: rm -rf %t && mkdir -p %t
// RUN: %clang -x cx-header -fcx-module=Boxes %S/Inputs/cx-pch-c.h -o %t/c.pch
// RUN: %clang -x cx -fcx-module=Boxes -include-pch %t/c.pch -S -emit-llvm \
// RUN:   -o - %s | FileCheck %s

int use(void) { return boxed(1); }
// CHECK-DAG: declare {{.*}}@boxed(
// CHECK-DAG: define {{.*}}@"_Z{{[0-9]+}}_Cx0$Boxes$usev"(
