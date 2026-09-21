// Rejections for Cx struct methods (M4).

// RUN: %clang -x cx -std=gnu17 -fsyntax-only -Xclang -verify %s

#module Counters

struct Counter {
  int value;
  void increment() { self.value += 1; }
};

void (*fp)(struct Counter *);

void use(struct Counter c) {
  // A method is bound to its receiver, so it never decays to a plain pointer.
  (void)c.increment; // expected-error {{reference to Cx method 'increment' must be a call}}
  fp = c.increment; // expected-error {{reference to Cx method 'increment' must be a call}}

  // `.` on the implicit receiver is not a general shorthand for pointers.
  struct Counter *p = &c;
  p.value = 1; // expected-error {{member reference type 'struct Counter *' is a pointer; did you mean to use '->'?}}
}
