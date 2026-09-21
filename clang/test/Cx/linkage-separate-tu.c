// The Cx symbol is the same in a producer and a consumer compiled separately
// (M3), and an unowned implementation file inherits the header's identity.

// RUN: %clang -x cx -std=gnu17 -I %S/Inputs -S -emit-llvm -o - \
// RUN:   %S/Inputs/cx-geom-impl.c | FileCheck --check-prefix=PRODUCER %s
// RUN: %clang -x cx -std=gnu17 -I %S/Inputs -S -emit-llvm -o - %s \
// RUN:   | FileCheck --check-prefix=CONSUMER %s

#include "cx-geom.h"

int consume(void) { return rect_area(6, 7); }

// PRODUCER-DAG: define {{.*}}@"_Z23_Cx0$Geometry$rect_areaii"
// CONSUMER-DAG: declare {{.*}}@"_Z23_Cx0$Geometry$rect_areaii"
// CONSUMER-DAG: define {{.*}}@consume(
