// RUN: %clang -x cx -target arm64-apple-macosx -fsyntax-only -ferror-limit=0 -Xclang -verify %s
#module Errs

enum Bad: int { case a(int) }  // expected-error {{'enum Bad' has payloads, so it cannot have a backing type}}
enum Rec { case node(Rec) }    // expected-error {{a payload of 'enum Rec' cannot hold 'enum Rec' itself}}
enum Voidy { case v(void) }    // expected-error {{a tuple element cannot have type 'void'}}

enum Token { case integer(int value) case location(int line, int column) case end }
void uses(Token t, int i) {
  Token a = .integer;          // expected-error {{case 'integer' of 'enum Token' takes a payload: write '.integer(...)'}}
  Token b = .end();            // expected-error {{case 'end' of 'enum Token' has no payload, so it takes no parentheses}}
  Token c = .location(1);      // expected-error {{case 'location' of 'enum Token' takes 2 values, but 1 was given}}
  Token d = .integer(1, 2);    // expected-error {{case 'integer' of 'enum Token' takes 1 value, but 2 were given}}
  Token e = .integer(count: 1); // expected-error {{the payload of case 'integer' is labelled 'value', but the argument is labelled 'count'}}
  Token f = .location(column: 1, line: 2); // expected-error {{tuple element 0 is labelled 'column', but '(int line, int column)' (aka '(int, int)') labels it 'line'}} expected-error {{tuple element 1 is labelled 'line'}}
  (void)(t == .end);           // expected-error {{values of 'enum Token' cannot be compared with '=='; match them with switch}}
  (void)t.rawValue;            // expected-error {{'enum Token' has no 'rawValue'}}
  (void)t.$tag;                // expected-error {{'enum Token' has no member '$tag'}}
  switch (t) { default: ; }    // expected-error {{a switch on 'enum Token' needs Cx pattern matching}}
  Token g = i;                 // expected-error {{incompatible}}
}
