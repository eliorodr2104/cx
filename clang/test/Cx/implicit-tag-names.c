// A tag name is usable as a type name on its own, wherever nothing in the
// ordinary namespace has that name and C could not read the code any other
// way. Every valid C program keeps its meaning; see implicit-tag-names-c.c.

// RUN: %clang -x cx -std=c89 -fsyntax-only -Xclang -verify=expected,pre23 %s
// RUN: %clang -x cx -std=gnu89 -fsyntax-only -Xclang -verify=expected,pre23 %s
// RUN: %clang -x cx -std=c99 -fsyntax-only -Xclang -verify=expected,pre23,c99 %s
// RUN: %clang -x cx -std=gnu17 -fsyntax-only -Xclang -verify=expected,pre23,c99 %s
// RUN: %clang -x cx -std=c23 -fsyntax-only -Xclang -verify=expected,c23 %s
// RUN: %clang -x cx -std=gnu23 -fsyntax-only -Xclang -verify=expected,c23 %s
// RUN: %clang -x cx -std=gnu17 -S -emit-llvm -o - %s -DCODEGEN | FileCheck %s
// The rule does not depend on anything the preprocessor removes.
// RUN: %clang -x cx -std=gnu17 -E %s -DCODEGEN -o %t.i
// RUN: %clang -x cx-cpp-output -std=gnu17 -S -emit-llvm -o - %t.i | FileCheck %s

#module Shapes

struct Size { int width; int height; }; // #size
union Bits { int i; float f; };
enum Mode { Fast, Slow };

/* Declarations: a tag name followed by a declarator. */
Size origin;
Size *cursor;
const Size fixed = { 1, 2 };
Size const also = { 3, 4 };
typedef Size Extent;
Bits raw;
Mode mode = Fast;
Size make(int w, int h);
void take(Size s, Size *p);
struct Holder { Size inner; Size *link; };

int use(void) {
  Size local = { 5, 6 };
  Size *p = &local;
  Mode m = Slow;
  /* Expressions: a tag name in parentheses. */
  unsigned long a = sizeof(Size) + sizeof(Size *) + _Alignof(Size);
  Size copy = (Size){ 7, 8 };
  void *v = (Size *)p;
  (void)v;
  return local.width + p->height + copy.width + (int)a + (int)m;
}

/* Construction with labels, and a type naming itself in its own methods. */
struct Point {
  int x;
  int y;
  ~mutating Point doubled() { Point p = Point(x: x * 2, y: y * 2); return p; }
};
int construct(void) { Point p = Point(x: 1, y: 2); return p.doubled().x; }

#ifndef CODEGEN
/* An ordinary declaration of the name wins, at any scope. */
struct Hidden { int a; };
int Hidden; // expected-note {{struct 'Hidden' is hidden by a non-type declaration of 'Hidden' here}}
void hidden(void) {
  Hidden x; // expected-error {{must use 'struct' tag to refer to type 'Hidden'}}
}

struct Inner { int a; };
void shadow(void) {
  int Inner = 1; // expected-note {{struct 'Inner' is hidden by a non-type declaration of 'Inner' here}}
  Inner y; // expected-error {{must use 'struct' tag to refer to type 'Inner'}}
}

/* An enumerator of the tag's own name wins over the tag. */
enum Color { Color, Red };
int color = Color;

/* Positions C can read keep their C meaning, or stay errors. */
void positions(void) {
  Size; // expected-error {{must use 'struct' tag to refer to type 'Size'}} expected-warning {{declaration does not declare anything}}
}

/* Until C23 an unlabelled `Tag(...)` is C's implicit function call, which a
   Cx module then rejects for lacking a prototype. From C23 it is construction. */
int call(void) {
  return Size(3); // pre23-error {{'Size' is declared in a Cx module, so it needs a prototype}} c99-error {{call to undeclared function 'Size'}} c23-error {{construction of 'Size' expects the field label 'width' here}} c23-error {{construction of 'Size' expects the field label 'height' here}}
}
// c23-note@#size {{field 'width' declared here}}
// c23-note@#size {{field 'height' declared here}}
#endif

// CHECK: @"_Z{{[0-9]+}}_Cx0$Shapes$origin" = global %struct.Size zeroinitializer
// CHECK: define {{.*}}@"_Z{{[0-9]+}}_Cx0$Shapes$construct
