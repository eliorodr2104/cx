// A tuple type made in a PCH is the same type in the file that uses it.
// RUN: rm -rf %t && mkdir -p %t
// RUN: %clang_cc1 -x cx-header -emit-pch -o %t/tuple.pch %S/Inputs/cx-tuple.h
// RUN: %clang_cc1 -x cx -include-pch %t/tuple.pch -fsyntax-only -verify %s
// RUN: %clang_cc1 -x cx -include %S/Inputs/cx-tuple.h -fsyntax-only -verify %s
// expected-no-diagnostics

#module Pairs

void use(void) {
  (int, float) local = make(3);
  shared = local;
  shared = make(4);
  float s = make(5).score;
  (int id, float score) again = shared;
  again.id = (int)s;
}
