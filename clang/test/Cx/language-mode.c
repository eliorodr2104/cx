// Cx is the selected C dialect plus Cx capability: -std= keeps choosing a C
// standard, and only Cx mode defines the availability macro (M0).

// RUN: %clang -x cx -dM -E %s | FileCheck --check-prefix=CX %s
// RUN: %clang -x c -dM -E %s | FileCheck --check-prefix=C %s
// RUN: %clang -x cx -std=c11 -dM -E %s | FileCheck --check-prefix=CX11 %s
// RUN: %clang -x cx -std=gnu23 -dM -E %s | FileCheck --check-prefix=CX23 %s

// A C++ standard is not a Cx standard.
// RUN: not %clang -x cx -std=c++17 -fsyntax-only %s 2>&1 \
// RUN:   | FileCheck --check-prefix=BADSTD %s

// The cc1 -x value round-trips through CompilerInvocation.
// RUN: %clang -x cx -fsyntax-only -Xclang -round-trip-args %s

// CX-DAG: #define __CX__ 1
// CX-DAG: #define __STDC_VERSION__ 201710L
// C-NOT: #define __CX__
// CX11-DAG: #define __CX__ 1
// CX11-DAG: #define __STDC_VERSION__ 201112L
// CX23-DAG: #define __CX__ 1
// CX23-DAG: #define __STDC_VERSION__ 202311L
// BADSTD: invalid argument '-std=c++17' not allowed with 'Cx'

int main(void) { return 0; }
