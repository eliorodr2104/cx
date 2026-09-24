// A storage class on a struct member is C's own error, and the access
// specifier in front of it must not turn it into a worse one (M4.4).

// RUN: %clang -x cx -std=gnu17 -fsyntax-only -Xclang -verify %s

#module Shapes

typedef struct Stored {
  int a;
  // `private` is recognised here; `static` is rejected on its own terms,
  // because C gives no storage class to a struct member. Cx has no
  // type-level storage of its own yet.
  private static int shared; // expected-error {{type name does not allow storage class to be specified}}
} Stored;
