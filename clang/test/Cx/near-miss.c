// A continuation member that nearly matches a declared one implements
// nothing: it introduces a second member and leaves the declared one without
// a definition. Reported here rather than at link time (M4.2).

// RUN: %clang -x cx -std=gnu17 -fsyntax-only -Xclang -verify %s
// RUN: %clang -x cx -std=gnu17 -fsyntax-only -Wno-cx-near-miss %s

#module Shapes

struct Drifted {
  int v;
  void set(int value x);     // expected-note {{'set' is declared here with the argument labels 'value:'}}
  void scale(int factor f);  // expected-note {{'scale' is declared here with the argument labels 'factor:'}}
  void keep(int only o);
  void helper();
};

struct Drifted {
  // The label drifted, so this is not the declared member.
  void set(int val x) { self.v = x; } // expected-error {{this definition does not match the declared member 'set'; it introduces a new member and leaves the declared one unimplemented}}

  // The parameter type drifted, with the same label.
  void scale(long factor f) { self.v *= (int)f; } // expected-error {{this definition does not match the declared member 'scale'; it introduces a new member and leaves the declared one unimplemented}}

  // An exact match is the declared member, and says nothing.
  void keep(int only o) { self.v = o; }

  // A member introduced only in the continuation is not a near miss: no
  // declared member shares its name.
  void helper() { self.v = 0; }
  void fresh(int a v) { self.v = v; }
};
