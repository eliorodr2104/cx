// The module name of a PCH is readable back whatever the PCH declares; an
// enum in a module-owned header once left it without an identifier ID.
// RUN: rm -rf %t && mkdir -p %t
// RUN: printf '#module Shared\nenum Legacy { L0, L1 };\n' > %t/legacy.h
// RUN: %clang_cc1 -x cx-header -emit-pch -o %t/legacy.pch %t/legacy.h
// RUN: %clang_cc1 -x cx -include-pch %t/legacy.pch -fsyntax-only -verify %s
// expected-no-diagnostics

#module Shared

int use(void) { return L1; }
