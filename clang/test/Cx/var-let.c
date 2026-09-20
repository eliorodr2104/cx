// Cx `var` and `let` inference (M1). Semicolons are still required.

// RUN: %clang -x cx -std=gnu17 -fsyntax-only -Xclang -verify %s
// RUN: %clang -x cx -std=c11 -fsyntax-only -Xclang -verify %s
// RUN: %clang -x cx -std=c23 -fsyntax-only -Xclang -verify %s
// RUN: %clang -x cx -std=gnu17 -Xclang -ast-dump -fsyntax-only %s | FileCheck %s

// Neither spelling is a declaration specifier in plain C.
// RUN: not %clang -x c -std=gnu17 -fsyntax-only %s 2>&1 \
// RUN:   | FileCheck --check-prefix=PLAINC %s
// PLAINC: unknown type name 'var'

// expected-no-diagnostics

int value = 10;
const int qualified = 3;
int table[4];
int callee(void);

// Each declarator deduces its own type; there is no shared inferred base type.
var count = 10,
    ratio = 10.0,
    scale = 10.0f,
    limit = 10L;
// CHECK: VarDecl {{.*}} count 'int'
// CHECK: VarDecl {{.*}} ratio 'double'
// CHECK: VarDecl {{.*}} scale 'float'
// CHECK: VarDecl {{.*}} limit 'long'

// `let` adds top-level const: the binding, not the pointee.
let pointer = &value;
// CHECK: VarDecl {{.*}} pointer 'int *const'
let immutable = 7;
// CHECK: VarDecl {{.*}} immutable 'const int'

void deduction(void) {
  // Top-level qualification of the initializer does not carry over.
  var unqualified = qualified;
  // CHECK: VarDecl {{.*}} unqualified 'int'

  // Pointee qualification is preserved.
  const int *pointee = &qualified;
  var preserved = pointee;
  // CHECK: VarDecl {{.*}} preserved 'const int *'

  // C array/function decay follows the selected C dialect.
  var decayed = table;
  // CHECK: VarDecl {{.*}} decayed 'int *'
  var fn = callee;
  // CHECK: VarDecl {{.*}} fn 'int (*)(void)'

  // A `var` binding stays mutable; a `let` pointee stays mutable.
  var counter = 0;
  counter = 1;
  let bound = &value;
  *bound = 20;

  (void)unqualified; (void)preserved; (void)decayed; (void)fn; (void)counter;
}
