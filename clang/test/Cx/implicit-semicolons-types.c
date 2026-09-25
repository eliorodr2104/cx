// A line that begins with a type name followed by a declarator starts a new
// declaration, so the ';' after the tag definition before it is implied. A
// declarator spelled like the type, `Q;` or `R(int);`, keeps C's reading.
// RUN: %clang -x cx -target arm64-apple-macosx -fsyntax-only -Xclang -verify %s
// expected-no-diagnostics

#module Types

struct Point { int x, y; }
Point origin(void);
enum Mode { Fast, Slow }
Mode current;
struct Pair { int a; }
Pair *pairs;
typedef int Count;
struct Box { int v; }
Count boxes;

// C: a variable named Q of type struct Q, and a function named R.
struct Q { int v; }
Q;
_Static_assert(sizeof(Q) == sizeof(int), "Q is the variable");
struct R { int v; }
R(int);
