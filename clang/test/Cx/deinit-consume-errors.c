// A consumed variable is not used on any path until it is assigned again.
// RUN: %clang -x cx -target arm64-apple-macosx -fsyntax-only -ferror-limit=0 -Xclang -verify %s

#module ConsumeErrors

void note(int)

typedef struct File {
  int fd
  init(int f) { fd = f }
  deinit() { note(fd) }
} File

typedef struct Owner { File file; int n; } Owner

void close(File f) {}
void show(const File *f) {}

void after(void) {
  File f = File(1)
  close(f) // expected-note {{consumed here}}
  show(&f) // expected-error {{'f' is used after it is consumed}}
}

void twice(void) {
  File f = File(1)
  close(f) // expected-note {{consumed here}}
  close(f) // expected-error {{'f' is used after it is consumed}}
}

void branch(int n) {
  File f = File(1)
  if (n)
    close(f) // expected-note {{consumed here}}
  note(f.fd) // expected-error {{'f' is used after it is consumed}}
}

void loop(int n) {
  File f = File(1)
  while (n--)
    close(f) // expected-error {{'f' is used after it is consumed}} expected-note {{consumed here}}
}

void deferred(void) {
  File f = File(1)
  defer { show(&f) } // expected-note {{used by this deferred block}}
  close(f) // expected-error {{'f' cannot be consumed, since a deferred block uses it}}
}

void inDefer(void) {
  File f = File(1)
  defer { close(f) } // expected-error {{a deferred block cannot consume 'f'}}
}

void notOwned(File *p, Owner *o) {
  close(*p) // expected-error {{'File' (aka 'struct File') has a deinit, so it cannot be copied}}
  close(o->file) // expected-error {{'File' (aka 'struct File') has a deinit, so it cannot be copied}}
  File g = File(1)
  File h = g // expected-error {{'File' (aka 'struct File') has a deinit, so it cannot be copied}}
}
