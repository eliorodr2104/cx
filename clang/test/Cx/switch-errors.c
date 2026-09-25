// RUN: %clang -x cx -target arm64-apple-macosx -fsyntax-only -ferror-limit=0 -Xclang -verify %s
#module Errs
enum Direction { case north, south, east, west }
enum Color { case red, green }
enum Token { case integer(int) case location(int line, int column) case end }

void missing(enum Direction d) {
  switch (d) { // expected-error {{switch on 'enum Direction' does not handle 'east', 'west'; add them or a 'default'}}
    case .north: ;
    case .south: ;
  }
}
void rules(enum Direction d, Token t) {
  switch (d) {
    case .north: ;          // expected-note {{handled here}}
    case .north: ;          // expected-error {{case 'north' is already handled}}
    case Color.red: ;       // expected-error {{'red' is not a case of 'enum Direction'}}
    case 1: ;               // expected-error {{a case of a Cx switch is written '.name' or 'Type.name'}}
    case .south:            // expected-error {{this case has no body; list the cases in one label to share a body}}
    case .east: ;
    case .west: { case .north: ; } // expected-error {{a case of a Cx switch must be at the top of the switch body}}
    default: ;              // expected-warning {{'default' is never reached: every case of 'enum Direction' is handled}}
  }
  switch (t) {
    case .end(x): ;         // expected-error {{case 'end' has no payload to bind}}
    case .location(a): ;    // expected-error {{case 'location' has 2 payload elements, but 1 name is bound}}
    default: ;
  }
  switch (t) {
    case .integer(v), .end: ; // expected-error {{a label with several cases cannot bind payloads}}
    default: ;
  }
  switch (t) {
    case .integer(v):       // expected-note {{variable 'v' declared const here}}
      v = 2;                // expected-error {{cannot assign to variable 'v' with const-qualified type}}
    default: ;
  }
}

// A clause is entered only through its case, which binds its payload.
void jumps(Token t, int n) {
  if (n)
    goto inside // expected-error {{cannot jump from this goto statement to its label}}
  switch (t) {
    case .integer(v): // expected-note {{jump enters a clause of a Cx switch}}
    inside:
      n = v
    case .location(line, column):
      if (line)
        goto again // expected-error {{cannot jump from this goto statement to its label}}
    case .end: // expected-note {{jump enters a clause of a Cx switch}}
    again:
      n = 0
      goto done
  }
done:
  // A clause may jump within itself, and a C switch keeps its fallthrough.
  switch (t) {
    case .end:
    retry:
      if (n--)
        goto retry
    default: ;
  }
  switch (n) {
    case 0: goto one;
    case 1: one: n = 2;
  }
}
