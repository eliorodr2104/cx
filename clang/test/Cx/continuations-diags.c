// What a Cx continuation may not do (M4).

// RUN: %clang -x cx -std=gnu17 -I %S/Inputs -fsyntax-only -Xclang -verify %s
// RUN: %clang -x cx -std=c23 -I %S/Inputs -fsyntax-only -Xclang -verify %s

#module Counters

struct Local { // expected-note {{primary definition of 'Local' is here}}
  int a;
  void m();
};

// A continuation adds behaviour, never storage.
struct Local {
  int b; // expected-error {{a continuation of 'Local' cannot add a stored field}}
  void m() {} // expected-note {{previous definition is here}}
};

// Exactly one implementation of a member.
struct Local {
  void m() {} // expected-error {{redefinition of 'm'}}
};
