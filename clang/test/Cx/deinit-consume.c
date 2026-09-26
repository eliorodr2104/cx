// A resource local or by-value parameter is consumed by passing it by value
// or returning it; a pointer borrows. A consumed variable is destroyed at the
// end of its scope only when it still holds its value.
// RUN: %clang -x cx -target arm64-apple-macosx -fsyntax-only -Xclang -verify %s
// RUN: %clang -x cx -std=gnu89 -target arm64-apple-macosx -fsyntax-only -Xclang -verify %s
// RUN: %clang -x cx -target arm64-apple-macosx -S -emit-llvm -o - %s | FileCheck %s
// Preprocessed output and -ast-print compile to the same code.
// RUN: %clang -x cx -target arm64-apple-macosx -E %s -o %t.i
// RUN: %clang -x cx-cpp-output -target arm64-apple-macosx -S -emit-llvm -o - %t.i | FileCheck %s
// RUN: %clang -x cx -target arm64-apple-macosx -Xclang -ast-print -fsyntax-only %s > %t.print.c
// RUN: %clang -x cx -target arm64-apple-macosx -S -emit-llvm -o - %t.print.c | FileCheck %s
// expected-no-diagnostics

#module Consume

void note(int)

typedef struct File {
  int fd
  init(int f) { fd = f }
  deinit() { note(fd) }
} File

typedef struct Owner { File file; int n; } Owner

void show(const File *f) { note(f->fd) }
void reopen(File *f) { *f = File(f->fd + 1) }

// The callee owns a by-value parameter and destroys it.
// CHECK-LABEL: define {{.*}}close
// CHECK: call void @"{{.*}}File{{.*}}deinit
void close(File f) { note(f.fd) }

// Returning a local moves it: it lives in the caller's slot, and its flag
// says it is no longer this function's to destroy.
// CHECK-LABEL: define {{.*}}open
// CHECK: store i1 true, ptr %cx.alive
// CHECK: store i1 false, ptr %cx.alive
// CHECK: ret i32
File open(int fd) {
  File f = File(fd)
  show(&f)
  return f
}

// Passed on, a parameter is not destroyed by this function.
// CHECK-LABEL: define {{.*}}forward
// CHECK: call {{.*}}close
// CHECK: cx.destroy:
File forward(File f, int keep) {
  if (keep)
    return f
  close(f)
  return File(0)
}

// Consumed on one path only: destroyed at the end only if still held.
// CHECK-LABEL: define {{.*}}maybe
// CHECK: %cx.alive = alloca i1
// CHECK: store i1 true, ptr %cx.alive
// CHECK: store i1 false, ptr %cx.alive
// CHECK: br i1 {{.*}}, label %cx.destroy, label %cx.destroy.done
void maybe(int n) {
  File f = File(n)
  if (n)
    close(f)
}

// Assigned after it is consumed, a variable holds a value again.
void again(void) {
  File f = open(1)
  close(f)
  f = File(2)
  reopen(&f)
  Owner o = Owner(file: f, n: 1)
  note(o.n)
}

// A loop declares its variable anew each iteration.
void loop(int n) {
  while (n--) {
    File f = File(n)
    close(f)
  }
}
