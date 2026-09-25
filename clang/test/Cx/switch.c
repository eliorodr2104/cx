// A switch on a Cx enum matches its cases: no fallthrough, payload bindings,
// and every case handled or a default.
// RUN: %clang -x cx -target arm64-apple-macosx -fsyntax-only -Xclang -verify %s
// RUN: %clang -x cx -std=gnu89 -target arm64-apple-macosx -fsyntax-only -Xclang -verify %s
// RUN: %clang -x cx -target arm64-apple-macosx -S -emit-llvm -o - %s | FileCheck %s
// Preprocessed output and -ast-print compile to the same code.
// RUN: %clang -x cx -target arm64-apple-macosx -E %s -o %t.i
// RUN: %clang -x cx-cpp-output -target arm64-apple-macosx -S -emit-llvm -o - %t.i | FileCheck %s
// RUN: %clang -x cx -target arm64-apple-macosx -Xclang -ast-print -fsyntax-only %s > %t.print.c
// RUN: %clang -x cx -target arm64-apple-macosx -S -emit-llvm -o - %t.print.c | FileCheck %s
// expected-no-diagnostics

#module Match

enum Direction { case north, south, east, west }
enum Status: unsigned short { case ok = 200, notFound = 404 }
enum Token {
  case integer(int value)
  case location(int line, int column)
  case end
}

// CHECK-LABEL: define {{.*}}@"_Z{{[0-9]+}}_Cx0$Match$axis
// CHECK: switch i32
int axis(enum Direction d) {
  switch (d) {
    case .north, .south:
      return 1
    case .east:
      return 2
    case Direction.west:
      return 3
  }
}

// Raw enums switch on their values; no break is needed, and a case does not
// fall into the next one.
// CHECK-LABEL: define {{.*}}@"_Z{{[0-9]+}}_Cx0$Match$code
// CHECK: switch i32 %{{.*}}, label %{{.*}} [
// CHECK-NEXT: i32 200, label
// CHECK-NEXT: i32 404, label
int code(Status s) {
  int n = 0
  switch (s) {
    case .ok:
      n += 1
    case .notFound:
      n += 10
  }
  return n
}

// Bindings are const copies of the payload elements; `_` discards one, and a
// case can ignore its payload. Without a default, an invalid value traps.
// CHECK-LABEL: define {{.*}}@"_Z{{[0-9]+}}_Cx0$Match$weigh
// CHECK: call void @llvm.trap()
int weigh(Token t) {
  switch (t) {
    case .integer(v):
      return v
    case .location(line, _):
      int twice = line * 2
      return twice
    case .end:
      return 0
  }
}
int any(Token t) {
  switch (t) {
    case .location:
      return 1
    default:
      return 0
  }
}

// break leaves the switch, continue its loop; switches nest.
int loop(Token *ts, int n) {
  int sum = 0
  int i
  for (i = 0; i < n; ++i) {
    switch (ts[i]) {
      case .end:
        break
      case .integer(v):
        if (v < 0) { continue }
        sum += v
      case .location(l, c):
        switch (l > c ? Direction.north : Direction.south) {
          case .north: sum += l
          default: sum += c
        }
    }
  }
  return sum
}

// A C switch keeps fallthrough.
// CHECK-LABEL: define {{.*}}@"_Z{{[0-9]+}}_Cx0$Match$legacyi"
int legacy(int x) {
  int r = 0;
  switch (x) { case 1: r += 1; case 2: r += 2; break; default: r = 9; }
  return r;
}
