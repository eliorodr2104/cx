// A default reads only fields that hold a value when it is applied.
// RUN: %clang -x cx -target arm64-apple-macosx -fsyntax-only -ferror-limit=0 -Xclang -verify %s

#module ReadsErrors

typedef struct Loop {
  int a = a + 1 // expected-error {{the default of 'a' reads 'a' itself}}
  int b = c // expected-error {{use of undeclared identifier 'c'}}
  int c
} Loop

typedef struct Late {
  int width // expected-note {{give 'width' a default, or compute 'height' in the initializer}}
  int height = width * 2 // expected-error {{the default of 'height' reads 'width', which the initializer sets only after the defaults are applied}}
  init(int w) { width = w }
} Late

typedef struct Cells {
  int n
  int cells[2] = { n, n } // expected-error {{the default of array field 'cells' cannot read other fields}}
} Cells

typedef struct Size {
  int width
  int height = width * 2
} Size

Size global = Size(width: 1) // expected-error {{construction of 'Size' (aka 'struct Size') computes a default from another field, so it cannot appear outside a function}}

void braces(void) {
  Size s = { 3 } // expected-error {{field 'height' has a default computed from other fields, which a braced list cannot apply; construct the value with 'Size(...)' or give the field}}
  Size t = { 3, 6 }
}
