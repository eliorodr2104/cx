// An initializer may delegate to another with self.init, exactly once on
// every path, touching self only after it.
// RUN: %clang -x cx -target arm64-apple-macosx -fsyntax-only -Xclang -verify %s
// RUN: %clang -x cx -target arm64-apple-macosx -S -emit-llvm -o - %s | FileCheck %s
// RUN: %clang -x cx -target arm64-apple-macosx -Xclang -ast-print -fsyntax-only %s > %t.print.c
// RUN: %clang -x cx -target arm64-apple-macosx -S -emit-llvm -o - %t.print.c | FileCheck %s
// RUN: FileCheck --check-prefix=PRINT %s < %t.print.c
// expected-no-diagnostics

#module Delegation

void note(int)

typedef struct Rect {
  float width
  float height

  init(float width w, float height h) {
    width = w
    height = h
  }

  // CHECK-LABEL: define {{.*}}$init{{.*}}f"(ptr {{.*}}%self, float
  // CHECK: call void @"{{.*}}$init$width:height:{{.*}}"(ptr {{.*}}%{{.*}}, float
  // PRINT: self->init(width: side, height: side)
  init(float side) {
    if (side < 0)
      side = 0
    self.init(width: side, height: side)
    note((int)self.area())
  }

  init(int kind) {
    if (kind)
      self.init(width: 1, height: 2)
    else
      self->init(2.0f)
  }

  ~mutating float area(void) { return width * height }
} Rect

int use(void) {
  Rect r = Rect(3.0f)
  Rect s = Rect(1)
  return (int)(r.area() + s.area())
}
