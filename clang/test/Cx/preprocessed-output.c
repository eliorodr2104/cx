// Ownership is a property of a source file, which preprocessed output only
// names in line markers. `-E` writes each file's module after its marker, and
// compiling preprocessed input takes the module of the file a marker names, so
// `-E`, `-save-temps` and `-frewrite-includes` all give the symbols of a
// direct compile.

// RUN: rm -rf %t && mkdir -p %t
// RUN: %clang -x cx -I %S/Inputs -S -emit-llvm -o - %s | FileCheck --check-prefix=SYM %s

// -E writes the modules, and its output compiles to the same symbols.
// RUN: %clang -x cx -I %S/Inputs -E %s -o %t/pp.i
// RUN: FileCheck --check-prefix=PP --input-file=%t/pp.i %s
// RUN: %clang -x cx-cpp-output -S -emit-llvm -o - %t/pp.i | FileCheck --check-prefix=SYM %s

// -save-temps goes through the same preprocessed file.
// RUN: cd %t && %clang -x cx -I %S/Inputs -save-temps -S -emit-llvm -o save.ll %s
// RUN: FileCheck --check-prefix=SYM --input-file=%t/save.ll %s

// -frewrite-includes keeps each header's own directive in its region.
// RUN: %clang -x cx -I %S/Inputs -E -frewrite-includes %s -o %t/rw.c
// RUN: %clang -x cx -S -emit-llvm -o - %t/rw.c | FileCheck --check-prefix=SYM %s

// A module assigned by the build is written out too.
// RUN: %clang -x cx -I %S/Inputs -DNO_DIRECTIVE -fcx-module=Build -E %s -o %t/build.i
// RUN: FileCheck --check-prefix=BUILDPP --input-file=%t/build.i %s
// RUN: %clang -x cx-cpp-output -S -emit-llvm -o - %t/build.i \
// RUN:   | FileCheck --check-prefix=BUILDSYM %s
// RUN: %clang -x cx-cpp-output -fcx-module=Build -S -emit-llvm -o - %t/build.i \
// RUN:   | FileCheck --check-prefix=BUILDSYM %s

#ifndef NO_DIRECTIVE
#module App
#endif
#include "cx-pp-lib.h"
#include "cx-pp-c.h"

int use(void) { return lib_value(1) + c_value(2); }

// SYM-DAG: define {{.*}}@"_Z{{[0-9]+}}_Cx0$App$usev"(
// SYM-DAG: declare {{.*}}@"_Z{{[0-9]+}}_Cx0$Library$lib_valuei"(
// SYM-DAG: declare {{.*}}@c_value(

// PP: # 1 "{{.*}}preprocessed-output.c"
// PP: #module App
// PP: # 1 "{{.*}}cx-pp-lib.h" 1
// PP: #module Library
// PP: # 1 "{{.*}}cx-pp-c.h" 1
// PP-NOT: #module

// BUILDPP: # 1 "{{.*}}preprocessed-output.c"
// BUILDPP-NEXT: #module Build
// BUILDSYM-DAG: define {{.*}}@"_Z{{[0-9]+}}_Cx0$Build$usev"(
// BUILDSYM-DAG: declare {{.*}}@"_Z{{[0-9]+}}_Cx0$Library$lib_valuei"(
// BUILDSYM-DAG: declare {{.*}}@c_value(
