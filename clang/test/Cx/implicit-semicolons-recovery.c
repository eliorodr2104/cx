// Error recovery with optional semicolons: a ';' is implied at most once per
// token, so a parser loop that cannot make progress falls back to C's
// ordinary missing-';' recovery instead of standing still.

// RUN: not %clang -x cx -std=gnu17 -fsyntax-only %s 2>&1 | FileCheck %s

#module Recovery

struct Stuck {
  private )(set) int v
  ~ { mutating void m() {}
}

void f(void) {
  int x = 1
  ) ]
  x = 2
}

// CHECK: error:
// CHECK: errors generated
