// Inputs that crashed code generation before M4.4, and the receiver lookup
// rule, checked in the emitted code.

// RUN: %clang -x cx -std=gnu23 -S -emit-llvm -o - %s | FileCheck %s
// RUN: %clang_cc1 -std=gnu23 -extract-api --pretty-sgf -triple arm64-apple-macosx \
// RUN:   -x cx-header %s -o - | FileCheck --check-prefix=API %s

// Cx has no symbol-graph vocabulary of its own yet, so it is reported as C.
// API: "interfaceLanguage": "c"

#module Crash

// A method body is real code even when its type is defined inside `typeof`
// or `sizeof`.
typeof(struct InTypeof { int x; int get() { return self.x; } }) in_typeof;
unsigned long in_sizeof = sizeof(struct InSizeof { int y; int get() { return y; } });
int use_typeof(void) { return in_typeof.get(); }
// CHECK-LABEL: define {{.*}}@"_ZN8InTypeof14_Cx0$Crash$getEP8InTypeof"(
// CHECK-LABEL: define {{.*}}@"_ZN8InSizeof14_Cx0$Crash$getEP8InSizeof"(

// A tag declared in a parameter list is mangled at file scope instead of
// recursing through the function that declares it.
int proto_struct(struct InParams { int x; } *p) { return p->x; }
int proto_enum(enum { ProtoZero } e) { return e; }
// CHECK-LABEL: define {{.*}}@"_Z23_Cx0$Crash$proto_structP8InParams"(
// CHECK-LABEL: define {{.*}}@"_Z21_Cx0$Crash$proto_enum

// Inside a method, only the method's own parameters and bindings shadow a
// member of the receiver. A file-scope declaration of the same name does not,
// whether the name starts a statement or an expression.
int count;
int total(void) { return 1; }

struct Bag {
  int count;
  ~mutating int size() { return count; }
  void add() { count++; }
  int total() { return count; }
  void reset() { total(); }
  int shadowed() { int count = 5; return count; }
};

// CHECK-LABEL: define {{.*}}@"_ZN3Bag15_Cx0$Crash$sizeEPK3Bag"(
// CHECK-NOT: $count"
// CHECK: ret i32
// CHECK-LABEL: define {{.*}}@"_ZN3Bag14_Cx0$Crash$addEP3Bag"(
// CHECK-NOT: $count"
// CHECK: ret void
// CHECK-LABEL: define {{.*}}@"_ZN3Bag16_Cx0$Crash$resetEP3Bag"(
// CHECK: call {{.*}}@"_ZN3Bag16_Cx0$Crash$totalEP3Bag"(
// CHECK-LABEL: define {{.*}}@"_ZN3Bag19_Cx0$Crash$shadowedEP3Bag"(
// CHECK: store i32 5, ptr %count
