// Cx raw and simple enums: a body that uses `case` selects a scoped Cx enum.
// RUN: %clang -x cx -target arm64-apple-macosx -fsyntax-only -Xclang -verify %s
// RUN: %clang -x cx -std=c23 -target arm64-apple-macosx -fsyntax-only -Xclang -verify %s
// RUN: %clang -x cx -target arm64-apple-macosx -S -emit-llvm -o - %s | FileCheck %s
// Preprocessed output and -ast-print compile to the same code.
// RUN: %clang -x cx -target arm64-apple-macosx -E %s -o %t.i
// RUN: %clang -x cx-cpp-output -target arm64-apple-macosx -S -emit-llvm -o - %t.i | FileCheck %s
// -ast-print spells _Static_assert as C23's static_assert, so its output is
// compiled as C23.
// RUN: %clang -x cx -target arm64-apple-macosx -Xclang -ast-print -fsyntax-only %s > %t.print.c
// RUN: %clang -x cx -std=c23 -target arm64-apple-macosx -S -emit-llvm -o - %t.print.c | FileCheck %s
// expected-no-diagnostics

#module Net

enum Status: unsigned short {
  case ok = 200, created
  case notFound = 404
}
enum Direction { case north, south, east, west }
// Earlier cases are visible unqualified inside the body.
enum Step: int { case first = 1; case second = first + 1 }

_Static_assert(sizeof(enum Status) == 2, "the backing type");
_Static_assert(sizeof(enum Direction) == 1, "the smallest unsigned integer");
_Static_assert(_Generic(Status.created, enum Status: 1, default: 0), "case type");
_Static_assert(Status.created == Status.created, "equality");

// Qualified cases; an ordinary variable of the enum's name keeps C's reading.
// CHECK-LABEL: define {{.*}} @"_Z{{[0-9]+}}_Cx0$Net$pickv"
// CHECK: ret i8 3
enum Direction pick(void) { return Direction.west; }
typedef enum Status Status;
// CHECK-LABEL: define {{.*}} @"_Z{{[0-9]+}}_Cx0$Net$finev"
// CHECK: ret i16 200
Status fine(void) { return Status.ok; }
int shadow(void) {
  struct { int west; } Direction = { 3 };
  return Direction.west;
}

// `.case` takes its enum from the expected type.
void takeDir(enum Direction d);
// CHECK-LABEL: define {{.*}} @"_Z{{[0-9]+}}_Cx0$Net$turn
// CHECK: call void @"_Z{{[0-9]+}}_Cx0$Net$takeDir{{[^"]*}}"(i8 {{.*}}2)
enum Direction turn(enum Direction d) {
  enum Direction next = .north
  next = .south
  takeDir(.east)
  if (d == .west) return .north
  return next
}
enum Direction pickOne(int flag) { return flag ? .north : .south }
enum Direction compass[] = { .north, .east, .south, .west };
_Static_assert(sizeof(compass) == 4, "four one-byte cases");
struct Point { int x, y; };
struct Point origin = { .x = 0, .y = 0 };

// rawValue is the only way to an integer.
// CHECK-LABEL: define {{.*}} @"_Z{{[0-9]+}}_Cx0$Net$code
// CHECK: ret i16 %{{.*}}
unsigned short code(Status s) { return s.rawValue; }
// CHECK-LABEL: define {{.*}} @"_Z{{[0-9]+}}_Cx0$Net$notFoundv"
// CHECK: ret i32 404
int notFound(void) { return Status.notFound.rawValue; }
