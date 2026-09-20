// The Cx mode is semantic state, not an ignorable option: it is recorded in a
// precompiled header and a mismatched consumer is rejected (M0).

// RUN: rm -rf %t && mkdir -p %t
// RUN: %clang -x cx-header %S/Inputs/cx-pch.h -o %t/cx.pch
// RUN: %clang -x cx -include-pch %t/cx.pch -fsyntax-only %s
// RUN: not %clang -x c -include-pch %t/cx.pch -fsyntax-only %s 2>&1 \
// RUN:   | FileCheck --check-prefix=MISMATCH %s

// RUN: %clang -x c-header %S/Inputs/cx-pch.h -o %t/c.pch
// RUN: %clang -x c -include-pch %t/c.pch -fsyntax-only %s
// RUN: not %clang -x cx -include-pch %t/c.pch -fsyntax-only %s 2>&1 \
// RUN:   | FileCheck --check-prefix=MISMATCH %s

// MISMATCH: error: Cx language mode

int main(void) { return from_pch(); }
