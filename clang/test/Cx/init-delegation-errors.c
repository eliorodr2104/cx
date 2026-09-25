// self.init is called exactly once on every path, before any use of self,
// and only in an initializer.
// RUN: %clang -x cx -target arm64-apple-macosx -fsyntax-only -Xclang -verify %s

#module DelegationErrors

void note(int)

typedef struct Rect {
  float width
  float height

  init(float width w, float height h) {
    width = w
    height = h
  }
  init(float side) {
    width = side // expected-error {{'self' is used before 'self.init' is called}}
    self.init(width: side, height: side)
  }
  init(int side) {
    note((int)self.area()) // expected-error {{'self' is used before 'self.init' is called}}
    self.init(width: side, height: side)
  }
  init(double side) {
    self.init(width: 1, height: 1)
    self.init(width: 2, height: 2) // expected-error {{'self.init' may already have been called here}}
  }
  init(long side) {
    if (side)
      self.init(width: 1, height: 1)
  } // expected-error {{initializer ends without calling 'self.init'}}
  init(short n) {
    while (n--)
      self.init(width: 1, height: 1) // expected-error {{'self.init' may already have been called here}}
  } // expected-error {{initializer ends without calling 'self.init'}}
  init(char c) {
    defer { self.init(width: 1, height: 1) } // expected-error {{'self.init' cannot be called in a deferred block}}
    self.init(width: 2, height: 2)
  }
  float area(void) {
    self.init(width: 1, height: 1) // expected-error {{'self.init' can only be called in an initializer}}
    return width * height
  }
} Rect
