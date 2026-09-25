// Tuple diagnostics.
// RUN: %clang_cc1 -x cx -fsyntax-only -verify %s

struct Unknown; // expected-note {{forward declaration of 'struct Unknown'}}

(void, int) v;              // expected-error {{a tuple element cannot have type 'void'}}
(struct Unknown, int) inc;  // expected-error {{a tuple element cannot have incomplete type 'struct Unknown'}}
(int a, int a) dup;         // expected-error {{duplicate tuple element label 'a'}}
(int $a, int) dollar;       // expected-error {{a tuple element label cannot contain '$'}}
(int, float) pair;
var (fa, fb) = pair;        // expected-error {{a tuple can only be destructured inside a function}}

void f(void) {
  var one = (x: 1);         // expected-error {{a tuple needs at least two elements}}
  (int id, float score) w = (score: 1.0, id: 2);
  // expected-error@-1 {{tuple element 0 is labelled 'score', but '(int id, float score)' (aka '(int, float)') labels it 'id'}}
  // expected-error@-2 {{tuple element 1 is labelled 'id', but '(int id, float score)' (aka '(int, float)') labels it 'score'}}
  var (a, b, c) = pair;     // expected-error {{cannot destructure a tuple of 2 elements into 3 names}}
  var (m, n) = 5;           // expected-error {{cannot destructure a value of type 'int', which is not a tuple}}
  var (p, p) = pair;        // expected-error {{redefinition of 'p'}} expected-note {{previous definition is here}}
  (int, int) ii = pair;     // expected-error {{initializing '(int, int)' with an expression of incompatible type '(int, float)'}}
  (int, int, int) three = (1, 2); // expected-error {{initializing '(int, int, int)' with an expression of incompatible type '(int, int)'}}
  int x = pair.$2;          // expected-error {{no member named '$2' in '(int, float)'}}
  float y = w.name;         // expected-error {{no member named 'name' in '(int, float)'}}
  let l = (1, 2);           // expected-note {{variable 'l' declared const here}}
  l.$0 = 3;                 // expected-error {{cannot assign to variable 'l' with const-qualified type 'const (int, int)'}}
}
