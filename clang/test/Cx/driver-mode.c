// Driver input-kind selection for the Cx language mode (M0).
//
// clangx defaults ordinary C sources to Cx; an explicit -x wins; plain clang is
// unaffected; and non-C inputs are never reclassified.

// RUN: %clang -### -c -x cx %s 2>&1 | FileCheck --check-prefix=CX %s
// RUN: %clang --driver-mode=cx -### -c %s 2>&1 | FileCheck --check-prefix=CX %s
// RUN: %clang --driver-mode=cx -### -c -x c %s 2>&1 | FileCheck --check-prefix=C %s
// RUN: %clang -### -c %s 2>&1 | FileCheck --check-prefix=C %s

// A -x none reset returns to extension-based inference, which is Cx here.
// RUN: %clang --driver-mode=cx -### -c -x c %s -x none %s 2>&1 \
// RUN:   | FileCheck --check-prefix=RESET %s

// Headers become Cx headers, so a Cx PCH can be built with clangx.
// RUN: %clang --driver-mode=cx -### -c %S/Inputs/cx-pch.h 2>&1 \
// RUN:   | FileCheck --check-prefix=CXHEADER %s

// Assembly and object inputs keep their own kinds.
// RUN: %clang --driver-mode=cx -### -c %S/Inputs/empty.s 2>&1 \
// RUN:   | FileCheck --check-prefix=ASM %s

// An unsupported driver-mode value is still rejected.
// RUN: not %clang --driver-mode=cxx -### -c %s 2>&1 \
// RUN:   | FileCheck --check-prefix=BADMODE %s

// CX: "-cc1"
// CX: "-x" "cx"
// C: "-cc1"
// C: "-x" "c"
// C-NOT: "-x" "cx"
// RESET: "-x" "c"
// RESET: "-x" "cx"
// CXHEADER: "-x" "cx-header"
// ASM: "-x" "assembler-with-cpp"
// ASM-NOT: "-x" "cx"
// BADMODE: unsupported argument 'cxx' to option '--driver-mode='

int main(void) { return 0; }
