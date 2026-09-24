// Valid C keeps its meaning in Cx wherever a tag name meets an identifier C
// already reads: the idioms below compile to the same code as C and as Cx.

// RUN: rm -rf %t && mkdir -p %t
// RUN: %clang -x c -std=gnu89 -w -S -emit-llvm -o - %s | grep -v -e ModuleID -e source_filename > %t/c89.ll
// RUN: %clang -x cx -std=gnu89 -w -S -emit-llvm -o - %s | grep -v -e ModuleID -e source_filename > %t/cx89.ll
// RUN: diff %t/c89.ll %t/cx89.ll
// RUN: %clang -x c -std=gnu17 -w -S -emit-llvm -o - %s | grep -v -e ModuleID -e source_filename > %t/c17.ll
// RUN: %clang -x cx -std=gnu17 -w -S -emit-llvm -o - %s | grep -v -e ModuleID -e source_filename > %t/cx17.ll
// RUN: diff %t/c17.ll %t/cx17.ll
// RUN: %clang -x c -std=c23 -w -S -emit-llvm -o - %s -DC23 | grep -v -e ModuleID -e source_filename > %t/c23.ll
// RUN: %clang -x cx -std=c23 -w -S -emit-llvm -o - %s -DC23 | grep -v -e ModuleID -e source_filename > %t/cx23.ll
// RUN: diff %t/c23.ll %t/cx23.ll

/* A tag and a function of one name: `struct stat` and `stat()`. */
struct stat { int size; };
int stat(const char *path, struct stat *out);
int probe(void) { struct stat st; return stat("x", &st) + st.size; }

/* A tag and a local variable of one name. */
struct timeval { long sec; };
long now(void) { struct timeval timeval; timeval.sec = 1; return timeval.sec; }

/* A tag and a typedef of one name. */
typedef struct Node Node;
struct Node { Node *next; };
Node *first(Node *n) { return n->next; }

/* A tag and an enumerator of one name. */
enum Kind { Kind, Other };
int kind(void) { return Kind + Other; }

/* A tag shadowed by an inner ordinary declaration. */
struct Pair { int a; };
int inner(void) { int Pair = 3; return Pair; }

#if !defined(__STDC_VERSION__) || __STDC_VERSION__ < 199901L
/* C89: implicit int reads the tag name as the thing declared. */
struct Count { int n; };
static Count;
int count(void) { return Count; }

/* C89: a call reads it as an implicitly declared function. */
struct Area { int a; };
int area(void) { return Area(3); }
#endif

#ifndef C23
/* Before C23: a K&R identifier list reads it as a parameter. */
struct Width { int w; };
int width(Width) int Width; { return Width; }
#endif
