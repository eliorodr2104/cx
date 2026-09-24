// An access specifier introduces a member; whatever declaration specifiers the
// member itself uses are C's business. Every level must be able to precede any
// of them (M4.4).

// RUN: %clang -x cx -std=gnu17 -fsyntax-only -Xclang -verify %s
// RUN: %clang -x cx -std=gnu17 -std=c99 -fsyntax-only -Xclang -verify %s
// RUN: %clang -x cx -std=gnu17 -std=c23 -fsyntax-only -Xclang -verify %s

// expected-no-diagnostics

#module Shapes

struct Inner { int z; };

typedef struct Qualified {
  private volatile int a;
  private _Atomic int b;
  private const int c;
  private unsigned long long d;
  private int *restrict e;
  private struct Inner f;
  private _Alignas(8) int g;
  private int h : 3;

  public volatile int i;
  internal _Atomic int j;
  private(set) const volatile int k;

  // A level still precedes `~mutating` and a method.
  private ~mutating int read(void) { return c; }
} Qualified;

// A type named like a level still wins: the spelling is contextual.
typedef int internal;
struct UsesTypedef { internal field; };
