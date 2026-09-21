// The generated surface requires every field, in declaration order, under its
// own name (M4).

// RUN: %clang -x cx -std=gnu17 -fsyntax-only -Xclang -verify %s

#module Shapes

typedef struct Size {
  int width;  // expected-note 2 {{field 'width' declared here}}
  int height; // expected-note 2 {{field 'height' declared here}}
} Size;

typedef union Choice { int a; } Choice;

void f(void) {
  Size a = Size(width: 1); // expected-error {{missing value for field 'height' in construction of 'Size' (aka 'struct Size'); it has no default}}

  // Whether the generated surface also accepts positional values is G02, so
  // for now the labels are required.
  Size b = Size(1, 2); // expected-error {{construction of 'Size' (aka 'struct Size') expects the field label 'width' here}} \
                       // expected-error {{construction of 'Size' (aka 'struct Size') expects the field label 'height' here}}

  // One clear error is enough for an out-of-order call.
  Size c = Size(height: 1, width: 2); // expected-error {{construction of 'Size' (aka 'struct Size') expects the field label 'width' here}}

  Size d = Size(width: 1, height: 2, extra: 3); // expected-error {{too many values in construction of 'Size' (aka 'struct Size')}}

  Choice e = Choice(a: 1); // expected-error {{generated construction is only available for a struct, not 'Choice' (aka 'union Choice')}}

  (void)a; (void)b; (void)c; (void)d; (void)e;
}
