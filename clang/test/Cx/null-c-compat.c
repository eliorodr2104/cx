// An existing identifier or macro named `null` keeps working in Cx mode (M1).

// RUN: %clang -x cx -std=gnu17 -fsyntax-only -Xclang -verify %s
// RUN: %clang -x c -std=gnu17 -fsyntax-only -Xclang -verify %s
// expected-no-diagnostics

#define null ((void *)0)
int *macroed = null;
#undef null

int null = 7;

int use(void) {
  null = null + 1;
  return null;
}

static int shadowing(void) {
  int *null = &(int){0};
  return *null;
}
