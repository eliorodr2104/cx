// Access is checked on every member a write or a pointer reaches, not only on
// the outermost one: an element of an array member, a field of a member
// struct, a field of an anonymous member (M4.5).

// RUN: %clang -x cx -std=gnu17 -I %S/Inputs -fsyntax-only -Xclang -verify %s

#module Other
#include "cx-nested.h"

// expected-note@Inputs/cx-nested.h:13 {{private member 'hidden' declared here}}
// expected-note@Inputs/cx-nested.h:13 {{private member 'hidden' declared here}}
// expected-note@Inputs/cx-nested.h:14 {{private member 'inner' declared here}}
// expected-note@Inputs/cx-nested.h:15 4 {{private member 'arr' declared here}}
// expected-note@Inputs/cx-nested.h:16 3 {{private member 'cell' declared here}}
// expected-note@Inputs/cx-nested.h:17 2 {{private member 'cells' declared here}}
// expected-note@Inputs/cx-nested.h:23 2 {{private member 'secret' declared here}}

int reads(Holder *h) {
  int a = h->hidden; // expected-error {{private member 'hidden' is not permitted here}}
  int b = h->inner; // expected-error {{private member 'inner' is not permitted here}}
  // Reading through `private(set)` is public, including through a method that
  // promises not to write.
  int c = h->arr[1] + h->cell.v + h->cells[0].v;
  int d = h->cell.get() + h->cells[1].get();
  const int *e = h->fixed;
  return a + b + c + d + *e;
}

void writes(Holder *h) {
  h->hidden = 1; // expected-error {{private member 'hidden' is not permitted here}}
  h->arr[0] = 2; // expected-error {{writing to private member 'arr' is not permitted here}}
  h->arr[1]++; // expected-error {{writing to private member 'arr' is not permitted here}}
  h->cell.v = 3; // expected-error {{writing to private member 'cell' is not permitted here}}
  h->cells[0].v = 4; // expected-error {{writing to private member 'cells' is not permitted here}}
  // A mutating method writes its receiver.
  h->cell.bump(); // expected-error {{writing to private member 'cell' is not permitted here}}
  h->cells[1].bump(); // expected-error {{writing to private member 'cells' is not permitted here}}
}

void escapes(Holder *h) {
  int *p = h->arr; // expected-error {{writing to private member 'arr' is not permitted here}}
  int *q = &h->arr[1]; // expected-error {{writing to private member 'arr' is not permitted here}}
  int *r = &h->cell.v; // expected-error {{writing to private member 'cell' is not permitted here}}
  (void)p; (void)q; (void)r;
}

// A field with a default is written by a brace initializer only when the list
// names it.
Guarded g1 = { 1 };
Guarded g2 = { .shown = 1 };
Guarded g3 = { 1, 2 }; // expected-error {{writing to private member 'secret' is not permitted here}}
Guarded g4 = { .secret = 2 }; // expected-error {{writing to private member 'secret' is not permitted here}}
