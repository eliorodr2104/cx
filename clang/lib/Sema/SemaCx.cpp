//===--- SemaCx.cpp - Semantic Analysis for Cx constructs -----------------===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//
//
// This file implements the semantic entry points that are specific to the Cx
// language mode. Cx shares Clang's Sema state; there is no separate Cx Sema.
//
//===----------------------------------------------------------------------===//

#include "clang/Sema/Lookup.h"
#include "clang/Sema/Sema.h"

using namespace clang;

bool Sema::isCxContextualKeyword(const IdentifierInfo *II, Scope *S) {
  if (!getLangOpts().CX || !II)
    return false;

  // A visible C declaration with that spelling keeps its C interpretation.
  // Macros never reach here: the preprocessor has already expanded them.
  LookupResult R(*this, II, SourceLocation(), LookupOrdinaryName,
                 RedeclarationKind::NotForRedeclaration);
  R.suppressDiagnostics();
  LookupName(R, S, /*AllowBuiltinCreation=*/false);
  return R.empty();
}
