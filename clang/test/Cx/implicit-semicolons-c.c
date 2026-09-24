// Valid C that spans lines keeps its meaning in Cx: every statement below is
// complete only at its ';', and compiles to the same code as C and as Cx.

// RUN: rm -rf %t && mkdir -p %t
// RUN: %clang -x c -std=gnu17 -S -emit-llvm -o - %s | grep -v -e ModuleID -e source_filename > %t/c.ll
// RUN: %clang -x cx -std=gnu17 -S -emit-llvm -o - %s | grep -v -e ModuleID -e source_filename > %t/cx.ll
// RUN: diff %t/c.ll %t/cx.ll
// RUN: %clang -x cx -std=gnu17 -fsyntax-only -Wcx-implicit-semicolon -Werror %s

int foo(int);
int bar = 2;
int table[3];

int calls(void) {
  int r = foo
  (bar);
  r = r
    + foo(1)
    - foo(2);
  r = table
    [1];
  r = r ?
      foo(3) :
      foo(4);
  return
    r;
}

struct S { int a; int b; }
  s = { 1, 2 },
  t;

int more(int x) {
  do
    x--;
  while (x > 0);
  if (x)
    x = 1;
  else
    x = 2;
  return x
    ;
}
