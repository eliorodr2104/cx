// Rejections for Cx `var` and `let` (M1).

// RUN: %clang -x cx -std=gnu17 -fsyntax-only -Xclang -verify %s
// RUN: %clang -x cx -std=c23 -fsyntax-only -Xclang -verify %s

int value = 10;

var missing; // expected-error {{declaration of variable 'missing' with deduced type 'var' requires an initializer}}
let unbound; // expected-error {{declaration of variable 'unbound' with deduced type 'const var' requires an initializer}}

void immutable_bindings(void) {
  let bound = &value; // expected-note {{variable 'bound' declared const here}}
  bound = &value; // expected-error {{cannot assign to variable 'bound' with const-qualified type 'int *const'}}

  let scalar = 1; // expected-note {{variable 'scalar' declared const here}}
  scalar = 2; // expected-error {{cannot assign to variable 'scalar' with const-qualified type 'const int'}}
}
