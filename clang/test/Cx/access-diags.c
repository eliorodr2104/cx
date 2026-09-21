// Malformed Cx access specifiers, and names that are not specifiers (M4).

// RUN: %clang -x cx -std=gnu17 -fsyntax-only -Xclang -verify %s

#module M

struct Bad {
  private public(set) int v; // expected-error {{write access cannot be broader than read access}}
  internal internal int w; // expected-error {{read access is already specified}}
};

// `private` is contextual: a member or a variable may still be called that.
struct Named {
  int private;
  int internal;
};

int use(struct Named n) { return n.private + n.internal; }
