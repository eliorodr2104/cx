// C that looks like a tuple keeps its C meaning in Cx: the comma operator,
// casts, compound literals, parenthesized declarators, function-type
// parameters and a block literal whose parameter list follows an attribute.
// The IR is identical as C and as Cx.
// RUN: %clang -fblocks -x c -std=gnu89 -S -emit-llvm -o %t.c89.ll %s
// RUN: %clang -fblocks -x cx -std=gnu89 -S -emit-llvm -o %t.cx89.ll %s
// RUN: diff %t.c89.ll %t.cx89.ll
// RUN: %clang -fblocks -x c -std=gnu17 -S -emit-llvm -o %t.c17.ll %s
// RUN: %clang -fblocks -x cx -std=gnu17 -S -emit-llvm -o %t.cx17.ll %s
// RUN: diff %t.c17.ll %t.cx17.ll
// RUN: %clang -fblocks -x c -std=c23 -S -emit-llvm -o %t.c23.ll %s
// RUN: %clang -fblocks -x cx -std=c23 -S -emit-llvm -o %t.cx23.ll %s
// RUN: diff %t.c23.ll %t.cx23.ll

typedef int T;
struct S { int a, b; };

int take(int v) { return v; }
void g(int ((int, float)));
int (paren) = 1;

int comma(int a, int b) {
  int x = (a, b);
  x = (a, b);
  (a, b);
  (void)x;
  x += take((a, b));
  // A parenthesized callee after a cast has no type while it is parsed.
  x += (int)(take)((a, b));
  x += (int)sizeof(int) + (int)sizeof((T)a) + ((T)a, b);
  struct S s = (struct S){(a, b), 2};
  for ((x = 0, a = 1); x < 2; x++, a++)
    ;
  return (a, b) + s.a + x;
}

// After an attribute, `(int, const char *, ...)` is the block's parameter
// list: an abstract function declarator, not a tuple.
int blocks(void) {
  void (^b)(int, const char *, ...) =
      ^ __attribute__((__format__(__printf__, 2, 3))) (int arg, const char *f, ...) {};
  b(1, "x");
  return 0;
}

#if __STDC_VERSION__ < 199901L
// In C89, `const (int, float)` is a parameter of function type returning an
// implicit `const int`.
void implicit_int_param(const (int, float));

// In C89, `var(a, b)` calls an implicitly declared `var`; only `= ...` after
// it would make it a destructuring declaration.
int implicit(int a, int b) {
  var(a, b);
  return 0;
}
#endif
