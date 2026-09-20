// Cx contextual `null` literal (M1). It reuses Clang's C23 nullptr semantics in
// every supported C dialect.

// RUN: %clang -x cx -std=gnu17 -fsyntax-only -Xclang -verify %s
// RUN: %clang -x cx -std=c11 -fsyntax-only -Xclang -verify %s
// RUN: %clang -x cx -std=c23 -fsyntax-only -Xclang -verify %s
// RUN: %clang -x cx -std=c11 -Xclang -ast-dump -fsyntax-only %s 2>/dev/null \
// RUN:   | FileCheck %s

// `null` is not a literal in plain C.
// RUN: not %clang -x c -std=c11 -fsyntax-only %s 2>&1 \
// RUN:   | FileCheck --check-prefix=PLAINC %s
// PLAINC: use of undeclared identifier 'null'

// expected-no-diagnostics

typedef struct Node Node;
struct Node { Node *next; };

Node *head = null;
// CHECK: VarDecl {{.*}} head 'Node *'
// CHECK-NEXT: ImplicitCastExpr
// CHECK-NEXT: CXXNullPtrLiteralExpr

void takes(int *p);

int uses(Node *node) {
  int *p = null;
  takes(null);
  if (node == null || p == null)
    return 1;
  p = null;
  return node->next != null;
}

// Legacy C spellings are untouched.
int *legacy = 0;
void *also = (void *)0;
