// An initializer initializes every field on every path before it leaves;
// fields may take braced defaults, and a const field is initialized by its
// first assignment.
// RUN: %clang -x cx -target arm64-apple-macosx -fsyntax-only -Xclang -verify %s
// RUN: %clang -x cx -std=gnu89 -target arm64-apple-macosx -fsyntax-only -Xclang -verify %s
// RUN: %clang -x cx -target arm64-apple-macosx -S -emit-llvm -o - %s | FileCheck %s
// Preprocessed output and -ast-print compile to the same code.
// RUN: %clang -x cx -target arm64-apple-macosx -E %s -o %t.i
// RUN: %clang -x cx-cpp-output -target arm64-apple-macosx -S -emit-llvm -o - %t.i | FileCheck %s
// RUN: %clang -x cx -target arm64-apple-macosx -Xclang -ast-print -fsyntax-only %s > %t.print.c
// RUN: %clang -x cx -target arm64-apple-macosx -S -emit-llvm -o - %t.print.c | FileCheck %s
// expected-no-diagnostics

#module Definite

void note(int)
_Noreturn void fail(void)

enum Mode { case read, write }

typedef struct Point { int x; int y; } Point

// Arrays and aggregates are initialized by their defaults.
typedef struct Buffer {
  int counts[4] = {1, 2, 3, 4}
  char name[8] = {}
  Point origin = {5, 6}
  int size
  init(int n) {
    size = n
    counts[0] = n
  }
} Buffer

// CHECK-LABEL: define {{.*}}braced
// CHECK: store i32 4, ptr
// CHECK: store i32 5, ptr
int braced(void) {
  Buffer b = Buffer(7)
  return b.counts[3] + b.origin.y + b.size
}

typedef struct Rect {
  float width
  float height

  // Both branches initialize both fields; the early return has them too.
  init(float side) {
    if (side > 10) {
      width = 10
      height = 10
      return
    }
    if (side > 0) {
      width = side
    } else {
      width = 1
    }
    height = width
  }

  // A switch that covers every case initializes the field on every path.
  init(Mode mode) {
    switch (mode) {
      case .read:
        width = 1
      case .write:
        width = 2
    }
    height = 0
    note((int)self.area())
  }

  // A path that cannot return needs nothing.
  init(int code) {
    if (code < 0)
      fail()
    width = code
    height = code
  }

  // A field assigned before a loop may be reassigned inside it.
  init(float w, int times) {
    width = w
    height = 0
    int i
    for (i = 0; i < times; i++)
      height = height + w
  }

  ~mutating float area(void) { return width * height }
} Rect

typedef struct Handle {
  const int id
  int fd
  int log[4] = {}
  init(int n) {
    fd = n
    defer { note(fd) }
    if (n > 0)
      id = n
    else
      id = 0
    log[0] = id
    defer { fd = -1 }
  }
} Handle

// Anonymous members: a struct needs all its members, a union any one.
typedef struct Value {
  int kind
  union {
    int i
    float f
    struct { int lo; int hi; }
  }
  init(int v) {
    kind = 0
    i = v
    note((int)f)
  }
  init(int lo0, int hi0) {
    kind = 1
    lo = lo0
    hi = hi0
  }
} Value

// CHECK-LABEL: define {{.*}}use
// CHECK: call {{.*}}$init
int use(void) {
  Rect r = Rect(3.0f)
  Rect m = Rect(Mode.write)
  Handle h = Handle(2)
  Value v = Value(1, 2)
  return (int)r.area() + (int)m.width + h.id + v.hi
}
