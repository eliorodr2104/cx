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

#include "clang/AST/Expr.h"
#include "clang/AST/ExprCXX.h"
#include "clang/Sema/Overload.h"
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

void Sema::AddCxLinkage(FunctionDecl *FD, const LookupResult &Previous) {
  if (!getLangOpts().CX || FD->hasAttr<CxLinkageAttr>())
    return;

  // The program entry point keeps its C name whatever owns its file.
  if (FD->isMain())
    return;

  // An entity's identity is decided by its first declaration and never by a
  // later one. A function first declared in an ordinary C header is still that
  // C entity when it is defined inside a module, and one first declared inside
  // a module keeps its module when a later declaration is written in an
  // unowned file. This runs before redeclaration merging, so the answer comes
  // from the lookup result rather than from a redeclaration chain.
  const IdentifierInfo *Module = nullptr;
  bool SawPrevious = false;
  for (const NamedDecl *ND : Previous) {
    const auto *Prev = dyn_cast<FunctionDecl>(ND->getUnderlyingDecl());
    if (!Prev)
      continue;
    SawPrevious = true;
    if (const auto *A = Prev->getAttr<CxLinkageAttr>()) {
      Module = A->getModule();
      break;
    }
  }

  if (!Module) {
    if (SawPrevious)
      return; // An existing C entity keeps its C identity.
    Module = Context.getCxModuleOwner(FD->getLocation());
    if (!Module)
      return;
  }

  FD->addAttr(
      CxLinkageAttr::CreateImplicit(Context, const_cast<IdentifierInfo *>(Module)));

  // A Cx entity has a mangled name carrying its labels and parameter types, so
  // several entities can share a base name. Clang already knows how to form C
  // overload sets and rank them with C conversions only.
  if (!FD->hasAttr<OverloadableAttr>())
    FD->addAttr(OverloadableAttr::CreateImplicit(Context));
}

/// The external argument label of \p PVD, or null when it has none.
static const IdentifierInfo *getCxLabel(const ParmVarDecl *PVD) {
  if (const auto *A = PVD->getAttr<CxArgumentLabelAttr>())
    return A->getLabel();
  return nullptr;
}

void Sema::AddCxArgumentLabel(Decl *Param, const IdentifierInfo *Label) {
  if (!Param || !Label)
    return;
  Param->addAttr(CxArgumentLabelAttr::CreateImplicit(
      Context, const_cast<IdentifierInfo *>(Label)));
}

void Sema::CheckCxArgumentLabelRedeclaration(FunctionDecl *FD) {
  const FunctionDecl *Prev = FD->getPreviousDecl();
  if (!Prev || Prev->getNumParams() != FD->getNumParams())
    return;

  for (unsigned I = 0, E = FD->getNumParams(); I != E; ++I) {
    const IdentifierInfo *New = getCxLabel(FD->getParamDecl(I));
    const IdentifierInfo *Old = getCxLabel(Prev->getParamDecl(I));
    if (New == Old)
      continue;
    // Labels are part of a Cx entity's identity and of its mangled name, so a
    // mismatch would silently produce two different symbols.
    Diag(FD->getParamDecl(I)->getLocation(),
         diag::err_cx_label_redeclaration_mismatch)
        << FD;
    Diag(Prev->getParamDecl(I)->getLocation(),
         diag::note_cx_parameter_declared_here);
    return;
  }
}

void Sema::CheckCxArgumentLabels(Expr *Call,
                                 ArrayRef<const IdentifierInfo *> Labels,
                                 ArrayRef<SourceLocation> LabelLocs) {
  const auto *CE = dyn_cast_or_null<CallExpr>(Call);
  if (!CE)
    return;

  bool AnyWritten = llvm::any_of(Labels, [](const IdentifierInfo *II) {
    return II != nullptr;
  });

  const FunctionDecl *Callee = CE->getDirectCallee();
  if (!Callee) {
    // A C function pointer carries no Cx argument-label interface.
    if (AnyWritten) {
      for (unsigned I = 0, E = Labels.size(); I != E; ++I)
        if (Labels[I])
          Diag(LabelLocs[I], diag::err_cx_label_through_pointer);
    }
    return;
  }

  bool AnyDeclared = llvm::any_of(
      Callee->parameters(),
      [](const ParmVarDecl *PVD) { return getCxLabel(PVD) != nullptr; });
  if (!AnyWritten && !AnyDeclared)
    return;

  unsigned NumParams = Callee->getNumParams();
  for (unsigned I = 0, E = Labels.size(); I != E; ++I) {
    // Arguments past the declared parameters are variadic and unlabeled.
    const IdentifierInfo *Expected =
        I < NumParams ? getCxLabel(Callee->getParamDecl(I)) : nullptr;
    const IdentifierInfo *Written = Labels[I];
    if (Written == Expected)
      continue;

    // Render as `name:` so the diagnostic shows the spelling the call needs.
    auto Spelling = [](const IdentifierInfo *II) {
      return (II->getName() + ":").str();
    };
    if (!Written) {
      Diag(LabelLocs[I], diag::err_cx_missing_argument_label)
          << Spelling(Expected);
    } else if (!Expected) {
      Diag(LabelLocs[I], diag::err_cx_extraneous_argument_label)
          << Spelling(Written);
    } else {
      Diag(LabelLocs[I], diag::err_cx_wrong_argument_label)
          << Spelling(Written) << Spelling(Expected);
    }
    if (I < NumParams)
      Diag(Callee->getParamDecl(I)->getLocation(),
           diag::note_cx_parameter_declared_here);
  }
}

bool Sema::HasDifferentCxArgumentLabels(const FunctionDecl *A,
                                        const FunctionDecl *B) {
  if (A->getNumParams() != B->getNumParams())
    return false; // A different arity already makes them overloads.
  for (unsigned I = 0, E = A->getNumParams(); I != E; ++I)
    if (getCxLabel(A->getParamDecl(I)) != getCxLabel(B->getParamDecl(I)))
      return true;
  return false;
}

bool Sema::CxCandidateAcceptsCallLabels(const FunctionDecl *FD,
                                        unsigned &BadArg) {
  unsigned NumParams = FD->getNumParams();
  for (unsigned I = 0, E = CxCallArgumentLabels.size(); I != E; ++I) {
    // Arguments past the declared parameters are variadic and unlabeled.
    const IdentifierInfo *Expected =
        I < NumParams ? getCxLabel(FD->getParamDecl(I)) : nullptr;
    if (CxCallArgumentLabels[I] != Expected) {
      BadArg = I;
      return false;
    }
  }
  return true;
}

/// Render a Cx compound name's label list, using `_` for a position that has
/// no label, so diagnostics show the spelling the source uses.
static std::string renderCxLabels(ArrayRef<const IdentifierInfo *> Labels) {
  std::string Result;
  for (const IdentifierInfo *II : Labels) {
    Result += II ? II->getName().str() : std::string("_");
    Result += ':';
  }
  return Result;
}

static std::string renderCxLabelsOf(const FunctionDecl *FD) {
  SmallVector<const IdentifierInfo *, 4> Labels;
  for (const ParmVarDecl *PVD : FD->parameters())
    Labels.push_back(getCxLabel(PVD));
  return renderCxLabels(Labels);
}

static bool cxFunctionHasLabels(const FunctionDecl *FD,
                                ArrayRef<const IdentifierInfo *> Labels) {
  if (FD->getNumParams() != Labels.size())
    return false;
  for (unsigned I = 0, E = Labels.size(); I != E; ++I)
    if (getCxLabel(FD->getParamDecl(I)) != Labels[I])
      return false;
  return true;
}

ExprResult Sema::BuildCxCompoundNameRef(Expr *Fn,
                                        ArrayRef<const IdentifierInfo *> Labels,
                                        SourceLocation LParenLoc,
                                        SourceLocation RParenLoc) {
  // Collect the candidates the name currently refers to.
  UnresolvedSet<8> Candidates;
  DeclarationNameInfo NameInfo;
  NestedNameSpecifierLoc QualifierLoc;
  bool RequiresADL = false;

  if (auto *ULE = dyn_cast<UnresolvedLookupExpr>(Fn->IgnoreParens())) {
    NameInfo = ULE->getNameInfo();
    QualifierLoc = ULE->getQualifierLoc();
    RequiresADL = ULE->requiresADL();
    for (auto I = ULE->decls_begin(), E = ULE->decls_end(); I != E; ++I)
      Candidates.addDecl(*I, I.getAccess());
  } else if (auto *DRE = dyn_cast<DeclRefExpr>(Fn->IgnoreParens())) {
    NameInfo = DeclarationNameInfo(DRE->getDecl()->getDeclName(),
                                   DRE->getLocation());
    Candidates.addDecl(DRE->getDecl());
  } else {
    Diag(LParenLoc, diag::err_cx_compound_name_not_function)
        << SourceRange(Fn->getBeginLoc(), RParenLoc);
    return ExprError();
  }

  UnresolvedSet<8> Matching;
  bool SawFunction = false;
  for (auto I = Candidates.begin(), E = Candidates.end(); I != E; ++I) {
    const auto *FD = dyn_cast<FunctionDecl>((*I)->getUnderlyingDecl());
    if (!FD)
      continue;
    SawFunction = true;
    if (cxFunctionHasLabels(FD, Labels))
      Matching.addDecl(*I, I.getAccess());
  }

  if (!SawFunction) {
    Diag(LParenLoc, diag::err_cx_compound_name_not_function)
        << SourceRange(Fn->getBeginLoc(), RParenLoc);
    return ExprError();
  }

  if (Matching.empty()) {
    Diag(Fn->getBeginLoc(), diag::err_cx_no_function_with_labels)
        << NameInfo.getName() << renderCxLabels(Labels)
        << SourceRange(Fn->getBeginLoc(), RParenLoc);
    for (auto I = Candidates.begin(), E = Candidates.end(); I != E; ++I)
      if (const auto *FD = dyn_cast<FunctionDecl>((*I)->getUnderlyingDecl()))
        Diag(FD->getLocation(), diag::note_cx_compound_name_candidate)
            << renderCxLabelsOf(FD);
    return ExprError();
  }

  if (Matching.size() == 1) {
    auto *FD = cast<FunctionDecl>((*Matching.begin())->getUnderlyingDecl());
    // Functions are l-values in C++ and r-values in C, as elsewhere.
    ExprValueKind VK = getLangOpts().CPlusPlus ? VK_LValue : VK_PRValue;
    return BuildDeclRefExpr(FD, FD->getType(), VK, NameInfo, QualifierLoc);
  }

  // Several declarations share these labels. Leave the choice to the target
  // type, exactly as an unfiltered overload set would.
  return UnresolvedLookupExpr::Create(
      Context, /*NamingClass=*/nullptr, QualifierLoc, NameInfo, RequiresADL,
      Matching.begin(), Matching.end(), /*KnownDependent=*/false,
      /*KnownInstantiationDependent=*/false);
}
