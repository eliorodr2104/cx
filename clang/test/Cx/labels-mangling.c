// Argument labels are part of the mangled Cx identity, and only of an owned
// entity's identity (M3).

// RUN: %clang -x cx -std=gnu17 -S -emit-llvm -o - %s | FileCheck %s

// An unowned file has no Cx linkage, so labels stay a source-level interface
// there and the symbol keeps its C name.
// RUN: %clang -x cx -std=gnu17 -S -emit-llvm -o - %s -DNO_MODULE \
// RUN:   | FileCheck --check-prefix=UNOWNED %s

#ifndef NO_MODULE
#module UI
#endif

void labeled(int width newWidth, int height newHeight) {
  (void)newWidth; (void)newHeight;
}
// CHECK-DAG: define {{.*}}@"_Z29_Cx0$UI$labeled$width:height:ii"
// UNOWNED-DAG: define {{.*}}@labeled(

// A partly labeled signature records the unlabeled positions too.
void mixed(int v, int mode m) { (void)v; (void)m; }
// CHECK-DAG: define {{.*}}@"_Z21_Cx0$UI$mixed$_:mode:ii"

// A wholly unlabeled function keeps the symbol it had before labels existed.
void unlabeled(int v, int w) { (void)v; (void)w; }
// CHECK-DAG: define {{.*}}@"_Z17_Cx0$UI$unlabeledii"
// UNOWNED-DAG: define {{.*}}@unlabeled(
