// A module name is part of every owned symbol, where `$` separates it from the
// entity's name, so a module name is an identifier without `$`.

// RUN: not %clang_cc1 -x cx -fcx-module='a b' -fsyntax-only %s 2>&1 \
// RUN:   | FileCheck --check-prefix=SPACE %s
// RUN: not %clang_cc1 -x cx -fcx-module=1x -fsyntax-only %s 2>&1 \
// RUN:   | FileCheck --check-prefix=DIGIT %s
// RUN: not %clang_cc1 -x cx -fcx-module='A$B' -fsyntax-only %s 2>&1 \
// RUN:   | FileCheck --check-prefix=DOLLAR %s
// RUN: %clang -x cx -std=gnu17 -fsyntax-only -Xclang -verify %s -DDIRECTIVE
// RUN: %clang -x cx -std=gnu17 -fsyntax-only -Xclang -verify %s -DPROTOTYPE
// RUN: not %clang -x cx -std=gnu17 -fsyntax-only %s -DPROTOTYPE \
// RUN:   -fdiagnostics-parseable-fixits 2>&1 | FileCheck --check-prefix=FIXIT %s

// SPACE: error: invalid Cx module name 'a b': a module name is an identifier without '$'
// DIGIT: error: invalid Cx module name '1x'
// DOLLAR: error: invalid Cx module name 'A$B'

#ifdef DIRECTIVE
#module A$B // expected-error {{invalid Cx module name 'A$B'}}
#endif
#ifdef PROTOTYPE
#module Shapes
// Every function in a module has a Cx symbol, which needs a prototype.
int knr(); // expected-error {{'knr' is declared in a Cx module, so it needs a prototype}}
// FIXIT: fix-it:"{{.*}}":{[[@LINE-1]]:9-[[@LINE-1]]:9}:"void"
#endif
int plain(void);
