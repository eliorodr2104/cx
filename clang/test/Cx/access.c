// Cx access control (M4). Access is decided by the declaration's ownership,
// not by the module of whoever included the header.

// The type's own implementation reaches everything.
// RUN: %clang -x cx -std=gnu17 -I %S/Inputs -fsyntax-only \
// RUN:   %S/Inputs/cx-user-impl.c

// Code in the owning module reaches public and internal members.
// RUN: %clang -x cx -std=gnu17 -I %S/Inputs -fsyntax-only -Xclang -verify %s \
// RUN:   -DSAME_MODULE

// Code outside it reaches only public members.
// RUN: %clang -x cx -std=gnu17 -I %S/Inputs -fsyntax-only -Xclang -verify %s \
// RUN:   -DOTHER_MODULE

#ifdef SAME_MODULE
#module Users
#endif
#ifdef OTHER_MODULE
#module Other
#endif

#include "cx-user.h"

int read_members(struct User *u) {
  int a = u->name;
  int b = u->generation;
#ifdef OTHER_MODULE
  // expected-error@-2 {{internal member 'generation' is not permitted here}}
  // expected-note@Inputs/cx-user.h:10 {{internal member 'generation' declared here}}
#endif
  // A private(set) member keeps its read access.
  int c = u->id;
  return a + b + c;
}

#ifdef SAME_MODULE
// expected-error@+2 {{private member 'token' is not permitted here}}
// expected-note@Inputs/cx-user.h:8 {{private member 'token' declared here}}
int read_private(struct User *u) { return u->token; }
#endif
