// After an error, the parser skips to the end of the line, which stands in
// for the ';' Cx lets it leave out, instead of to the next ';' in the file.
// RUN: %clang -x cx -target arm64-apple-macosx -fsyntax-only -ferror-limit=0 -Xclang -verify %s

#module Recovery

typedef struct Size {
  int w
  init(int s) { w = s } // expected-note {{initializer declared here}}
} Size

void note(int)

void lines(void) {
  Size a = (Size){ 1 } // expected-error {{'Size' has an initializer, so it is constructed with 'Size(...)', not with braces}}
  int x = missing1 // expected-error {{use of undeclared identifier 'missing1'}}
  int y = ) // expected-error {{expected expression}}
  int z = missing2 + // expected-error {{use of undeclared identifier 'missing2'}}
    1
  note(missing3) // expected-error {{use of undeclared identifier 'missing3'}}
}

int after(void) {
  return missing4 // expected-error {{use of undeclared identifier 'missing4'}}
}
