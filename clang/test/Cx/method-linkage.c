// Method linkage: a method defined in its type's body is emitted where it is
// used and merged at link time, so a type with methods can live in a shared
// header. One implemented in a continuation is a strong definition, and one
// whose type no other translation unit can name is internal.

// RUN: %clang -x cx -std=gnu17 -I %S/Inputs -S -emit-llvm -o - %s | FileCheck %s
// RUN: %clang -x cx -std=gnu17 -I %S/Inputs -S -emit-llvm -o - %s \
// RUN:   | FileCheck --check-prefix=UNUSED %s

#module Geo
#include "cx-method-header.h"

int use_header(Pt *p) { return p->get(); }
// CHECK-DAG: define linkonce_odr {{.*}}@"_ZN2Pt{{[0-9]+}}_Cx0$Geo$getEP2Pt"(

// The header only declared `set`; this file implements it.
struct Pt { void set(int v) { self.x = v; } };
// CHECK-DAG: define {{(dso_local )?}}void @"_ZN2Pt{{[0-9]+}}_Cx0$Geo$setEP2Pti"(

// A method nothing uses is not emitted.
typedef struct Unused { int v; int get() { return v; } } Unused;
// UNUSED-NOT: Unused{{.*}}get

// An unnamed type named by a typedef takes the typedef name for linkage.
typedef struct { int w; void inc() { self.w++; } } Anon;
void use_anon(Anon *a) { a->inc(); }
// CHECK-DAG: define linkonce_odr {{.*}}@"_ZN4Anon{{[0-9]+}}_Cx0$Geo$incEP4Anon"(

// Nothing else can name these types, so their methods are internal.
static struct { int v; int f() { return self.v; } } unnamed;
int use_unnamed(void) { return unnamed.f(); }
// CHECK-DAG: define internal {{.*}}$_0{{.*}}_Cx0$Geo$f

int use_local(void) {
  struct Local { int v; int f() { return self.v; } } l = {3};
  return l.f();
}
// CHECK-DAG: define internal {{.*}}Local{{.*}}_Cx0$Geo$f
