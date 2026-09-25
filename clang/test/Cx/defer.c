// A Cx defer runs its block when the enclosing scope is left: in reverse
// registration order, per loop iteration, after the return value is computed.
// RUN: %clang -x cx -target arm64-apple-macosx -fsyntax-only -Xclang -verify %s
// RUN: %clang -x cx -std=gnu89 -target arm64-apple-macosx -fsyntax-only -Xclang -verify %s
// RUN: %clang -x cx -target arm64-apple-macosx -S -emit-llvm -o - %s | FileCheck %s
// Preprocessed output and -ast-print compile to the same code.
// RUN: %clang -x cx -target arm64-apple-macosx -E %s -o %t.i
// RUN: %clang -x cx-cpp-output -target arm64-apple-macosx -S -emit-llvm -o - %t.i | FileCheck %s
// RUN: %clang -x cx -target arm64-apple-macosx -Xclang -ast-print -fsyntax-only %s > %t.print.c
// RUN: %clang -x cx -target arm64-apple-macosx -S -emit-llvm -o - %t.print.c | FileCheck %s
// RUN: FileCheck --check-prefix=PRINT %s < %t.print.c
// expected-no-diagnostics

void note(int)
int next(void)

enum Step { case open, read(int count), close }

// CHECK-LABEL: define {{.*}}void @order(
// CHECK: call void @note(i32 {{.*}}3)
// CHECK: call void @note(i32 {{.*}}2)
// CHECK: call void @note(i32 {{.*}}1)
// CHECK: ret void
// PRINT-LABEL: void order(
// PRINT: defer {
void order(void) {
  defer { note(1) }
  defer { note(2) }
  note(3)
}

// Each iteration runs its own defer before the next one starts.
// CHECK-LABEL: define {{.*}}void @loop(
// CHECK: for.body:
// CHECK: call void @note(i32 {{.*}}%
// CHECK: call void @note(i32 {{.*}}0)
// CHECK: for.inc:
void loop(int n) {
  int i
  for (i = 0; i < n; i++) {
    if (i > 8) break
    defer { note(0) }
    note(i)
  }
}

// The value is read before the defer overwrites its source.
// CHECK-LABEL: define {{.*}}i32 @value(
// CHECK: [[V:%.*]] = load i32, ptr %x
// CHECK: store i32 5, ptr %
// CHECK: ret i32 [[V]]
int value(void) {
  var x = 4
  let p = &x
  defer { *p = 5 }
  return x
}

// Leaving early runs only the defers already reached.
// CHECK-LABEL: define {{.*}}void @early(
// CHECK: call void @note(i32 {{.*}}1)
// CHECK-NOT: call void @note(i32 {{.*}}2)
// CHECK: ret void
void early(int stop) {
  defer { note(1) }
  if (stop) return
  defer { note(2) }
}

// A clause of a Cx switch is its own scope: the defer runs before leaving it.
// CHECK-LABEL: define {{.*}}void @steps(
// CHECK: call void @note(i32 {{.*}}7)
// CHECK: call void @note(i32 {{.*}}8)
void steps(Step s) {
  switch (s) {
    case .read(count):
      defer { note(8) }
      note(7)
    case .open, .close:
      note(9)
  }
}

// A goto out of the scope runs its defer.
// CHECK-LABEL: define {{.*}}void @jump(
// CHECK: call void @note(i32 {{.*}}4)
// CHECK: call void @note(i32 {{.*}}5)
void jump(void) {
  {
    defer { note(5) }
    note(4)
    goto done
  }
done:
  return
}

// Anywhere but before a block, `defer` is an ordinary name.
// CHECK-LABEL: define {{.*}}i32 @name(
int name(void) {
  int defer = next()
  defer = defer + 1
  defer++
  return defer
}
