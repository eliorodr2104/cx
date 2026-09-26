// A field default may read an earlier field: one with a default of its own,
// or, when the type has no custom initializer, one construction is given.
// Such defaults are applied in declaration order, after the others.
// RUN: %clang -x cx -target arm64-apple-macosx -fsyntax-only -Xclang -verify %s
// RUN: %clang -x cx -std=gnu89 -target arm64-apple-macosx -fsyntax-only -Xclang -verify %s
// RUN: %clang -x cx -target arm64-apple-macosx -S -emit-llvm -o - %s | FileCheck %s
// Preprocessed output and -ast-print compile to the same code.
// RUN: %clang -x cx -target arm64-apple-macosx -E %s -o %t.i
// RUN: %clang -x cx-cpp-output -target arm64-apple-macosx -S -emit-llvm -o - %t.i | FileCheck %s
// RUN: %clang -x cx -target arm64-apple-macosx -Xclang -ast-print -fsyntax-only %s > %t.print.c
// RUN: %clang -x cx -target arm64-apple-macosx -S -emit-llvm -o - %t.print.c | FileCheck %s
// RUN: FileCheck --check-prefix=PRINT %s < %t.print.c
// expected-no-diagnostics

#module Reads

int next(void)

// Generated construction: every earlier field has a value.
typedef struct Size {
  int width
  int height = width * 2
  const int area = width * height
} Size

// A custom initializer: only earlier defaulted fields.
typedef struct Grid {
  int cols = 4
  int rows = cols + 1
  int cells
  init(int extra) { cells = rows * cols + extra }
} Grid

// PRINT: int height = width * 2;
// CHECK-LABEL: define {{.*}}sizes
// CHECK: %[[W:.*]] = load i32, ptr %width
// CHECK: %[[H:.*]] = mul nsw i32 %[[W]], 2
// CHECK: store i32 %[[H]], ptr %height
// CHECK: store i32 {{.*}}, ptr %area
// PRINT: Size a = Size(width: 3);
int sizes(void) {
  Size a = Size(width: 3)
  Size b = Size(width: 2, height: 5)
  return a.area + b.area
}

// The given value is evaluated once.
// CHECK-LABEL: define {{.*}}once
// CHECK: call {{.*}}next
// CHECK-NOT: call {{.*}}next
// CHECK: ret
int once(void) {
  Size s = Size(width: next())
  return s.height
}

// CHECK-LABEL: define {{.*}}grids
// CHECK: load i32, ptr %cols
// CHECK: store i32 {{.*}}, ptr %rows
// CHECK: call {{.*}}Grid{{.*}}init
int grids(void) {
  Grid g = Grid(1)
  return g.cells
}

