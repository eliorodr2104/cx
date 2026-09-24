// `_` spells an unlabeled position, and `$`, which separates the parts of a
// Cx symbol, cannot appear in a label.

// RUN: %clang -x cx -std=gnu17 -fsyntax-only -Xclang -verify %s
// RUN: %clang -x cx -std=gnu17 -S -emit-llvm -o - %s -DNO_ERRORS | FileCheck %s

#module Labels

void move(int _ v, int mode m) {} // expected-note {{previous definition is here}}
void call(void) { move(1, mode: 2); }
// CHECK: define {{.*}}@"_Z{{[0-9]+}}_Cx0$Labels$move$_:mode:ii"(

#ifndef NO_ERRORS
// The same labels, so the same entity: this redefines `move`.
void move(int v, int mode m) {} // expected-error {{redefinition of 'move'}}

void dollar(int a$b v); // expected-error {{an argument label cannot contain '$'}}
#endif
