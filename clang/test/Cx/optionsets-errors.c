// RUN: %clang -x cx -target arm64-apple-macosx -fsyntax-only -ferror-limit=0 -Xclang -verify %s
#module Errs

enum Valued: OptionSet { case a = 4 }       // expected-error {{a case of option set 'enum Valued' is its own bit, so it cannot have a value}}
enum Carrying: OptionSet { case a(int) }    // expected-error {{a case of option set 'enum Carrying' cannot have a payload}}
#define C8(p) p##0, p##1, p##2, p##3, p##4, p##5, p##6, p##7
enum Big: OptionSet { case C8(a), C8(b), C8(c), C8(d), C8(e), C8(f), C8(g), C8(h), z } // expected-error {{option set 'enum Big' has 65 cases, but at most 64 fit in its bits}}

enum Permission: OptionSet { case read, write, execute }
enum Style: OptionSet { case bold, italic }
void rules(enum Permission p, enum Style s, int i) {
  (void)(p + p);                // expected-error {{'+' does not apply to option set 'enum Permission'; use '|', '&', '^', '-' or '~'}}
  (void)(p << 1);               // expected-error {{'<<' does not apply to option set 'enum Permission'}}
  (void)(p | s);                // expected-error {{cannot combine 'enum Permission' with 'enum Style'; both must be the same option set}}
  (void)(p & i);                // expected-error {{cannot combine 'enum Permission' with 'int'}}
  (void)(p < p);                // expected-error {{cases of 'enum Permission' have no order}}
  if (p & .read) {}             // expected-error {{a value of 'enum Permission' is not a condition}}
  enum Permission q = 3;        // expected-error {{incompatible}}
  (void)[.read];                // expected-error {{the option set of this literal is not known here}}
  (void)p.contains(of: .read);  // expected-error {{'contains' takes one option set argument}}
  (void)p.isSubset(.read);      // expected-error {{'isSubset' takes one option set argument, labelled 'of:'}}
  switch (p) { default: ; }     // expected-error {{an option set is not a closed set of states}}
}
