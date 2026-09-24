// Expression completion in C walks every identifier in the table. With a
// precompiled header loaded, the walk can pull declarations in and add
// identifiers as it goes, which used to invalidate the iterator and abort the
// process -- the way clangd dies mid-session (M4.4).

// RUN: %clang -x cx-header -std=gnu17 -o %t.pch %S/Inputs/cx-counter.h
// RUN: %clang -x cx -std=gnu17 -include-pch %t.pch -fsyntax-only \
// RUN:   -Xclang -code-completion-at=%s:15:11 %s | FileCheck %s

#module Counters

int helper(int v);

int use(void) {
  int x = he
  return x;
}

// CHECK-DAG: COMPLETION: helper : [#int#]helper(<#int v#>)
// CHECK-DAG: COMPLETION: use : [#int#]use()
