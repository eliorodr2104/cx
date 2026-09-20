// Malformed Cx `#module` directives (M2).

// RUN: %clang -x cx -std=gnu17 -fsyntax-only -Xclang -verify %s

#module // expected-error {{expected a module name after '#module'}}

int after(void);
