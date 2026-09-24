// -ast-print writes Cx back as Cx: labels, access, `~mutating`, method calls
// on their receiver, and the primary file's module, so the output compiles to
// the same symbols. An AST file keeps its ownership when dumped.

// RUN: rm -rf %t && mkdir -p %t
// RUN: %clang_cc1 -x cx -ast-print %s -o %t/printed.c
// RUN: FileCheck --input-file=%t/printed.c %s
// RUN: %clang_cc1 -x cx -emit-llvm -o %t/direct.ll %s
// RUN: %clang_cc1 -x cx -emit-llvm -o %t/printed.ll %t/printed.c
// RUN: grep -o '@"[^"]*"' %t/direct.ll | sort -u > %t/direct.sym
// RUN: grep -o '@"[^"]*"' %t/printed.ll | sort -u > %t/printed.sym
// RUN: diff %t/direct.sym %t/printed.sym

// RUN: %clang -x cx -emit-ast -o %t/file.ast %s
// RUN: %clang_cc1 -x ast -ast-dump-all %t/file.ast | FileCheck --check-prefix=DUMP %s

#module Printing

typedef struct Counter {
  private(set) int value;
  private int secret;
  ~mutating int get() { return value; }
  void add(int by n) { value += n; }
} Counter;

int move(int x v) { return v; }

int use(Counter *c) {
  c->add(by: 2);
  return c->get() + move(x: 1);
}

// CHECK: #module Printing
// CHECK: private(set) int value;
// CHECK: private int secret;
// CHECK: ~mutating int get(void) {
// CHECK: void add(int by n) {
// CHECK: int move(int x v) {
// CHECK: c->add(by: 2);
// CHECK: return c->get() + move(x: 1);

// DUMP: FunctionDecl {{.*}} imported cx-module Printing {{.*}}use 'int (Counter *)'
