// A generated initializer does not expose inaccessible state: it is only
// available where every field it writes may be written (M4).

// RUN: %clang -x cx -std=gnu17 -fsyntax-only -Xclang -verify %s

#module Shapes

typedef struct Guarded {
  int shown;
  private int hidden; // expected-note 2 {{private member 'hidden' declared here}}
} Guarded;

typedef struct Open { int a; int b; } Open;

// Even inside the owning module, file scope is not inside the type.
void outside(void) {
  Guarded g = Guarded(shown: 1, hidden: 2); // expected-error {{writing to private member 'hidden' is not permitted here}}
  (void)g;
}

// The type's own implementation may construct it.
struct Guarded {
  void reset() {
    Guarded fresh = Guarded(shown: 0, hidden: 0);
    self.shown = fresh.shown;
  }
};

// A wholly public struct is constructible anywhere.
void anywhere(void) {
  Open o = Open(a: 1, b: 2);
  (void)o;
}

// The restriction is on writing, so reading the public part still works.
int read_shown(Guarded *g) { return g->shown; }
int read_hidden(Guarded *g) { return g->hidden; } // expected-error {{private member 'hidden' is not permitted here}}
