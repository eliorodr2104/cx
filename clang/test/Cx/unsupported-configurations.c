// What Cx cannot represent yet is rejected rather than compiled to colliding
// symbols or merged entities: the Microsoft ABI, and Clang modules.

// RUN: not %clang_cc1 -x cx -triple x86_64-pc-windows-msvc -fsyntax-only %s 2>&1 \
// RUN:   | FileCheck --check-prefix=MSVC %s
// RUN: not %clang_cc1 -x cx -triple aarch64-pc-windows-msvc -fsyntax-only %s 2>&1 \
// RUN:   | FileCheck --check-prefix=MSVC %s
// RUN: %clang_cc1 -x cx -triple x86_64-pc-windows-gnu -fsyntax-only %s
// RUN: %clang_cc1 -x c -triple x86_64-pc-windows-msvc -fsyntax-only %s
// RUN: not %clang_cc1 -x cx -fmodules -fsyntax-only %s 2>&1 \
// RUN:   | FileCheck --check-prefix=MODULES %s

// MSVC: error: target '{{.*}}-windows-msvc' is not supported in Cx mode yet: the Microsoft ABI has no encoding for Cx module and argument-label identity
// MODULES: error: Clang modules ('-fmodules') are not supported in Cx mode yet

int plain(void);
