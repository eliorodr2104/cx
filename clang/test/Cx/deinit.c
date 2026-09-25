// A type with a deinit owns a resource: its locals, fields and array
// elements are destroyed when they end, a replaced or discarded value is
// destroyed at once, and p->init / p->deinit manage raw storage.
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

#module Res

void note(int)
void *malloc(unsigned long)
void free(void *)

typedef struct File {
  int fd
  init(int f) { fd = f }
  deinit() { note(fd) }
} File

// The body runs first, then the fields, last declared first.
typedef struct Pair {
  File first
  File second
  init(int a, int b) {
    first = File(a)
    second = File(b)
  }
  deinit() { note(0) }
} Pair

// No deinit of its own: it gets one that destroys its field.
typedef struct Holder {
  File file
  int count
} Holder

File open(int f) { return File(f) }

// Locals end last first, sharing the cleanup order of defer.
// CHECK-LABEL: define {{.*}}scopes
// CHECK: call void @"{{.*}}note{{.*}}"(i32 {{.*}}9)
// CHECK: call void @"{{.*}}Holder{{.*}}deinit{{.*}}"(ptr {{.*}}%h)
// CHECK: call void @"{{.*}}note{{.*}}"(i32 {{.*}}7)
// CHECK: call void @"{{.*}}File{{.*}}deinit{{.*}}"(ptr {{.*}}%a)
void scopes(void) {
  File a = File(1)
  defer { note(7) }
  Holder h = Holder(file: File(2), count: 3)
  note(9)
}

// The implicit deinit is emitted wherever it is used, like a method.
// CHECK-LABEL: define linkonce_odr {{.*}}"_Z{{.*}}Holder{{.*}}deinit
// CHECK: call void @"{{.*}}File{{.*}}deinit{{.*}}"(ptr {{.*}}%file)

// CHECK-LABEL: define {{.*}}pairs
// CHECK-LABEL: define linkonce_odr {{.*}}"_Z{{.*}}Pair{{.*}}deinit
// CHECK: call void @"{{.*}}note{{.*}}"(i32 {{.*}}0)
// CHECK: call void @"{{.*}}File{{.*}}deinit{{.*}}"(ptr {{.*}}%second)
// CHECK: call void @"{{.*}}File{{.*}}deinit{{.*}}"(ptr {{.*}}%first)
void pairs(void) {
  Pair p = Pair(1, 2)
}

// The new value is built before the old one is destroyed.
// CHECK-LABEL: define {{.*}}replace
// CHECK: %cx.new = alloca
// CHECK: call void @"{{.*}}File{{.*}}init{{.*}}"(ptr {{.*}}, i32 {{.*}}2)
// CHECK: call void @"{{.*}}File{{.*}}deinit{{.*}}"(ptr {{.*}}%f)
void replace(void) {
  File f = File(1)
  f = File(2)
  f = open(3)
}

// A discarded value is destroyed at once.
// CHECK-LABEL: define {{.*}}discard
// CHECK: %cx.discarded = alloca
// CHECK: call {{.*}}open
// CHECK: call void @"{{.*}}File{{.*}}deinit{{.*}}"(ptr {{.*}}%cx.discarded)
void discard(void) {
  (void)open(1)
  open(2)
}

// An array is destroyed from its last element to its first.
// CHECK-LABEL: define {{.*}}arrays
// CHECK: arraydestroy.body:
// CHECK: call void @"{{.*}}File{{.*}}deinit
void arrays(void) {
  File files[2] = { File(1), open(2) }
  files[1] = File(3)
}

// Leaving early destroys what was constructed so far.
// CHECK-LABEL: define {{.*}}early
// CHECK: call void @"{{.*}}File{{.*}}deinit
int early(int stop) {
  File a = File(1)
  if (stop)
    return 1
  File b = File(2)
  return b.fd
}

// Raw storage is constructed and destroyed explicitly.
// CHECK-LABEL: define {{.*}}heap
// CHECK: call void @"{{.*}}File{{.*}}init{{.*}}"(ptr {{.*}}, i32 {{.*}}5)
// CHECK: call void @"{{.*}}File{{.*}}deinit
// CHECK: call void @{{.*}}free
// PRINT: p->init(5)
// PRINT: (p + 1)->init(6)
// PRINT: p->deinit()
void heap(void) {
  File *p = malloc(2 * sizeof(File))
  p->init(5)
  (p + 1)->init(6)
  Holder *h = malloc(sizeof(Holder))
  h->init(file: File(7), count: 1)
  h->deinit()
  (p + 1)->deinit()
  p->deinit()
  free(h)
  free(p)
}
