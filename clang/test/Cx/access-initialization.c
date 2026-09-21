// Initializing a record writes its fields, so it needs the same write access
// a field assignment needs (M4.1). Without this an aggregate initializer is a
// way around every access restriction on the type.

// RUN: %clang -x cx -std=gnu17 -fsyntax-only -Xclang -verify %s

#module Shapes

typedef struct Guarded {
  int shown;
  private int hidden; // expected-note 5 {{private member 'hidden' declared here}}
} Guarded;

typedef struct Open { int a; int b; } Open;

typedef struct Nested { Guarded inner; } Nested;

void outside(Guarded *g) {
  Guarded a = { 1, 2 };            // expected-error {{writing to private member 'hidden' is not permitted here}}
  Guarded b = { .shown = 1 };      // expected-error {{writing to private member 'hidden' is not permitted here}}
  Guarded c = (Guarded){ 1, 2 };   // expected-error {{writing to private member 'hidden' is not permitted here}}
  *g = (Guarded){ 3, 4 };          // expected-error {{writing to private member 'hidden' is not permitted here}}
  Nested n = { { 1, 2 } };         // expected-error {{writing to private member 'hidden' is not permitted here}}
  (void)a; (void)b; (void)c; (void)n;
}

// A wholly public struct initializes anywhere, as C always did.
void public_is_untouched(void) {
  Open o = { 1, 2 };
  Open p = { .a = 1 };
  (void)o; (void)p;
}

// The type's own implementation may initialize it.
struct Guarded {
  void reset() {
    Guarded fresh = { 0, 0 };
    self.shown = fresh.shown;
  }
};
