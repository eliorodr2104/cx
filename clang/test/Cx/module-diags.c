// Rejections for Cx `#module` (M2).

// RUN: %clang -x cx -std=gnu17 -fsyntax-only -Xclang -verify %s

// `#module` is not a directive in plain C.
// RUN: not %clang -x c -std=gnu17 -fsyntax-only %s 2>&1 \
// RUN:   | FileCheck --check-prefix=PLAINC %s
// PLAINC: invalid preprocessing directive

#module Owner // expected-note {{previous module declaration is here}}

#module Owner // expected-error {{this file already declares a module owner}}

#if 0
// Repeats are rejected whether or not the names match.
#endif

int declared(void);
