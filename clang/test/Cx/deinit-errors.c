// A resource value has one owner: it is never copied, lives only where it is
// destroyed, and its deinit is called explicitly only through a pointer.
// RUN: %clang -x cx -target arm64-apple-macosx -fsyntax-only -ferror-limit=0 -Xclang -verify %s

#module DeinitErrors

void note(int)
void take(int, ...)

typedef struct File {
  int fd
  init(int f) { fd = f }
  deinit() { note(fd) }
  void close(void) {
    self->deinit() // expected-error {{'self' cannot be destroyed inside its own method}}
  }
} File


union Holds { File f; int x; } // expected-error {{a union member cannot have type 'File' (aka 'struct File'), which has a deinit}}
typedef struct Anon {
  int kind
  union { File f; int x; } // expected-error {{a union member cannot have type 'File' (aka 'struct File'), which has a deinit}}
} Anon

typedef struct Plain { int x; } Plain
typedef struct Owner { File file; int count; } Owner

File open(int f) { return File(f) }

extern File global // expected-error {{a variable with static storage cannot hold 'File' (aka 'struct File'), which has a deinit}}
void param(File f) {} // expected-error {{a parameter cannot hold 'File' (aka 'struct File'), which has a deinit}}
typedef (File, int) Pair // expected-error {{a tuple element cannot hold 'File' (aka 'struct File'), which has a deinit}}
enum Slot { case some(File f), none } // expected-error {{an enum payload cannot hold 'File' (aka 'struct File'), which has a deinit}}

File copies(File *p) {
  File a = File(1)
  File b = a // expected-error {{'File' (aka 'struct File') has a deinit, so it cannot be copied}}
  b = a // expected-error {{'File' (aka 'struct File') has a deinit, so it cannot be copied}}
  File c = *p // expected-error {{'File' (aka 'struct File') has a deinit, so it cannot be copied}}
  File d = (b = File(2)) // expected-error {{'File' (aka 'struct File') has a deinit, so it cannot be copied}}
  Owner o = Owner(file: a, count: 1) // expected-error 2 {{'File' (aka 'struct File') has a deinit, so it cannot be copied}}
  return a // expected-error {{'File' (aka 'struct File') has a deinit, so it cannot be copied}}
}

void storage(void) {
  static File s = File(1) // expected-error {{a variable with static storage cannot hold 'File' (aka 'struct File'), which has a deinit}}
  File later // expected-error {{a variable without an initializer cannot hold 'File' (aka 'struct File'), which has a deinit}}
  take(1, open(2)) // expected-error {{a variadic argument cannot hold 'File' (aka 'struct File'), which has a deinit}}
  note(open(3).fd) // expected-error {{reading a field of a new 'File' (aka 'struct File') would never run its deinit; store it in a variable first}}
}

void leftOut(void) {
  Owner o = { .count = 1 } // expected-error {{field 'file' must be given explicitly, since 'File' (aka 'struct File') has a deinit}}
  File files[2] = { File(1) } // expected-error {{every element must be given explicitly, since 'File' (aka 'struct File') has a deinit}}
}

void explicit(File *p, Plain *q) {
  File f = File(1)
  f.deinit() // expected-error {{a deinit is called only through a pointer; a variable is destroyed automatically}}
  q->deinit() // expected-error {{'Plain' (aka 'struct Plain') has no deinit}}
  p->deinit()
}

void jumps(int n) {
  goto past // expected-error {{cannot jump from this goto statement to its label}}
  File f = File(1) // expected-note {{jump bypasses initialization of a variable with a deinit}}
past:
  note(n)
}

typedef struct Twice {
  File file
  init(int n) {
    file = File(n)
    file = File(n + 1) // expected-error {{resource field 'file' may already be initialized here}}
  }
  init(float x) {
    file = File(1)
    defer { file = File(2) } // expected-error {{a deferred block cannot assign resource field 'file'}}
  }
} Twice

union Bad4 { int x; deinit() {} } // expected-error {{a union cannot declare a deinit}}
typedef struct Bad3 { int x; ~mutating deinit() {} } Bad3 // expected-error {{a deinit destroys its receiver, so it cannot be '~mutating'}}
struct Bad2 {
  int x
  int deinit() { return 0 } // expected-error {{a deinit takes no parameters and declares no return type}}
};
struct Bad1 {
  int x
  deinit(int y) {} // expected-error {{a deinit takes no parameters and declares no return type}}
};

