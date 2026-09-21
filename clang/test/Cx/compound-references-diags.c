// Rejections for Cx compound names (M3).

// RUN: %clang -x cx -std=gnu17 -fsyntax-only -Xclang -verify %s

#module Draw

void move(int x value); // expected-note 2 {{candidate has the argument labels 'x:'}}
void move(float x value); // expected-note 2 {{candidate has the argument labels 'x:'}}

int not_a_function;

void (*wrong_label)(int) = &move(z:); // expected-error {{no function named 'move' has the argument labels 'z:'}}
void (*wrong_arity)(int) = &move(x:y:); // expected-error {{no function named 'move' has the argument labels 'x:y:'}}
void (*not_fn)(int) = &not_a_function(x:); // expected-error {{a compound name must refer to a function}}

// Without sufficient context the remaining candidates are ambiguous.
void use(void) {
  (void)&move(x:); // expected-error {{reference to overloaded function could not be resolved; did you mean to call it?}}
  // expected-note@7 {{possible target for call}}
  // expected-note@8 {{possible target for call}}
}
