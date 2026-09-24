// C23 accepts a redefinition of a tag with a compatible body. A Cx
// continuation is written the same way, so the two meet at every reopening:
// in an owned file the continuation wins, and an unowned file keeps C's rule.

// RUN: %clang -x cx -std=c23 -fsyntax-only -Xclang -verify=owned %s
// RUN: %clang -x cx -std=c23 -fsyntax-only -Xclang -verify=unowned %s -DUNOWNED
// RUN: %clang -x c -std=c23 -fsyntax-only -Xclang -verify=unowned %s -DUNOWNED

#ifndef UNOWNED
#module Shapes
#endif

struct Same { int a; }; // owned-note {{primary definition of 'Same' is here}}
#ifdef UNOWNED
// unowned-no-diagnostics
struct Same { int a; };
#else
struct Same { int a; }; // owned-error {{a continuation of 'Same' cannot add a stored field}}

struct Point { int x; int get(); };
struct Point { int get() { return self.x; } };
int use(struct Point *p) { return p->get(); }
#endif
