// Write access covers every modification, not only `=` (M4).

// RUN: %clang -x cx -std=gnu17 -I %S/Inputs -fsyntax-only -Xclang -verify %s

#module Other
#include "cx-user.h"

// expected-note@Inputs/cx-user.h:9 6 {{private member 'id' declared here}}

void modify(struct User *u) {
  u->id = 5;  // expected-error {{writing to private member 'id' is not permitted here}}
  u->id += 1; // expected-error {{writing to private member 'id' is not permitted here}}
  u->id++;    // expected-error {{writing to private member 'id' is not permitted here}}
  --u->id;    // expected-error {{writing to private member 'id' is not permitted here}}

  // A mutable pointer to the member is a way to write it.
  int *p = &u->id; // expected-error {{writing to private member 'id' is not permitted here}}
  (void)p;
}

// Reading is still public, and so is an address that cannot write.
int read_ok(struct User *u) { return u->id; }
const int *addr_ok(const struct User *u) { return &u->id; }

// A mutable pointer is rejected even where the result is only read, because
// the expression itself produces a writable pointer. The exact
// address-exposure rule is G08.
const int *addr_from_mutable(struct User *u) {
  return &u->id; // expected-error {{writing to private member 'id' is not permitted here}}
}
