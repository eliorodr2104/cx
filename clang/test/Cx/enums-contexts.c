// `.case` takes its enum from the member it initializes: struct fields in
// braces, positional or designated, nested lists, compound literals, unions,
// scalars in braces, and tuple literal elements.
// RUN: %clang -x cx -target arm64-apple-macosx -fsyntax-only -Xclang -verify %s
// RUN: %clang -x cx -target arm64-apple-macosx -S -emit-llvm -o - %s | FileCheck %s
// RUN: %clang -x cx -target arm64-apple-macosx -Xclang -ast-print -fsyntax-only %s > %t.print.c
// RUN: %clang -x cx -target arm64-apple-macosx -S -emit-llvm -o - %t.print.c | FileCheck %s
// expected-no-diagnostics

#module Ctx

enum Color { case red, green, blue }
enum Token { case integer(int) case end }
enum Perm: OptionSet { case read, write }
struct Pixel { int x; enum Color c; };
struct Frame { struct Pixel p[2]; Token t; enum Perm perm; };
union Either { enum Color c; int i; };

// CHECK-DAG: $Ctx$a" = global {{.*}} { i32 1, i8 1,
struct Pixel a = { 1, .green };
// CHECK-DAG: $Ctx$b" = global {{.*}} { i32 2, i8 2,
struct Pixel b = { .c = .blue, .x = 2 };
// After a designator, positions go on with the next field.
// CHECK-DAG: $Ctx$c" = global {{.*}} { i32 3, i8 0,
struct Pixel c = { .x = 3, .red };
struct Frame f = { { { 1, .red }, { .c = .green } }, .integer(4), [.read] };
union Either u = { .blue };
enum Color scalar = { .green };
// CHECK-DAG: $Ctx$t" = global {{.*}} { i32 1, i8 1,
(int, enum Color) t = (1, .green);
(int, (enum Color, enum Color)) nested = (1, (.red, .blue));

// A tuple field in braces takes an unlabelled tuple, which C cannot read
// there since no C type is a tuple.
struct Box { (int, int) corner; int n; };
// CHECK-DAG: $Ctx$box" = global {{.*}}{ i32 1, i32 2 }, i32 3 }
struct Box box = { (1, 2), 3 };

struct Pixel make(void) { return (struct Pixel){ 5, .blue }; }
(int, enum Color) pair(void) { return (2, .red); }
void takeTuple((int, enum Color) p);
void use(void) {
  (int, enum Color) v;
  v = (3, .blue);
  takeTuple((4, .green));
  struct Pixel local = { .c = .red };
  struct Frame frames[] = { { .t = .end, .perm = [] } };
}
