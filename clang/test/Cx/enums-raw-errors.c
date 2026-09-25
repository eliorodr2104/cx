// RUN: %clang -x cx -target arm64-apple-macosx -fsyntax-only -ferror-limit=0 -Xclang -verify %s
#module Errs

enum { case a };            // expected-error {{a Cx enum needs a name}}
enum Mixed { m0, case m1 }; // expected-error {{'case' cannot appear in an enum whose first enumerator has none}}
enum Clause { case c0
  c1 }                      // expected-error {{each clause of a Cx enum starts with 'case'}}
enum Simple { case s0 = 3 } // expected-error {{a case of 'enum Simple' cannot have a value}}
enum Dup: int { case d0 = 1, d1 = 1 } // expected-error {{cases 'd0' and 'd1' of 'enum Dup' share the value 1}}
enum Tiny: unsigned char { case t0 = 300 } // expected-error {{enumerator value is not representable in the underlying type}}
enum After: int { case x0 y0 }        // expected-error {{expected ',', ';', a line break or '}' after a Cx enum case}}

enum Color { case red, green }
enum Color bad1(void) { return Color.blue; } // expected-error {{'enum Color' has no case named 'blue'}}
enum Color bad2(void) { return red; }        // expected-error {{'red' is a case of 'Color'; write '.red' where 'Color' is expected, or 'Color.red'}}

void ctx(void) {
  var v = .red;                          // expected-error {{the enum of '.red' is not known here; write 'Type.red'}}
  int n = .red;                          // expected-error {{the enum of '.red' is not known here}}
  // Brace elision: C gives `.red` to p.y, an int; the element is typed as the
  // field it would be without elision, so the mismatch is an error, never a
  // silent conversion.
  struct { struct { int x, y; } p; enum Color c; } o = { 1, .red }; // expected-error {{incompatible}}
}

enum Raw: unsigned char { case r0, r1 }
int printf(const char *, ...);
void rules(enum Raw r, enum Color c, int i) {
  (void)c.rawValue;          // expected-error {{'enum Color' has no 'rawValue', since it has no backing type}}
  (void)r.other;             // expected-error {{'enum Raw' has no member 'other'}}
  (void)(int)r;              // expected-error {{cannot cast 'enum Raw' to 'int'; a Cx enum does not convert to other types, use '.rawValue'}}
  (void)(enum Raw)i;         // expected-error {{cannot cast 'int' to 'enum Raw'}}
  (void)(int)c;              // expected-error {{cannot cast 'enum Color' to 'int'; a Cx enum does not convert to other types}}
  int j = r;                 // expected-error {{incompatible}}
  enum Raw k = 1;            // expected-error {{incompatible}}
  if (r) {}                  // expected-error {{a value of 'enum Raw' is not a condition}}
  (void)(r && i);            // expected-error {{a value of 'enum Raw' is not a condition}}
  (void)(r ? 1 : 2);         // expected-error {{a value of 'enum Raw' is not a condition}}
  (void)(r < Raw.r1);        // expected-error {{cases of 'enum Raw' have no order}}
  (void)(r + 1);             // expected-error {{invalid operands}}
  r++;                       // expected-error {{cannot increment}}
  printf("%d", r);           // expected-error {{a value of 'enum Raw' cannot be passed to '...'; pass '.rawValue'}}
  switch (r) { default: ; }  // pattern matching (M6d)
  int *p = &r;               // expected-error {{incompatible pointer types}}
}
