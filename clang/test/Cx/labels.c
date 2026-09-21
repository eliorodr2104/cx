// Cx argument labels (M3). A labeled parameter is a complete C parameter
// declarator followed by the local name: the declarator's name is the external
// label, the extra identifier is the name visible inside the body.

// RUN: %clang -x cx -std=gnu17 -fsyntax-only -Xclang -verify %s
// RUN: %clang -x cx -std=gnu17 -Xclang -ast-dump -fsyntax-only %s \
// RUN:   | FileCheck %s

// The syntax is not valid C, so no C program changes meaning.
// RUN: not %clang -x c -std=gnu17 -fsyntax-only %s 2>&1 \
// RUN:   | FileCheck --check-prefix=PLAINC %s
// PLAINC: expected ')'

// expected-no-diagnostics

#module UI

void applySize(int w, int h);

void resize(int width newWidth, int height newHeight) {
  // The local name is what the body sees.
  applySize(newWidth, newHeight);
}
// CHECK: ParmVarDecl {{.*}} newWidth 'int'
// CHECK-NEXT: CxArgumentLabelAttr {{.*}} width
// CHECK: ParmVarDecl {{.*}} newHeight 'int'
// CHECK-NEXT: CxArgumentLabelAttr {{.*}} height

// The label attaches after the complete declarator, not after a guessed type.
void install(void (*callback)(int) handler) { handler(0); }
// CHECK: ParmVarDecl {{.*}} handler 'void (*)(int)'
// CHECK-NEXT: CxArgumentLabelAttr {{.*}} callback

// An ordinary one-name parameter stays positional. Omitting a label in the
// declaration does not create one at the call site.
void positional(int width, int height);

void handler_fn(int);

void use(void) {
  resize(width: 800, height: 600);
  positional(800, 600);
  install(callback: handler_fn);
}
