// C enums, including C23 fixed-underlying ones, keep their meaning under Cx.
// RUN: %clang -x cx -std=c23 -fsyntax-only -Xclang -verify %s
// RUN: %clang -x c -std=c23 -fsyntax-only -Xclang -verify=c %s
// expected-no-diagnostics
typedef unsigned char OptionSet;
enum Code : unsigned char { OK = 0, FAIL };
enum Plain { A, B };
enum Flags : OptionSet { F0 };
int use(void) { return OK + A + F0 + (enum Code)1 + (int)FAIL; }
enum Plain plain = 1;
#ifndef __CX__
enum E { case x }; // c-error {{expected identifier}}
#endif
