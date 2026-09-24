// Tools see Cx identity: labels and the module are part of a USR, so label
// overloads are distinct symbols to an index, and a declaration's extent
// covers the whole declaration.

// RUN: c-index-test core -print-source-symbols -- -x cx %s | FileCheck --check-prefix=USR %s
// RUN: c-index-test -test-load-source all -x cx %s | FileCheck --check-prefix=EXTENT %s

#module Tools

void move(int x v) {}
void move(int y v) {}
// USR: function/C | move | c:@F@move@CX@Tools$x#I# |
// USR: function/C | move | c:@F@move@CX@Tools$y#I# |

typedef struct Box {
  int w;
  init(int width w);
  ~mutating int area();
} Box;
// The introducers `~mutating` and access specifiers precede the declaration
// proper, as C++ access specifiers do.
// EXTENT: tooling-identity.c:17:3: FunctionDecl=init:17:3 Extent=[17:3 - 17:20]
// EXTENT: tooling-identity.c:18:17: FunctionDecl=area:18:17 Extent=[18:13 - 18:23]
