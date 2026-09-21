// Overload resolution explains which label or type made a candidate
// inapplicable, and never picks by declaration order (M3).

// RUN: %clang -x cx -std=gnu17 -fsyntax-only -Xclang -verify %s

#module Draw

void move(int x value); // expected-note 2 {{candidate function not viable: expects argument label 'x:' for argument 1}}
void move(int y value); // expected-note 2 {{candidate function not viable: expects argument label 'y:' for argument 1}}

void two(int a, long b); // expected-note {{candidate function}}
void two(long a, int b); // expected-note {{candidate function}}

void use(void) {
  move(1); // expected-error {{no matching function for call to 'move'}}
  move(z: 1); // expected-error {{no matching function for call to 'move'}}
  two(1, 1); // expected-error {{call to 'two' is ambiguous}}
}
