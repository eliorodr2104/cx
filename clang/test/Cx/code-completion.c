// Completion has to agree with the language: a Cx method is a candidate, an
// inaccessible member is not, and a labelled parameter is offered under the
// label the call has to write (M4.3).

#module Shapes

typedef struct Box {
  int shown;
  private int hidden;

  init(int shown s) { shown = s; }
  void grow(int by n) { shown += n; }
  ~mutating int get(void) { return self.shown; }
} Box;

int outside(Box *b) {
  return b->shown;
}

// From outside the type: the methods are offered, the private field is not,
// the receiver is not part of the signature, and `by:` is shown.
// RUN: %clang -x cx -std=gnu17 -fsyntax-only \
// RUN:   -Xclang -code-completion-at=%s:17:13 %s \
// RUN:   | FileCheck --check-prefix=OUTSIDE %s
// OUTSIDE-DAG: COMPLETION: get : [#int#]get()
// OUTSIDE-DAG: COMPLETION: grow : [#void#]grow(<#by: int n#>)
// OUTSIDE-DAG: COMPLETION: shown : [#int#]shown

// `init` is reached through `Box(...)`, never as a member.
// RUN: %clang -x cx -std=gnu17 -fsyntax-only \
// RUN:   -Xclang -code-completion-at=%s:17:13 %s \
// RUN:   | FileCheck --check-prefix=NOTOFFERED %s
// NOTOFFERED-NOT: COMPLETION: hidden
// NOTOFFERED-NOT: COMPLETION: init

// From inside the type the private field is a candidate, through `self->`
// and through the `self.` shorthand alike.
// RUN: %clang -x cx -std=gnu17 -fsyntax-only \
// RUN:   -Xclang -code-completion-at=%s:13:41 %s \
// RUN:   | FileCheck --check-prefix=INSIDE %s
// INSIDE-DAG: COMPLETION: hidden : [#int#]hidden
// INSIDE-DAG: COMPLETION: shown : [#int#]shown
// INSIDE-DAG: COMPLETION: grow : [#void#]grow(<#by: int n#>)
