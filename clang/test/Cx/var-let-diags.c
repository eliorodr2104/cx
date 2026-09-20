// Rejections for Cx `var` and `let` (M1).

// RUN: %clang -x cx -std=gnu17 -fsyntax-only -Xclang -verify %s
// RUN: %clang -x cx -std=c23 -fsyntax-only -Xclang -verify %s

int value = 10;

var missing; // expected-error {{declaration of variable 'missing' with deduced type 'var' requires an initializer}}
let unbound; // expected-error {{declaration of variable 'unbound' with deduced type 'let' requires an initializer}}

// `auto`, `__auto_type`, `var` and `let` stay four distinct spellings.
int cx_var_param(var x);          // expected-error {{'var' not allowed in function prototype}}
int cx_let_param(let y);          // expected-error {{'let' not allowed in function prototype}}
int gnu_param(__auto_type z);     // expected-error {{'__auto_type' not allowed in function prototype}}

// Only a Cx group deduces independently; __auto_type keeps its C rule.
__auto_type shared = 1, // expected-error {{'__auto_type' deduced as 'int' in declaration of 'shared' and deduced as 'double' in declaration of 'mismatch'}}
            mismatch = 2.0;

void immutable_bindings(void) {
  let bound = &value; // expected-note {{variable 'bound' declared const here}}
  bound = &value; // expected-error {{cannot assign to variable 'bound' with const-qualified type 'int *const'}}

  let scalar = 1; // expected-note {{variable 'scalar' declared const here}}
  scalar = 2; // expected-error {{cannot assign to variable 'scalar' with const-qualified type 'const int'}}
}
