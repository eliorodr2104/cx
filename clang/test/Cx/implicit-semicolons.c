// A ';' may be left out where C requires one, when the next token is on a new
// line, is a closing brace, or is the end of the file. Wherever C can read
// the tokens as a continuation it does, so no valid C changes meaning; see
// implicit-semicolons-c.c. -Wcx-implicit-semicolon reports each one.

// RUN: %clang -x cx -std=gnu17 -fsyntax-only -Xclang -verify %s
// RUN: %clang -x cx -std=gnu17 -fsyntax-only -Wcx-implicit-semicolon -Xclang -verify=expected,warn %s
// RUN: %clang -x cx -std=c23 -fsyntax-only -Wcx-implicit-semicolon -Xclang -verify=expected,warn %s
// RUN: %clang -x cx -std=gnu17 -S -emit-llvm -o - %s -DCODEGEN | FileCheck %s
// Preprocessed output keeps the line breaks that end statements.
// RUN: %clang -x cx -std=gnu17 -E %s -DCODEGEN -o %t.i
// RUN: %clang -x cx-cpp-output -std=gnu17 -S -emit-llvm -o - %t.i | FileCheck %s

#module Semis

int compute(void);
void effect(int v) // warn-warning {{';' implied by the line break}}
_Noreturn void stop(void);

struct Point {
  int x // warn-warning {{';' implied by the line break}}
  int y // warn-warning {{';' implied by the line break}}
  int sum() { return x + y } // warn-warning {{';' implied by the closing brace}}
} // warn-warning {{';' implied by the line break}}

typedef struct Point Pt // warn-warning {{';' implied by the line break}}
typedef struct Frame { int count; } Frame // warn-warning {{';' implied by the line break}}
Frame firstFrame // warn-warning {{';' implied by the line break}}
Pt *lastPoint // warn-warning {{';' implied by the line break}}
struct Holder {
  int kind // warn-warning {{';' implied by the line break}}
  union { int i; float f; } // warn-warning {{';' implied by the line break}}
  struct { int lo; int hi; } // warn-warning {{';' implied by the line break}}
} // warn-warning {{';' implied by the line break}}
enum Mode { Fast, Slow } // warn-warning {{';' implied by the line break}}

int statements(int n) {
  var count = 0 // warn-warning {{';' implied by the line break}}
  let limit = 10 // warn-warning {{';' implied by the line break}}
  int a = 1, // A declaration list continues across the line.
      b = 2 // warn-warning {{';' implied by the line break}}
  for (int i = 0; i < n; i++) {
    if (i == limit)
      break // warn-warning {{';' implied by the line break}}
    else
      count += a + b // warn-warning {{';' implied by the line break}}
  }
  do count-- // warn-warning {{';' implied by the line break}}
  while (count > 100) // warn-warning {{';' implied by the line break}}
  if (count)
    effect(count) // warn-warning {{';' implied by the line break}}
  else
    effect(0) // warn-warning {{';' implied by the line break}}
  goto done // warn-warning {{';' implied by the line break}}
done:
  count = ({ int t = count; t }) // warn-warning {{';' implied by the closing brace}} warn-warning {{';' implied by the line break}}
  { count++ } // warn-warning {{';' implied by the closing brace}}
  return count // warn-warning {{';' implied by the line break}}
}

/* C reads these across the line break, so they are one statement each. */
int continued(int v) {
  effect
  (v) // warn-warning {{';' implied by the line break}}
  v = v
    - 1 // warn-warning {{';' implied by the line break}}
  return
    compute() // warn-warning {{';' implied by the line break}}
}

/* A closing brace never starts a returned value. */
void bare(int v) {
  if (v) { return } // warn-warning {{';' implied by the closing brace}}
  return // warn-warning {{';' implied by the line break}}
}

#ifndef CODEGEN
/* `break` and `continue` take a label in C2y, so a name on the next line
   continues them; that reading wins, as every C reading does. */
void named(int n) {
  while (n) {
    break
    n = 0; // expected-error {{named 'break' is only supported in C2y}} expected-error {{'break' label does not name an enclosing loop or 'switch'}} expected-error {{expected expression}}
  }
}

/* Only a boundary stands in for a ';'. */
int same_line(int v) {
  v = 1 v = 2; // expected-error {{expected ';' after expression}}
  return v;
}

/* A for header keeps its separators. */
void header(int n) {
  for (int i = 0
       i < n; i++) {} // expected-error {{expected ';' in 'for' statement specifier}}
}
#endif

// CHECK-LABEL: define {{.*}}continued
// CHECK: call void @"_Z{{[0-9]+}}_Cx0$Semis$effecti"(
// CHECK: sub nsw i32 {{.*}}, 1
// CHECK: [[R:%.*]] = call i32 @"_Z{{[0-9]+}}_Cx0$Semis$computev"()
// CHECK: ret i32 [[R]]

int last = 1 // warn-warning {{';' implied by the end of file}}
