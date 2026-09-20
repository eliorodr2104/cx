// A visible C typedef, macro or object named `var` / `let` keeps its C meaning
// in Cx mode (M1).

// RUN: %clang -x cx -std=gnu17 -fsyntax-only -Xclang -verify %s
// RUN: %clang -x c -std=gnu17 -fsyntax-only -Xclang -verify %s
// expected-no-diagnostics

typedef int var;
typedef unsigned let;

var typedefed = 1;
let alsoTypedefed = 2;

#define let int
let macroed = 3;
#undef let

int use(void) {
  int var = 4;   // an ordinary object named 'var'
  var = var + 1;
  return typedefed + (int)alsoTypedefed + macroed + var;
}
