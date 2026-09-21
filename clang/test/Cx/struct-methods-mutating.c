// `~mutating` promises the receiver's stored value is not modified, which is
// what lets the method be called on a constant value (M4).

// RUN: %clang -x cx -std=gnu17 -fsyntax-only -Xclang -verify %s

#module Counters

struct Counter {
  int value;

  void increment() { self.value += 1; } // expected-note {{mark the method '~mutating' if it does not modify the receiver}}
  ~mutating int current() { return value; }
};

void on_mutable(void) {
  struct Counter c = { 0 };
  c.increment();
  c.current();
}

void on_constant(void) {
  const struct Counter c = { 0 };
  c.current();
  c.increment(); // expected-error {{cannot call mutating method 'increment' on a constant value of type 'const struct Counter'}}
}

struct Guard {
  int value;
  // The promise is enforced: a `~mutating` method cannot write the receiver.
  // expected-note@+1 {{variable 'self' declared const here}}
  ~mutating void cheat() { self.value = 1; } // expected-error {{cannot assign to variable 'self' with const-qualified type 'const struct Guard *'}}
};
