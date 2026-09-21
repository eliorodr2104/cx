// A field may carry a declaration-site default. A field with one is not
// required at construction, so it is not part of the construction surface and
// its access does not bound where the type can be built (M4e).

// RUN: %clang -x cx -std=gnu17 -fsyntax-only -Xclang -verify %s
// RUN: %clang -x cx -std=gnu17 -S -emit-llvm -o - %s -DNO_ERRORS \
// RUN:   | FileCheck %s

// `= ...` on a struct member is not valid C.
// RUN: not %clang -x c -std=gnu17 -fsyntax-only %s 2>&1 \
// RUN:   | FileCheck --check-prefix=PLAINC %s
// PLAINC: expected ';' at end of declaration list

#module Shapes

typedef struct Guarded {
  int shown;
  private int hidden = 7;
} Guarded;

typedef struct Mixed {
  int a;
  int b = 2;
  int c;
  int d = 4;
} Mixed;

typedef struct AllDefault { int a = 1; int b = 2; } AllDefault;

int build(void) {
  // A private field with a default is not exposed, so the type stays
  // constructible from outside its implementation.
  Guarded g = Guarded(shown: 10);

  // Any defaulted field may be skipped, or given a value.
  Mixed m = Mixed(a: 1, c: 3);
  Mixed n = Mixed(a: 1, b: 9, c: 3);

  // An empty call is available when nothing is required.
  AllDefault e = AllDefault();

  return g.shown + m.a + m.b + n.b + e.a + e.b;
}
// CHECK: define {{.*}}@"_Z17_Cx0$Shapes$buildv"

// C initialization of the same type agrees: an omitted field takes its
// declared default, so the two spellings never disagree.
int braces(void) {
  Mixed m = { 1 };
  return m.b;
}
// CHECK: define {{.*}}@"_Z18_Cx0$Shapes$bracesv"

#ifndef NO_ERRORS
void missing(void) {
  // A field without a default still has to be given.
  Mixed bad = Mixed(b: 2); // expected-error {{construction of 'Mixed' (aka 'struct Mixed') expects the field label 'a' here}} \
                           // expected-error {{missing value for field 'c' in construction of 'Mixed' (aka 'struct Mixed'); it has no default}}
  // expected-note@22 {{field 'a' declared here}}
  // expected-note@24 {{field 'c' declared here}}
  (void)bad;
}

void still_private(void) {
  // Naming the private field is still writing it.
  Guarded g = Guarded(shown: 1, hidden: 2); // expected-error {{writing to private member 'hidden' is not permitted here}}
  // expected-note@18 {{private member 'hidden' declared here}}
  (void)g;
}
#else
// expected-no-diagnostics
#endif
