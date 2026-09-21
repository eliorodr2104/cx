// Cx struct methods (M4). A method is an associated function with an implicit
// `self` receiver: it adds no storage and no per-instance pointer.

// RUN: %clang -x cx -std=gnu17 -fsyntax-only -Xclang -verify %s
// RUN: %clang -x cx -std=gnu17 -S -emit-llvm -o - %s | FileCheck %s

// A function member is not valid C, so no C program changes meaning.
// RUN: not %clang -x c -std=gnu17 -fsyntax-only %s -DNO_MODULE 2>&1 \
// RUN:   | FileCheck --check-prefix=PLAINC %s
// PLAINC: field 'increment' declared as a function

// expected-no-diagnostics

#ifndef NO_MODULE
#module Counters
#endif

struct Counter {
  int value;

  void increment() {
    self.value += 1;    // `.` on the implicit receiver
  }

  ~mutating int current() {
    return value;       // an unqualified field of the receiver
  }

  // A method may be declared before the fields it uses.
  ~mutating int doubled() { return value * 2; }
};

// The receiver is a leading parameter, so the record gains no storage.
struct Plain { int value; };
_Static_assert(sizeof(struct Counter) == sizeof(struct Plain), "layout changed");

int use(void) {
  struct Counter c = { 0 };
  c.increment();
  c.increment();
  return c.current() + c.doubled();
}

// The symbol carries the record, the module and the parameter types, and the
// receiver is an ordinary pointer parameter. `~mutating` is a const receiver.
// CHECK-DAG: define {{.*}}@"_ZN7Counter23_Cx0$Counters$incrementEP7Counter"(ptr noundef %self)
// CHECK-DAG: define {{.*}}@"_ZN7Counter21_Cx0$Counters$currentEPK7Counter"(ptr noundef %self)

// A call passes the receiver's address; nothing is copied.
// CHECK: call void @"_ZN7Counter23_Cx0$Counters$incrementEP7Counter"(ptr noundef %c)

// `(void)` is an empty parameter list written the C way: the declarator
// carries a void pseudo-parameter that the receiver must not be added to.
typedef struct Empty {
  int v;
  ~mutating int read(void) { return v; }
} Empty;
int use_empty(Empty *e) { return e->read(); }
