// Cx option sets: each case is an independent bit, and a value is a set of
// them, with set literals and set algebra.
// RUN: %clang -x cx -target arm64-apple-macosx -fsyntax-only -Xclang -verify %s
// RUN: %clang -x cx -std=gnu89 -target arm64-apple-macosx -fsyntax-only -Xclang -verify %s
// RUN: %clang -x cx -target arm64-apple-macosx -S -emit-llvm -o - %s | FileCheck %s
// Preprocessed output and -ast-print compile to the same code.
// RUN: %clang -x cx -target arm64-apple-macosx -E %s -o %t.i
// RUN: %clang -x cx-cpp-output -target arm64-apple-macosx -S -emit-llvm -o - %t.i | FileCheck %s
// RUN: %clang -x cx -target arm64-apple-macosx -Xclang -ast-print -fsyntax-only %s > %t.print.c
// RUN: %clang -x cx -std=c23 -target arm64-apple-macosx -S -emit-llvm -o - %t.print.c | FileCheck %s
// expected-no-diagnostics

#module Perm

enum Permission: OptionSet {
  case read, write, execute
}

// The backing type is the smallest unsigned integer with a bit per case.
#define C8(p) p##0, p##1, p##2, p##3, p##4, p##5, p##6, p##7
enum Eight: OptionSet { case C8(a) }
enum Nine: OptionSet { case C8(a), b }
enum Seventeen: OptionSet { case C8(a), C8(b), c }
enum SixtyFour: OptionSet { case C8(a), C8(b), C8(c), C8(d), C8(e), C8(f), C8(g), C8(h) }
_Static_assert(sizeof(enum Permission) == 1, "three bits");
_Static_assert(sizeof(enum Eight) == 1 && sizeof(enum Nine) == 2, "8 and 9");
_Static_assert(sizeof(enum Seventeen) == 4 && sizeof(enum SixtyFour) == 8, "17 and 64");
_Static_assert(Permission.execute.rawValue == 4, "case i is bit i");
_Static_assert(SixtyFour.h7.rawValue == 0x8000000000000000ull, "bit 63");

// CHECK-LABEL: define {{.*}} @"_Z{{[0-9]+}}_Cx0$Perm$grantv"
// CHECK: ret i8 3
enum Permission grant(void) { return [.read, .write] }
enum Permission none(void) { return [] }

// CHECK-LABEL: define {{.*}} @"_Z{{[0-9]+}}_Cx0$Perm$algebra
// CHECK: or i8
// CHECK: and i8
// CHECK: xor i8
// The complement stays within the declared bits.
// CHECK: xor i8 %{{.*}}, 7
unsigned char algebra(enum Permission a, enum Permission b) {
  enum Permission u = a | b
  enum Permission i = a & b
  enum Permission d = a ^ b
  enum Permission c = ~a
  enum Permission r = a - b
  a |= .execute
  a -= .read
  return (u | i | d | c | r | a).rawValue
}

// contains, subset, superset and disjointness evaluate each operand once.
// A literal stays one operand: [.read, .write] is 3, not .read | (...).
// CHECK-LABEL: define {{.*}} @"_Z{{[0-9]+}}_Cx0$Perm$tests
// CHECK: and i8 2, %{{.*}}
// CHECK: and i8 3, %{{.*}}
int tests(enum Permission p, enum Permission q) {
  int n = 0
  if (p.contains(.write)) n += 1
  if (p.contains([.read, .write])) n += 2
  if (p.isSubset(of: q)) n += 4
  if (p.isSuperset(of: q)) n += 8
  if (p.isDisjoint(with: [.execute])) n += 16
  if (p == [] || p != q) n += 32
  return n
}

enum Permission global = [.read, .execute];
