// A Cx defer takes a block, cannot be left early from inside, and cannot be
// entered or skipped by a jump.
// RUN: %clang -x cx -target arm64-apple-macosx -fsyntax-only -Xclang -verify %s

#module DeferErrors

typedef int jmp_buf[48]
int setjmp(jmp_buf env)
void longjmp(jmp_buf env, int val)
void note(int)
void release(int *p)

void block(int *p) {
  defer release(p) // expected-error {{expected '{' after 'defer'}}
  defer if (p) release(p) // expected-error {{expected '{' after 'defer'}}
}

int exits(int n) {
  defer {
    return 1 // expected-error {{cannot return from a defer statement}}
  }
  while (n) {
    defer {
      break // expected-error {{cannot break out of a defer statement}}
    }
    defer {
      continue // expected-error {{cannot continue loop outside of enclosing defer statement}}
    }
  }
  // Loops and switches inside the block may leave themselves.
  defer {
    while (n) { break }
    for (;;) { continue }
  }
  return 0
}

void jumps(int n) {
  goto inside // expected-error {{cannot jump from this goto statement to its label}}
  defer { // expected-note {{jump enters a defer statement}}
  inside:
    note(1)
  }

  goto past // expected-error {{cannot jump from this goto statement to its label}}
  defer { note(2) } // expected-note {{jump bypasses defer statement}}
past:

  {
  again:
    defer { // expected-note {{jump exits a defer statement}}
      goto again // expected-error {{cannot jump from this goto statement to its label}}
    }
  }
}

void cases(int n) {
  switch (n) {
    case 1:
      defer { note(1) } // expected-note {{jump bypasses defer statement}}
    case 2: // expected-error {{cannot jump from switch statement to this case label}}
      note(2)
  }
}

jmp_buf env
void nonlocal(void) {
  defer {
    setjmp(env) // expected-error {{cannot use 'setjmp' inside a defer statement}}
    longjmp(env, 1) // expected-error {{cannot use 'longjmp' inside a defer statement}}
  }
}
