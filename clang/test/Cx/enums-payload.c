// Cx payload enums: a case may carry a payload, and a value holds one case
// with only that case's payload alive.
// RUN: %clang -x cx -target arm64-apple-macosx -fsyntax-only -Xclang -verify %s
// RUN: %clang -x cx -std=gnu89 -target arm64-apple-macosx -fsyntax-only -Xclang -verify %s
// RUN: %clang -x cx -target arm64-apple-macosx -S -emit-llvm -o - %s | FileCheck %s
// Preprocessed output and -ast-print compile to the same code.
// RUN: %clang -x cx -target arm64-apple-macosx -E %s -o %t.i
// RUN: %clang -x cx-cpp-output -target arm64-apple-macosx -S -emit-llvm -o - %t.i | FileCheck %s
// RUN: %clang -x cx -target arm64-apple-macosx -Xclang -ast-print -fsyntax-only %s > %t.print.c
// RUN: %clang -x cx -std=c23 -target arm64-apple-macosx -S -emit-llvm -o - %t.print.c | FileCheck %s
// expected-no-diagnostics

#module Lex

enum Token {
  case integer(int)
  case location(int line, int column)
  case next(Token *)
  case end
}

// The one-byte tag precedes the union of payloads, aligned for its pointer.
_Static_assert(sizeof(enum Token) == 16, "tag and union");
_Static_assert(sizeof(Token) == sizeof(enum Token), "one type");
typedef enum Token Tok;
_Static_assert(sizeof(Tok) == 16, "typedef");

// CHECK-LABEL: define {{.*}} @"_Z{{[0-9]+}}_Cx0$Lex$numberv"
// CHECK: store i8 0,
// CHECK: store i32 7,
Token number(void) { return .integer(7) }
// CHECK-LABEL: define {{.*}} @"_Z{{[0-9]+}}_Cx0$Lex$atii"
// CHECK: store i8 1,
Token at(int l, int c) { return Token.location(line: l, column: c) }
Token positional(void) { return .location(3, 4) }
// Elements convert as initialization would.
Token converted(void) { return .location(3.0, 'a') }
// CHECK-LABEL: define {{.*}} @"_Z{{[0-9]+}}_Cx0$Lex$finishv"
// CHECK: store i8 3,
Token finish(void) { return .end }

void take(Token t);
Token chain(Token *rest, int flag) {
  Token t = .next(rest)
  t = .end
  take(.integer(1))
  return flag ? .end : Tok.integer(2)
}

// A zero-initialized value holds the first case with a zeroed payload.
Token zero;
Token table[] = { .integer(1), .end };
_Static_assert(sizeof(table) == 32, "two values");
