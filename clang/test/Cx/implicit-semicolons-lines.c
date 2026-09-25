// A line that begins with `(`, `[`, `++`, `--`, or with `*`, `&`, `-`, `+`
// and then assigns, starts a new statement wherever C could not continue the
// previous one with it. Where C can, it still does.
// RUN: %clang -x cx -std=gnu17 -target arm64-apple-macosx -fsyntax-only -Werror %s
// RUN: %clang -x cx -std=gnu17 -target arm64-apple-macosx -fsyntax-only -Wcx-implicit-semicolon -Xclang -verify=warn %s
// RUN: %clang -x cx -std=gnu17 -target arm64-apple-macosx -S -emit-llvm -o - %s | FileCheck %s
// RUN: %clang -x cx -std=gnu17 -target arm64-apple-macosx -Xclang -ast-print -fsyntax-only %s > %t.print.c
// RUN: %clang -x cx -std=gnu17 -target arm64-apple-macosx -S -emit-llvm -o - %t.print.c | FileCheck %s

void effect(int);
int value(void);

// CHECK-LABEL: define {{.*}}@calls
// CHECK: call void @effect(i32 {{.*}}1)
// CHECK: call void @effect(i32 {{.*}}2)
// CHECK: call void @effect(i32 {{.*}}3)
void calls(void) {
  effect(1) // warn-warning {{';' implied by the line break}}
  (void)value() // warn-warning {{';' implied by the line break}}
  effect(2) // warn-warning {{';' implied by the line break}}
  (effect)(3) // warn-warning {{';' implied by the line break}}
}

// A function on the line before is still called, as in C.
// CHECK-LABEL: define {{.*}}@called
// CHECK: call void @effect(i32 {{.*}}4)
void called(void) {
  effect
  (4) // warn-warning {{';' implied by the line break}}
}

// CHECK-LABEL: define {{.*}}@stores
// CHECK: store i32 1, ptr %n
// CHECK: store i32 3, ptr
// CHECK: store i32 5, ptr
void stores(int *p, int *q) {
  int n = 1 // warn-warning {{';' implied by the line break}}
  *p = 3 // warn-warning {{';' implied by the line break}}
  n = n // warn-warning {{';' implied by the line break}}
  *q++ = 5 // warn-warning {{';' implied by the line break}}
  effect(n) // warn-warning {{';' implied by the line break}}
}

// Arithmetic across lines stays one expression.
// CHECK-LABEL: define {{.*}}@arith
// CHECK: sub nsw i32
// CHECK: mul nsw i32
int arith(int a, int b, int *p) {
  int x = a
    - b // warn-warning {{';' implied by the line break}}
  x = x
    *b // warn-warning {{';' implied by the line break}}
  return x // warn-warning {{';' implied by the line break}}
}

// CHECK-LABEL: define {{.*}}@counts
// CHECK: store i32 7, ptr %n
// CHECK: add nsw i32 {{.*}}, 1
void counts(void) {
  int n = 7 // warn-warning {{';' implied by the line break}}
  ++n // warn-warning {{';' implied by the line break}}
  effect(n) // warn-warning {{';' implied by the line break}}
}

// A pointer or array on the line before is still indexed, as in C.
// CHECK-LABEL: define {{.*}}@indexed
// CHECK: getelementptr inbounds i32, ptr {{.*}}, i64 2
int indexed(int *p) {
  return p
    [2] // warn-warning {{';' implied by the line break}}
}
