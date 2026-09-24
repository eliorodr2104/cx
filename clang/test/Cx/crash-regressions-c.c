// Plain C that crashed the compiler in Cx mode before M4.4: a cast applied to
// a parenthesized base of `->`, which the Cx method lookup saw before it had a
// type. It must mean exactly what it means in C.

// RUN: %clang -x c -fsyntax-only -Xclang -verify %s
// RUN: %clang -x cx -fsyntax-only -Xclang -verify %s
// expected-no-diagnostics

struct T { int c; long *p; };

long cast_member(struct T *t) { return (long)(t)->c; }
long cast_nested(char *b) { return *(long *)((struct T *)b)->p; }
