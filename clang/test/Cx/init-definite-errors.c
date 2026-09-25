// An initializer that leaves a field uninitialized on some path, uses a
// field or self too early, or assigns a const field twice is an error, and
// a type with an initializer is not built with braces.
// RUN: %clang -x cx -target arm64-apple-macosx -fsyntax-only -ferror-limit=0 -Xclang -verify %s

#module DefiniteErrors

void note(int)
void touch(int *p)
int ready(void)

typedef struct Point { int x; int y; } Point

typedef struct Pair {
  int a
  int b // expected-note 4 {{field declared here}}
  init(int x) {
    a = x
    if (x) {
      return // expected-error {{field 'b' is not initialized when the initializer returns}}
    }
    b = x
  }
  init(float x) {
    a = 1
    if (x > 0)
      b = 2
  } // expected-error {{field 'b' is not initialized when the initializer ends}}
  init(int x, int n) {
    a = x
    while (n--)
      b = n
  } // expected-error {{field 'b' is not initialized when the initializer ends}}
  init(double x) {
    a = b // expected-error {{field 'b' is used before it is initialized}}
    b = 1
  }
  init(char c) {
    a = 1
    touch(&b) // expected-error {{field 'b' is used before it is initialized}}
    b = 2
  }
  init(long v) {
    a = 1
    note(self.sum()) // expected-error {{'self' is used before field 'b' is initialized}}
    b = 2
  }
  init(short v) {
    a = 1
    Pair copy = *self // expected-error {{'self' is used before field 'b' is initialized}}
    b = copy.a
  }
  init(unsigned v) {
    defer { note(b) } // expected-error {{field 'b' is used before it is initialized}}
    a = 1
    b = 2
  }
  init(unsigned char v) {
    a = 1
    defer { b = 3 } // expected-error {{a deferred block cannot initialize field 'b'}}
  } // expected-error {{field 'b' is not initialized when the initializer ends}}
  int sum(void) { return a + b }
} Pair

typedef struct Shape {
  Point origin
  int values[3] // expected-note 2 {{an array field is initialized by a default}}
  init(int x) {
    origin.x = x // expected-error {{field 'origin' is used before it is initialized}}
    origin = (Point){ x, x }
    values[0] = 1 // expected-error {{field 'values' is used before it is initialized}}
  } // expected-error {{field 'values' is not initialized when the initializer ends}}
  init(float x) {
    origin = (Point){ 0, 0 }
  } // expected-error {{field 'values' is not initialized when the initializer ends}}
} Shape

typedef struct Token {
  const int id // expected-note {{field declared here}}
  init(int x) {
    id = x
    id = x + 1 // expected-error {{const field 'id' may already be initialized here}}
  }
  init(int x, int n) {
    while (n--)
      id = n // expected-error {{const field 'id' may already be initialized here}}
  } // expected-error {{field 'id' is not initialized when the initializer ends}}
  init(float x) {
    id = 1
    id += 1 // expected-error {{const field 'id' may already be initialized here}}
  }
  init(double x) {
    id = 1
    defer { id = 2 } // expected-error {{a deferred block cannot assign const field 'id'}}
  }
} Token

typedef struct Value {
  int kind
  union { // expected-note {{anonymous union declared here}}
    int i
    float f
  }
  init(int v) {
    kind = v
  } // expected-error {{no member of the anonymous union is initialized when the initializer ends}}
  init(float v) {
    kind = 1
    note(self.get()) // expected-error {{'self' is used before its anonymous union is initialized}}
    f = v
  }
  int get(void) { return i }
} Value

typedef struct Size {
  int w
  int h
  init(int side) { // expected-note 4 {{initializer declared here}}
    w = side
    h = side
  }
} Size

typedef struct Frame { Size size; int count; } Frame

// Static storage and left-out members are zero, as in C.
Size global
static Frame framed = { .count = 1 }

void braces(void) {
  Size a = { 1, 2 } // expected-error {{'Size' has an initializer, so it is constructed with 'Size(...)', not with braces}}
  Frame f = { { 1, 2 }, 3 } // expected-error {{'Size' has an initializer, so it is constructed with 'Size(...)', not with braces}}
  Frame e = { 1, 2, 3 } // expected-error {{'Size' has an initializer, so it is constructed with 'Size(...)', not with braces}}
  Frame g = { Size(2), 1 }
  Frame h = { .count = 1 }
  static Size zero
  Size later
  later = Size(5)
  Size copy = later
  note(g.count + h.count + zero.w + copy.w + global.w + framed.count + e.count)
  // Recovery from an error in an initializer skips to the next ';'.
  Size b = (Size){ 3, 4 }; // expected-error {{'Size' has an initializer, so it is constructed with 'Size(...)', not with braces}}
}
