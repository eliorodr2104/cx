// The Cx driver reads C as Cx, from stdin as from a file.

// RUN: echo '__CX__' | %clang --driver-mode=cx -E - | FileCheck --check-prefix=CX %s
// RUN: echo '__CX__' | %clang -E - | FileCheck --check-prefix=C %s

// CX: {{^}}1{{$}}
// C: {{^}}__CX__{{$}}
