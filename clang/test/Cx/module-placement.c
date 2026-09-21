// `#module` names the file's module, so it must precede the file's
// declarations. The signal comes from the parser, which is what makes the
// rule transparent to a header guard (M4.2).

// RUN: %clang -x cx -std=gnu17 -fsyntax-only -Xclang -verify %s

int before; // expected-note {{first declaration in this file is here}}

#module Late // expected-error {{'#module' must precede every declaration in its file}}

int after;
