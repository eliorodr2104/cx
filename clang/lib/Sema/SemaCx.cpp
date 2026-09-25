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
#include "clang/Basic/SourceManager.h"
#include "clang/AST/ExprCXX.h"
#include "clang/Sema/Overload.h"
#include "clang/Sema/Initialization.h"
#include "llvm/ADT/ScopeExit.h"
#include "llvm/Support/SaveAndRestore.h"
#include "clang/Sema/Designator.h"
#include "clang/Sema/Lookup.h"
#include "clang/Sema/ScopeInfo.h"
#include "clang/Sema/Sema.h"
#include "TypeLocBuilder.h"
#include "clang/AST/ASTConsumer.h"

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

static const IdentifierInfo *getCxLabel(const ParmVarDecl *PVD);

/// Whether \p Prev is a declaration of the same entity as \p New. A variable
/// has one entity per name; an owned function is one of an overload set, and
/// only a declaration with its type and labels declares the same one.
static bool isCxSameEntity(ASTContext &Ctx, const VarDecl *Prev,
                           const VarDecl *New) {
  return true;
}
static bool isCxSameEntity(ASTContext &Ctx, const FunctionDecl *Prev,
                           const FunctionDecl *New) {
  if (!Prev->hasAttr<CxLinkageAttr>())
    return true; // A C function has no overloads; any one is the entity.
  if (!Ctx.hasSameType(Prev->getType(), New->getType()) ||
      Prev->getNumParams() != New->getNumParams())
    return false;
  for (unsigned I = 0, E = New->getNumParams(); I != E; ++I)
    if (getCxLabel(Prev->getParamDecl(I)) != getCxLabel(New->getParamDecl(I)))
      return false;
  return true;
}

/// The module that owns the entity \p New declares, following the rule that
/// an entity's identity is decided by its first declaration and never by a
/// later one. \p Prev receives that first declaration, and \p SawPrevious
/// whether one was found at all, which is what tells "no owner because the
/// entity is a C one" from "no owner because it is new here".
template <typename DeclT>
static const IdentifierInfo *cxModuleOfFirstDecl(ASTContext &Ctx,
                                                 const LookupResult &Previous,
                                                 const DeclT *New,
                                                 bool &SawPrevious,
                                                 const DeclT *&First) {
  SawPrevious = false;
  First = nullptr;
  for (const NamedDecl *ND : Previous) {
    const auto *Prev = dyn_cast<DeclT>(ND->getUnderlyingDecl());
    if (!Prev || !isCxSameEntity(Ctx, Prev, New))
      continue;
    SawPrevious = true;
    First = Prev;
    if (const auto *A = Prev->template getAttr<CxLinkageAttr>())
      return A->getModule();
  }
  return nullptr;
}

/// Report an entity that a module declares after another module declared it
/// first. Both would claim the same symbol, and which one wins would depend on
/// include order.
template <typename DeclT>
static void diagnoseCxOtherModule(Sema &S, const DeclT *New, const DeclT *First,
                                  const IdentifierInfo *Module) {
  const IdentifierInfo *Here =
      S.getASTContext().getCxModuleOwner(New->getLocation());
  if (!First || !Module || !Here || Here == Module)
    return;
  S.Diag(New->getLocation(), diag::err_cx_redeclared_in_other_module)
      << New << Module << Here;
  S.Diag(First->getLocation(), diag::note_previous_declaration);
}

void Sema::AddCxLinkage(VarDecl *VD, const LookupResult &Previous) {
  if (!getLangOpts().CX || VD->hasAttr<CxLinkageAttr>())
    return;

  // Only externally visible module-owned data needs a module in its symbol.
  // A local, a member, or anything with internal linkage is already distinct
  // from every other translation unit's.
  if (!VD->getDeclContext()->getRedeclContext()->isTranslationUnit() ||
      VD->getStorageClass() == SC_Static)
    return;

  bool SawPrevious = false;
  const VarDecl *First = nullptr;
  const IdentifierInfo *Module =
      cxModuleOfFirstDecl(Context, Previous, VD, SawPrevious, First);
  diagnoseCxOtherModule(*this, VD, First, Module);
  if (!Module) {
    if (SawPrevious)
      return; // An existing C entity keeps its C identity.
    Module = Context.getCxModuleOwner(VD->getLocation());
    if (!Module)
      return;
  }

  VD->addAttr(CxLinkageAttr::CreateImplicit(
      Context, const_cast<IdentifierInfo *>(Module)));
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
  bool SawPrevious = false;
  const FunctionDecl *First = nullptr;
  const IdentifierInfo *Module =
      cxModuleOfFirstDecl(Context, Previous, FD, SawPrevious, First);
  diagnoseCxOtherModule(*this, FD, First, Module);

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

/// A Cx method's first parameter is the implicit receiver, which no written
/// argument corresponds to. Everything that lines call arguments up with
/// parameters has to skip it.
static unsigned cxSelfOffset(const FunctionDecl *FD) {
  return FD->hasAttr<CxMethodAttr>() ? 1 : 0;
}

unsigned Sema::getCxReceiverOffset(const FunctionDecl *FD) const {
  return getLangOpts().CX && FD ? cxSelfOffset(FD) : 0;
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
  // `_` spells an unlabeled position, as in the compound name `move(_:mode:)`.
  if (Label->isStr("_"))
    return;
  // `$` separates the parts of a Cx symbol, so a label cannot contain it.
  if (Label->getName().contains('$')) {
    Diag(Param->getLocation(), diag::err_cx_label_dollar);
    return;
  }
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
    // A call whose callee is already in error says nothing about labels.
    if (CE->getCallee()->containsErrors())
      return;
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

  unsigned Self = cxSelfOffset(Callee);
  unsigned NumParams = Callee->getNumParams();
  for (unsigned I = 0, E = Labels.size(); I != E; ++I) {
    // Arguments past the declared parameters are variadic and unlabeled.
    const IdentifierInfo *Expected =
        I + Self < NumParams ? getCxLabel(Callee->getParamDecl(I + Self))
                             : nullptr;
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
    if (I + Self < NumParams)
      Diag(Callee->getParamDecl(I + Self)->getLocation(),
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
  unsigned Self = cxSelfOffset(FD);
  unsigned NumParams = FD->getNumParams();
  for (unsigned I = 0, E = CxCallArgumentLabels.size(); I != E; ++I) {
    // Arguments past the declared parameters are variadic and unlabeled.
    const IdentifierInfo *Expected =
        I + Self < NumParams ? getCxLabel(FD->getParamDecl(I + Self))
                             : nullptr;
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
  // The implicit receiver is not a written argument and has no label.
  for (const ParmVarDecl *PVD : FD->parameters().drop_front(cxSelfOffset(FD)))
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

//===----------------------------------------------------------------------===//
// Struct methods
//===----------------------------------------------------------------------===//

/// Whether \p FD is a Cx initializer. An initializer is a method named
/// `init`: it takes the receiver the same way, and everything that gives a
/// method its labels, its linkage and its symbol applies unchanged.
static bool isCxInit(const FunctionDecl *FD) {
  return FD->hasAttr<CxMethodAttr>() && FD->getDeclName().isIdentifier() &&
         FD->getName() == "init";
}

bool Sema::isCxInitializer(const FunctionDecl *FD) const {
  return FD && isCxInit(FD);
}

/// Whether \p FD is a Cx deinit: the method that destroys its receiver.
static bool isCxDeinitDecl(const FunctionDecl *FD) {
  return FD->hasAttr<CxMethodAttr>() && FD->getDeclName().isIdentifier() &&
         FD->getName() == "deinit";
}

/// The method of \p RD that \p New redeclares, or null when it introduces a
/// new one. Identity is the name, the written parameter types and the
/// argument labels; local parameter names may differ. The return type and the
/// receiver's constness are not part of it, so a match may still conflict.
static FunctionDecl *findCxMethodDeclaration(ASTContext &Ctx,
                                             const RecordDecl *RD,
                                             const FunctionDecl *New) {
  const auto *NewFPT = New->getType()->getAs<FunctionProtoType>();
  for (Decl *D : RD->decls()) {
    auto *Old = dyn_cast<FunctionDecl>(D);
    if (!Old || Old == New || !Old->hasAttr<CxMethodAttr>())
      continue;
    if (Old->getDeclName() != New->getDeclName())
      continue;
    if (Old->getNumParams() != New->getNumParams())
      continue;
    const auto *OldFPT = Old->getType()->getAs<FunctionProtoType>();
    if (!OldFPT || !NewFPT || OldFPT->isVariadic() != NewFPT->isVariadic())
      continue;
    // Parameter 0 is the receiver.
    bool SameParams = true;
    for (unsigned I = 1, E = Old->getNumParams(); I != E; ++I)
      if (!Ctx.hasSameType(OldFPT->getParamType(I), NewFPT->getParamType(I)))
        SameParams = false;
    if (!SameParams)
      continue;
    bool SameLabels = true;
    for (unsigned I = 0, E = Old->getNumParams(); I != E; ++I)
      if (getCxLabel(Old->getParamDecl(I)) != getCxLabel(New->getParamDecl(I)))
        SameLabels = false;
    if (SameLabels)
      return Old;
  }
  return nullptr;
}

void Sema::DiagnoseCxNearMiss(const RecordDecl *RD, const FunctionDecl *New) {
  // Only a continuation implements what was declared elsewhere. Inside the
  // primary definition two same-named members are an ordinary overload pair.
  if (!RD->isCompleteDefinition())
    return;

  for (const Decl *D : RD->decls()) {
    const auto *Old = dyn_cast<FunctionDecl>(D);
    if (!Old || Old == New || !Old->hasAttr<CxMethodAttr>())
      continue;
    if (Old->getDeclName() != New->getDeclName())
      continue;
    // A member that already has an implementation is not the one this
    // definition meant to supply.
    if (Old->isDefined())
      continue;
    // Same name and same number of written arguments, yet not a
    // redeclaration: the labels or the parameter types drifted. Left alone
    // this silently becomes a new member and a link error on the declared
    // one.
    if (Old->getNumParams() != New->getNumParams())
      continue;

    Diag(New->getLocation(), diag::warn_cx_continuation_near_miss) << Old;
    Diag(Old->getLocation(), diag::note_cx_near_miss_declaration)
        << Old << renderCxLabelsOf(Old);
    return;
  }
}

Decl *Sema::ActOnCxMethodDeclarator(Scope *S, Decl *TagD, Declarator &D,
                                    bool NonMutating) {
  auto *RD = dyn_cast_or_null<RecordDecl>(TagD);
  if (!RD)
    return nullptr;

  DeclarationNameInfo NameInfo = GetNameForDeclarator(D);
  if (!NameInfo.getName().isIdentifier()) {
    Diag(D.getIdentifierLoc(), diag::err_cx_method_name);
    return nullptr;
  }

  TypeSourceInfo *TInfo = GetTypeForDeclarator(D);
  const auto *FT = TInfo->getType()->getAs<FunctionType>();
  if (!FT) {
    Diag(D.getBeginLoc(), diag::err_cx_method_needs_prototype);
    return nullptr;
  }

  bool InvalidInit = false;
  if (NameInfo.getName().getAsIdentifierInfo()->isStr("deinit")) {
    const auto *FPT = dyn_cast<FunctionProtoType>(FT);
    if (!FT->getReturnType()->isVoidType() || (FPT && FPT->getNumParams())) {
      Diag(D.getBeginLoc(), diag::err_cx_deinit_signature);
      InvalidInit = true;
    }
    if (NonMutating) {
      Diag(D.getBeginLoc(), diag::err_cx_deinit_mutating);
      NonMutating = false;
    }
    if (RD->isUnion()) {
      Diag(D.getBeginLoc(), diag::err_cx_deinit_in_union);
      InvalidInit = true;
    }
  }
  if (NameInfo.getName().getAsIdentifierInfo()->isStr("init")) {
    // The parser gives an initializer a `void` return type, so a written one
    // is the only way this fails.
    if (!FT->getReturnType()->isVoidType()) {
      Diag(D.getBeginLoc(), diag::err_cx_init_return_type);
      return nullptr;
    }
    // An initializer establishes the value, so its receiver is never const.
    if (NonMutating) {
      Diag(D.getBeginLoc(), diag::err_cx_init_mutating);
      NonMutating = false;
    }
    if (RD->isUnion()) {
      Diag(D.getBeginLoc(), diag::err_cx_init_in_union);
      InvalidInit = true;
    }
  }

  // A method is an associated function: `self` is an ordinary leading
  // parameter, so the type gains no per-instance pointer and the record gains
  // no storage. `~mutating` is a const pointee, which is what lets the method
  // be called on a `let` or `const` value.
  QualType RecTy = Context.getCanonicalTagType(RD);
  if (NonMutating)
    RecTy = RecTy.withConst();
  QualType SelfTy = Context.getPointerType(RecTy);

  SmallVector<QualType, 8> ParamTys;
  ParamTys.push_back(SelfTy);
  FunctionProtoType::ExtProtoInfo EPI;
  if (const auto *FPT = dyn_cast<FunctionProtoType>(FT)) {
    ParamTys.append(FPT->param_types().begin(), FPT->param_types().end());
    EPI = FPT->getExtProtoInfo();
  } else {
    // `void increment()` on a method means no parameters. Cx methods are not
    // K&R declarations, so an empty list is not an unspecified one.
    EPI.ExtInfo = FT->getExtInfo();
  }
  QualType MethodTy =
      Context.getFunctionType(FT->getReturnType(), ParamTys, EPI);

  FunctionDecl *FD = FunctionDecl::Create(
      Context, RD, D.getBeginLoc(), NameInfo, MethodTy,
      Context.getTrivialTypeSourceInfo(MethodTy, D.getBeginLoc()), SC_None,
      getCurFPFeatures().isFPConstrained(), /*isInlineSpecified=*/false,
      /*hasWrittenPrototype=*/true, ConstexprSpecKind::Unspecified,
      /*TrailingRequiresClause=*/{});

  SmallVector<ParmVarDecl *, 8> Params;
  auto *Self = ParmVarDecl::Create(
      Context, FD, SourceLocation(), D.getIdentifierLoc(),
      &Context.Idents.get("self"), SelfTy,
      Context.getTrivialTypeSourceInfo(SelfTy, D.getIdentifierLoc()), SC_None,
      /*DefArg=*/nullptr);
  Self->setImplicit();
  Self->setScopeInfo(0, 0);
  Params.push_back(Self);

  const DeclaratorChunk::FunctionTypeInfo &FTI = D.getFunctionTypeInfo();
  // `(void)` is an empty parameter list written the C way: the declarator
  // carries one unnamed void pseudo-parameter that the function type does
  // not, so it must not become a real one here.
  bool VoidParameterList =
      FTI.NumParams == 1 && !FTI.Params[0].Ident &&
      isa_and_nonnull<ParmVarDecl>(FTI.Params[0].Param) &&
      cast<ParmVarDecl>(FTI.Params[0].Param)->getType()->isVoidType();
  for (unsigned I = 0; !VoidParameterList && I != FTI.NumParams; ++I) {
    if (auto *P = dyn_cast_or_null<ParmVarDecl>(FTI.Params[I].Param)) {
      // The receiver is always `self`; a parameter of that name would hide it.
      if (P->getIdentifier() && P->getIdentifier()->isStr("self")) {
        Diag(P->getLocation(), diag::err_cx_self_parameter);
        P->setInvalidDecl();
      }
      P->setOwningFunction(FD);
      P->setScopeInfo(0, Params.size());
      Params.push_back(P);
    }
  }
  FD->setParams(Params);
  // The declaration extends to the end of its declarator; a body, when it
  // follows, extends it further.
  FD->setRangeEnd(D.getSourceRange().getEnd());
  // Linkage for a declaration held by a record is computed the way a class
  // member's is, which needs an access specifier. Cx members are public by
  // default; M4b makes that explicit and adds the other levels.
  FD->setAccess(AS_public);
  FD->addAttr(CxMethodAttr::CreateImplicit(Context));
  if (InvalidInit)
    FD->setInvalidDecl();

  // A method cannot be a C entity, so it always carries a Cx name. The module
  // may be absent when the file has no owner; the record name still keeps the
  // symbol distinct.
  const IdentifierInfo *Owner = Context.getCxModuleOwner(FD->getLocation());
  FD->addAttr(CxLinkageAttr::CreateImplicit(
      Context, const_cast<IdentifierInfo *>(Owner)));
  if (!FD->hasAttr<OverloadableAttr>())
    FD->addAttr(OverloadableAttr::CreateImplicit(Context));

  ProcessDeclAttributes(S, FD, D);

  // A method and a stored member of one name would make `value.name` mean
  // two things; C rejects two fields of one name the same way.
  for (Decl *D : RD->decls())
    if ((isa<FieldDecl>(D) || isa<IndirectFieldDecl>(D)) &&
        cast<NamedDecl>(D)->getDeclName() == FD->getDeclName()) {
      Diag(FD->getLocation(), diag::err_duplicate_member) << FD->getDeclName();
      Diag(D->getLocation(), diag::note_previous_declaration);
      FD->setInvalidDecl();
      break;
    }

  // A continuation implements what the primary definition declared, so a
  // matching method is a redeclaration and not a second entity. A match that
  // differs in its return type or in `~mutating` is the same method declared
  // twice, not an overload: nothing at a call could choose between them.
  if (FunctionDecl *Prev = findCxMethodDeclaration(Context, RD, FD)) {
    if (!Context.hasSameType(Prev->getType(), FD->getType())) {
      Diag(FD->getLocation(), diag::err_conflicting_types) << FD;
      Diag(Prev->getLocation(), diag::note_previous_declaration);
      FD->setInvalidDecl();
    } else {
      FD->setPreviousDeclaration(Prev);
      FD->setAccess(Prev->getAccess());
    }
  } else {
    // Whether a type has initializers decides how every construction of it
    // is built, so a continuation, which another translation unit may never
    // see, cannot introduce one. It can implement a declared one.
    if (isCxInit(FD) && RD->isCompleteDefinition()) {
      Diag(FD->getLocation(), diag::err_cx_init_in_continuation) << RD;
      Diag(RD->getLocation(), diag::note_cx_primary_definition) << RD;
      FD->setInvalidDecl();
    }
    // Whether a type has a deinit decides whether its values copy.
    if (isCxDeinitDecl(FD) && RD->isCompleteDefinition()) {
      Diag(FD->getLocation(), diag::err_cx_deinit_in_continuation) << RD;
      Diag(RD->getLocation(), diag::note_cx_primary_definition) << RD;
      FD->setInvalidDecl();
    }
    DiagnoseCxNearMiss(RD, FD);
  }

  RD->addDecl(FD);
  return FD;
}

FunctionDecl *Sema::LookupCxMethod(const RecordDecl *RD, DeclarationName Name) {
  if (!getLangOpts().CX || !RD)
    return nullptr;
  for (Decl *D : RD->decls())
    if (auto *FD = dyn_cast<FunctionDecl>(D))
      if (FD->hasAttr<CxMethodAttr>() && !isCxInit(FD) &&
          FD->getDeclName() == Name)
        return FD;
  return nullptr;
}

bool Sema::isCxSelfReference(const Expr *E) {
  if (!getLangOpts().CX || !E)
    return false;
  const auto *DRE = dyn_cast<DeclRefExpr>(E->IgnoreParenImpCasts());
  if (!DRE)
    return false;
  const auto *PVD = dyn_cast<ParmVarDecl>(DRE->getDecl());
  if (!PVD || !PVD->isImplicit())
    return false;
  const auto *FD = dyn_cast_or_null<FunctionDecl>(PVD->getDeclContext());
  return FD && FD->hasAttr<CxMethodAttr>() && FD->getNumParams() > 0 &&
         FD->getParamDecl(0) == PVD;
}

/// The object a modification ultimately reaches, looking through member
/// access, indexing and dereference. \p Member, when given, receives the
/// innermost member the chain names.
static const Expr *cxUnderlyingObject(const Expr *E,
                                      const ValueDecl **Member = nullptr) {
  while (E) {
    E = E->IgnoreParenImpCasts();
    if (const auto *ME = dyn_cast<MemberExpr>(E)) {
      if (Member && !*Member)
        *Member = ME->getMemberDecl();
      E = ME->getBase();
      continue;
    }
    if (const auto *ASE = dyn_cast<ArraySubscriptExpr>(E)) {
      E = ASE->getBase();
      continue;
    }
    if (const auto *UO = dyn_cast<UnaryOperator>(E);
        UO && UO->getOpcode() == UO_Deref) {
      E = UO->getSubExpr();
      continue;
    }
    break;
  }
  return E;
}

bool Sema::DiagnoseCxNonMutatingWrite(const Expr *E, SourceLocation Loc) {
  if (!getLangOpts().CX)
    return false;
  FunctionDecl *Method = getCurrentCxMethod();
  if (!Method || Method->getNumParams() == 0)
    return false;
  // Only a `~mutating` method promised anything; a mutating one reaching here
  // is an ordinary const problem.
  QualType Pointee = Method->getParamDecl(0)->getType()->getPointeeType();
  if (Pointee.isNull() || !Pointee.isConstQualified())
    return false;
  // The message names the member being written, so a write that reaches no
  // member -- `*self = other` -- is left to C's own diagnostic.
  const ValueDecl *Member = nullptr;
  if (!isCxSelfReference(cxUnderlyingObject(E, &Member)) || !Member)
    return false;

  Diag(Loc, diag::err_cx_write_through_non_mutating)
      << Member << Method << E->getSourceRange();
  Diag(Method->getLocation(), diag::note_cx_remove_non_mutating) << Method;
  return true;
}

FunctionDecl *Sema::getCurrentCxMethod() {
  if (!getLangOpts().CX)
    return nullptr;
  auto *FD = dyn_cast_or_null<FunctionDecl>(CurContext);
  return FD && FD->hasAttr<CxMethodAttr>() ? FD : nullptr;
}

ExprResult Sema::BuildCxMethodRef(Expr *Base, bool IsArrow,
                                  SourceLocation OpLoc, FunctionDecl *M,
                                  const DeclarationNameInfo &NameInfo) {
  // C classifies a member access by its base, so the value kind follows it.
  ExprValueKind VK = IsArrow ? VK_LValue : Base->getValueKind();
  return MemberExpr::Create(
      Context, Base, IsArrow, OpLoc, NestedNameSpecifierLoc(), SourceLocation(),
      M, DeclAccessPair::make(M, M->getAccess()), NameInfo,
      /*TemplateArgs=*/nullptr, Context.BoundMemberTy, VK, OK_Ordinary,
      NOUR_None);
}

ExprResult Sema::BuildCxMethodCall(Expr *Callee, SourceLocation LParenLoc,
                                   MultiExprArg Args,
                                   SourceLocation RParenLoc) {
  auto *ME = cast<MemberExpr>(Callee->IgnoreParens());
  auto *Named = cast<FunctionDecl>(ME->getMemberDecl());
  Expr *Base = ME->getBase();

  // The receiver is passed by address; nothing about the value is copied.
  ExprResult SelfArg;
  if (ME->isArrow()) {
    SelfArg = Base;
  } else {
    llvm::SaveAndRestore<bool> TakingAddress(CxTakingReceiverAddress, true);
    SelfArg = CreateBuiltinUnaryOp(ME->getOperatorLoc(), UO_AddrOf, Base);
    if (SelfArg.isInvalid())
      return ExprError();
  }

  SmallVector<Expr *, 8> AllArgs;
  AllArgs.push_back(SelfArg.get());
  AllArgs.append(Args.begin(), Args.end());

  // Several methods may share this name, so the call picks among them the
  // same way a free function call does. The receiver is just the first
  // argument, which is what makes that reuse possible.
  const auto *RD = cast<RecordDecl>(Named->getDeclContext());
  OverloadCandidateSet Candidates(ME->getMemberLoc(),
                                  OverloadCandidateSet::CSK_Normal);
  llvm::SmallPtrSet<const FunctionDecl *, 4> Seen;
  for (Decl *D : RD->decls()) {
    auto *M = dyn_cast<FunctionDecl>(D);
    if (!M || !M->hasAttr<CxMethodAttr>() ||
        M->getDeclName() != Named->getDeclName())
      continue;
    if (!Seen.insert(M->getCanonicalDecl()).second)
      continue;
    AddOverloadCandidate(M, DeclAccessPair::make(M, M->getAccess()), AllArgs,
                         Candidates);
  }

  OverloadCandidateSet::iterator Best;
  switch (Candidates.BestViableFunction(*this, ME->getMemberLoc(), Best)) {
  case OR_Success:
    break;
  case OR_No_Viable_Function:
    Candidates.NoteCandidates(
        PartialDiagnosticAt(ME->getMemberLoc(),
                            PDiag(diag::err_ovl_no_viable_function_in_call)
                                << Named << ME->getSourceRange()),
        *this, OCD_AllCandidates, AllArgs);
    return ExprError();
  case OR_Ambiguous:
    Candidates.NoteCandidates(
        PartialDiagnosticAt(ME->getMemberLoc(),
                            PDiag(diag::err_ovl_ambiguous_call)
                                << Named << ME->getSourceRange()),
        *this, OCD_AmbiguousCandidates, AllArgs);
    return ExprError();
  case OR_Deleted:
    return ExprError();
  }
  FunctionDecl *Method = Best->Function;

  // A deinit runs automatically for a variable, so it is called only on
  // storage a pointer reaches, and never on the method's own receiver.
  if (isCxDeinitDecl(Method)) {
    bool OnSelf = isCxSelfReference(Base);
    if (!ME->isArrow() || OnSelf) {
      Diag(ME->getMemberLoc(), diag::err_cx_deinit_call) << (OnSelf ? 1 : 0);
      return ExprError();
    }
  }

  if (CheckCxMemberAccess(Method, ME->getMemberLoc(), /*ForWrite=*/false))
    return ExprError();

  // A `~mutating` method takes a const receiver, so a constant value accepts
  // it; a mutating one does not.
  QualType SelfParamTy = Method->getParamDecl(0)->getType();
  QualType Pointee = SelfParamTy->getPointeeType();
  QualType GivenPointee = SelfArg.get()->getType()->getPointeeType();
  if (!GivenPointee.isNull() && GivenPointee.isConstQualified() &&
      !Pointee.isConstQualified()) {
    Diag(ME->getMemberLoc(), diag::err_cx_mutating_on_const)
        << Method << GivenPointee;
    Diag(Method->getLocation(), diag::note_cx_mark_non_mutating);
    return ExprError();
  }
  // No method takes a volatile receiver, so a volatile value has none to
  // call; C would only warn and drop the qualifier.
  if (!GivenPointee.isNull() && GivenPointee.isVolatileQualified()) {
    Diag(ME->getMemberLoc(), diag::err_cx_method_on_volatile)
        << Method << GivenPointee;
    return ExprError();
  }
  // A mutating method writes the receiver, so a receiver reached through
  // members needs the write access of every one of them. A `~mutating` one
  // only reads it.
  if (!ME->isArrow() && !Pointee.isConstQualified() &&
      CheckCxWriteAccessChain(Base))
    return ExprError();

  ExprResult Fn = BuildDeclRefExpr(
      Method, Method->getType(),
      getLangOpts().CPlusPlus ? VK_LValue : VK_PRValue,
      DeclarationNameInfo(Method->getDeclName(), ME->getMemberLoc()),
      NestedNameSpecifierLoc());
  if (Fn.isInvalid())
    return ExprError();

  return BuildResolvedCallExpr(Fn.get(), Method, LParenLoc, AllArgs, RParenLoc);
}

/// The receiver's record inside a Cx method body, with \p Self set to the
/// receiver parameter. Null outside one.
RecordDecl *Sema::getCxReceiverRecord(ParmVarDecl **Self) {
  FunctionDecl *Method = getCurrentCxMethod();
  if (!Method || Method->getNumParams() == 0)
    return nullptr;
  ParmVarDecl *P = Method->getParamDecl(0);
  const auto *PT = P->getType()->getAs<PointerType>();
  if (!PT)
    return nullptr;
  const auto *RT = PT->getPointeeType()->getAs<RecordType>();
  if (!RT)
    return nullptr;
  if (Self)
    *Self = P;
  return RT->getDecl();
}

bool Sema::isCxImplicitSelfMember(const DeclarationNameInfo &NameInfo) {
  if (!getLangOpts().CX)
    return false;
  RecordDecl *RD = getCxReceiverRecord();
  if (!RD)
    return false;
  if (LookupCxMethod(RD, NameInfo.getName()))
    return true;
  LookupResult Fields(*this, NameInfo, LookupMemberName);
  Fields.suppressDiagnostics();
  LookupQualifiedName(Fields, RD);
  return !Fields.empty();
}

bool Sema::isCxReceiverLookup(const LookupResult &R) {
  FunctionDecl *Method = getCurrentCxMethod();
  return Method && llvm::none_of(R, [&](const NamedDecl *D) {
           return Method->Encloses(D->getDeclContext());
         });
}

ExprResult
Sema::BuildCxImplicitSelfMemberRef(const DeclarationNameInfo &NameInfo,
                                   Scope *S) {
  ParmVarDecl *Self = nullptr;
  RecordDecl *RD = getCxReceiverRecord(&Self);
  if (!RD)
    return ExprEmpty();

  FunctionDecl *Sibling = LookupCxMethod(RD, NameInfo.getName());
  if (!Sibling) {
    LookupResult Fields(*this, NameInfo, LookupMemberName);
    Fields.suppressDiagnostics();
    LookupQualifiedName(Fields, RD);
    if (Fields.empty())
      return ExprEmpty();
  }

  ExprResult Base = BuildDeclRefExpr(
      Self, Self->getType(), VK_LValue,
      DeclarationNameInfo(Self->getDeclName(), NameInfo.getLoc()),
      NestedNameSpecifierLoc());
  if (Base.isInvalid())
    return ExprError();

  // A method is an associated function, so ordinary member lookup does not
  // reach it and the reference has to be built here -- the same way
  // `self.method` builds it.
  if (Sibling)
    return BuildCxMethodRef(Base.get(), /*IsArrow=*/true, NameInfo.getLoc(),
                            Sibling, NameInfo);

  CXXScopeSpec SS;
  return BuildMemberReferenceExpr(Base.get(), Self->getType(),
                                  NameInfo.getLoc(), /*IsArrow=*/true, SS,
                                  SourceLocation(),
                                  /*FirstQualifierInScope=*/nullptr, NameInfo,
                                  /*TemplateArgs=*/nullptr, S);
}

bool Sema::isCxContinuationOf(const TagDecl *Def, SourceLocation Loc) {
  if (!getLangOpts().CX || !isa_and_nonnull<RecordDecl>(Def))
    return false;

  // A continuation and the primary definition must belong to the same module.
  // Without a module the two must be the same file: an ordinary C header's
  // types stay C types, and the unnamed-module rule is still open (G04).
  const IdentifierInfo *Here = Context.getCxModuleOwner(Loc);
  const IdentifierInfo *There = Context.getCxModuleOwner(Def->getLocation());
  if (Here && There)
    return Here == There;
  if (Here || There)
    return false;
  SourceManager &SM = Context.getSourceManager();
  return SM.getFileID(SM.getExpansionLoc(Loc)) ==
         SM.getFileID(SM.getExpansionLoc(Def->getLocation()));
}


//===----------------------------------------------------------------------===//
// Access control
//===----------------------------------------------------------------------===//

void Sema::AddCxAccess(Decl *Member, std::optional<unsigned> Read,
                       std::optional<unsigned> Write, bool InContinuation,
                       SourceLocation Loc) {
  if (!Member || !getLangOpts().CX)
    return;

  // Primary members are public. A member introduced only in a continuation is
  // an implementation detail, so it is private unless it says otherwise.
  unsigned R = Read.value_or(InContinuation ? CxAccess_private
                                            : CxAccess_public);
  unsigned W = Write.value_or(R);

  // Write visibility cannot be broader than read visibility: a member nobody
  // can read but anybody can write is not an interface.
  if (W < R) {
    Diag(Loc, diag::err_cx_access_wider_write);
    W = R;
  }

  // An implementation keeps the access of the declaration it implements.
  if (const auto *FD = dyn_cast<FunctionDecl>(Member))
    if (const FunctionDecl *Prev = FD->getPreviousDecl())
      if (const auto *A = Prev->getAttr<CxAccessAttr>()) {
        R = A->getRead();
        W = A->getWrite();
      }

  // Public is the default everywhere it is not restricted, so recording it
  // would only add noise to every member of every struct.
  if (R == CxAccess_public && W == CxAccess_public)
    return;

  Member->addAttr(CxAccessAttr::CreateImplicit(Context, R, W));
}

/// The record a Cx member belongs to, or null.
static const RecordDecl *getCxMemberRecord(const NamedDecl *Member) {
  return dyn_cast_or_null<RecordDecl>(Member->getDeclContext());
}

bool Sema::isCxMemberAccessible(const NamedDecl *Member, SourceLocation Loc,
                                bool ForWrite) {
  return !cxMemberAccessLevelViolated(Member, Loc, ForWrite).has_value();
}

/// The access level \p Member would violate at \p Loc, or nothing when the
/// access is permitted. Shared by the check that diagnoses and by code
/// completion, which must not.
std::optional<unsigned>
Sema::cxMemberAccessLevelViolated(const NamedDecl *Member, SourceLocation Loc,
                                  bool ForWrite) {
  if (!getLangOpts().CX || !Member)
    return std::nullopt;
  const auto *A = Member->getAttr<CxAccessAttr>();
  if (!A)
    return std::nullopt;
  unsigned Level = ForWrite ? A->getWrite() : A->getRead();
  if (Level == CxAccess_public)
    return std::nullopt;

  const RecordDecl *RD = getCxMemberRecord(Member);
  if (!RD)
    return std::nullopt;

  // Access is decided by the declaration's ownership, not by the module of
  // whoever included the header.
  const IdentifierInfo *Owner =
      Context.getCxModuleOwner(RD->getCanonicalDecl()->getLocation());

  // Anything lexically inside the type -- a method body, or a continuation --
  // is the type's own implementation.
  for (const DeclContext *DC = CurContext; DC; DC = DC->getLexicalParent()) {
    if (DC->getPrimaryContext() == RD->getPrimaryContext())
      return std::nullopt;
    if (const auto *FD = dyn_cast<FunctionDecl>(DC))
      if (FD->hasAttr<CxMethodAttr>() &&
          isa<RecordDecl>(FD->getDeclContext()) &&
          cast<RecordDecl>(FD->getDeclContext())->getPrimaryContext() ==
              RD->getPrimaryContext())
        return std::nullopt;
  }

  // Internal additionally admits any code in the owning module.
  if (Level == CxAccess_internal && Owner &&
      Context.getCxModuleOwner(Loc) == Owner)
    return std::nullopt;

  return Level;
}

bool Sema::CheckCxMemberAccess(const NamedDecl *Member, SourceLocation Loc,
                               bool ForWrite) {
  std::optional<unsigned> Level =
      cxMemberAccessLevelViolated(Member, Loc, ForWrite);
  if (!Level)
    return false;

  Diag(Loc, diag::err_cx_member_inaccessible) << Member << *Level << ForWrite;
  Diag(Member->getLocation(), diag::note_cx_member_declared_here)
      << *Level << Member;
  return true;
}

bool Sema::CheckCxWriteAccessChain(const Expr *E) {
  if (!getLangOpts().CX || !E)
    return false;
  E = E->IgnoreParenImpCasts();
  // The member as written, which names an anonymous member it goes through.
  const NamedDecl *Written = nullptr;
  SourceLocation WrittenLoc;
  while (true) {
    if (const auto *ME = dyn_cast<MemberExpr>(E)) {
      // A method reference is rejected as a value elsewhere.
      const auto *Field = dyn_cast<FieldDecl>(ME->getMemberDecl());
      if (Field && !Written) {
        Written = Field;
        WrittenLoc = ME->getMemberLoc();
      }
      if (Field && Field->isAnonymousStructOrUnion()) {
        if (std::optional<unsigned> Level = cxMemberAccessLevelViolated(
                Field, WrittenLoc, /*ForWrite=*/true)) {
          Diag(WrittenLoc, diag::err_cx_member_inaccessible)
              << Written << *Level << /*ForWrite=*/true;
          Diag(Field->getLocation(), diag::note_cx_member_declared_here)
              << *Level << Written;
          return true;
        }
      } else if (Field && CheckCxMemberAccess(Field, ME->getMemberLoc(),
                                              /*ForWrite=*/true)) {
        return true;
      }
      if (ME->isArrow())
        return false;
      E = ME->getBase()->IgnoreParens();
      continue;
    }
    if (const auto *ASE = dyn_cast<ArraySubscriptExpr>(E)) {
      // An element of an array member is part of that member; an element
      // reached through a pointer is part of another object.
      const auto *Decay =
          dyn_cast<ImplicitCastExpr>(ASE->getBase()->IgnoreParens());
      if (!Decay || Decay->getCastKind() != CK_ArrayToPointerDecay)
        return false;
      E = Decay->getSubExpr()->IgnoreParens();
      continue;
    }
    return false;
  }
}

bool Sema::hasCxFieldDefaults(QualType T) {
  if (!getLangOpts().CX || T.isNull())
    return false;
  const auto *RD = Context.getBaseElementType(T)->getAsRecordDecl();
  if (!RD || !(RD = RD->getDefinition()))
    return false;
  // A C record cannot contain itself by value, so this terminates.
  return llvm::any_of(RD->fields(), [&](const FieldDecl *FD) {
    return FD->hasInClassInitializer() || hasCxFieldDefaults(FD->getType());
  });
}

//===----------------------------------------------------------------------===//
// Generated memberwise construction
//===----------------------------------------------------------------------===//

/// A variable with automatic storage that \p E evaluates, other than one \p E
/// declares itself (a statement expression's bindings).
static const VarDecl *findCxEvaluatedLocal(const Stmt *S, SourceRange Own) {
  if (!S)
    return nullptr;
  if (const auto *DRE = dyn_cast<DeclRefExpr>(S))
    if (const auto *VD = dyn_cast<VarDecl>(DRE->getDecl()))
      if (VD->hasLocalStorage() && DRE->isNonOdrUse() != NOUR_Unevaluated &&
          !Own.fullyContains(VD->getSourceRange()))
        return VD;
  for (const Stmt *Child : S->children())
    if (const VarDecl *VD = findCxEvaluatedLocal(Child, Own))
      return VD;
  return nullptr;
}

void Sema::AddCxFieldDefault(Decl *Field, SourceLocation EqualLoc,
                             ExprResult Init) {
  auto *FD = dyn_cast_or_null<FieldDecl>(Field);
  if (!FD) {
    if (Field)
      Diag(EqualLoc, diag::err_cx_field_default_not_field);
    return;
  }
  if (Init.isInvalid()) {
    FD->setInvalidDecl();
    return;
  }

  // A default is evaluated wherever the type is constructed -- including in
  // its own methods and in other functions -- where a local of the function
  // that declares a local type does not exist.
  if (const VarDecl *Local =
          findCxEvaluatedLocal(Init.get(), Init.get()->getSourceRange())) {
    Diag(Init.get()->getExprLoc(), diag::err_cx_field_default_local) << Local;
    Diag(Local->getLocation(), diag::note_declared_at);
    FD->setInvalidDecl();
    return;
  }

  // The default is an ordinary initialization of the field, checked here so
  // that a bad one is reported where it is written rather than at every
  // construction that relies on it.
  InitializedEntity Entity = InitializedEntity::InitializeMember(FD);
  ExprResult Converted =
      PerformCopyInitialization(Entity, EqualLoc, Init.get());
  if (Converted.isInvalid()) {
    FD->setInvalidDecl();
    return;
  }
  FD->setInClassInitializer(Converted.get());
}

ParsedType Sema::getCxImplicitTagType(const IdentifierInfo &II,
                                      SourceLocation Loc, Scope *S) {
  if (!getLangOpts().CX)
    return nullptr;
  // The tag first: it is the rare case, and looking it up has no effect on
  // the translation unit.
  LookupResult Tags(*this, &II, Loc, LookupTagName);
  Tags.suppressDiagnostics();
  LookupName(Tags, S);
  auto *Tag = Tags.getAsSingle<TagDecl>();
  if (!Tag)
    return nullptr;
  // Anything ordinary with this name wins, including a library builtin that a
  // C call would implicitly declare. Only now may the lookup declare one.
  LookupResult Ordinary(*this, &II, Loc, LookupOrdinaryName);
  Ordinary.suppressDiagnostics();
  LookupName(Ordinary, S, /*AllowBuiltinCreation=*/true);
  if (!Ordinary.empty())
    return nullptr;
  QualType T = Context.getTypeDeclType(ElaboratedTypeKeyword::None,
                                       /*Qualifier=*/std::nullopt, Tag);
  // A payload enum's name is the type of its values.
  T = getCxTagSpellingType(Tag, T);
  TypeLocBuilder TLB;
  auto TL = TLB.push<TagTypeLoc>(T);
  TL.setElaboratedKeywordLoc(SourceLocation());
  TL.setQualifierLoc(NestedNameSpecifierLoc());
  TL.setNameLoc(Loc);
  return CreateParsedType(T, TLB.getTypeSourceInfo(Context, T));
}

ParsedType Sema::getCxConstructionType(const IdentifierInfo *II,
                                       SourceLocation Loc, Scope *S,
                                       bool AllowImplicitTag) {
  if (!getLangOpts().CX || !II)
    return nullptr;
  ParsedType T = getTypeName(*II, Loc, S);
  if (!T && AllowImplicitTag)
    T = getCxImplicitTagType(*II, Loc, S);
  if (!T)
    return nullptr;
  // Unions are accepted here and rejected in ActOnCxConstruction, so the
  // diagnostic explains the rule instead of falling back to "not an
  // expression".
  QualType QT = GetTypeFromParser(T);
  // The values of a payload enum are built through its cases.
  const auto *RT = QT->getAs<RecordType>();
  return RT && !RT->getDecl()->hasAttr<CxPayloadEnumAttr>() ? T : nullptr;
}

FunctionDecl *Sema::getCxFirstInitializer(const RecordDecl *RD) const {
  for (Decl *D : RD->decls())
    if (auto *FD = dyn_cast<FunctionDecl>(D))
      if (isCxInit(FD))
        return FD;
  return nullptr;
}

/// Whether \p RD declares a custom initializer, which replaces the generated
/// memberwise surface entirely.
static bool hasCxInitializer(const RecordDecl *RD) {
  for (Decl *D : RD->decls())
    if (const auto *FD = dyn_cast<FunctionDecl>(D))
      if (isCxInit(FD))
        return true;
  return false;
}

FunctionDecl *Sema::resolveCxInit(RecordDecl *RD, QualType T,
                                  SourceLocation Loc,
                                  ArrayRef<const IdentifierInfo *> Labels,
                                  MutableArrayRef<Expr *> AllArgs) {
  // The initializers are an overload set like any other; the receiver is the
  // first argument, which is what lets the ordinary resolution apply.
  FunctionDecl *Init = nullptr;
  {
    CxCallLabelScope LabelScope(*this, Labels);
    OverloadCandidateSet Candidates(Loc, OverloadCandidateSet::CSK_Normal);
    llvm::SmallPtrSet<const FunctionDecl *, 4> Seen;
    for (Decl *D : RD->decls()) {
      auto *FD = dyn_cast<FunctionDecl>(D);
      if (!FD || !isCxInit(FD) || !Seen.insert(FD->getCanonicalDecl()).second)
        continue;
      AddOverloadCandidate(FD, DeclAccessPair::make(FD, FD->getAccess()),
                           AllArgs, Candidates);
    }

    OverloadCandidateSet::iterator Best;
    switch (Candidates.BestViableFunction(*this, Loc, Best)) {
    case OR_Success:
      Init = Best->Function;
      break;
    case OR_No_Viable_Function:
      Candidates.NoteCandidates(
          PartialDiagnosticAt(Loc,
                              PDiag(diag::err_cx_no_viable_initializer) << T),
          *this, OCD_AllCandidates, AllArgs);
      return nullptr;
    case OR_Ambiguous:
      Candidates.NoteCandidates(
          PartialDiagnosticAt(Loc,
                              PDiag(diag::err_cx_ambiguous_initializer) << T),
          *this, OCD_AmbiguousCandidates, AllArgs);
      return nullptr;
    case OR_Deleted:
      return nullptr;
    }
  }

  // Construction is bounded by the initializer it selects, not by the fields.
  if (CheckCxMemberAccess(Init, Loc, /*ForWrite=*/false))
    return nullptr;
  return Init;
}

ExprResult Sema::BuildCxInitConstruction(QualType T, RecordDecl *RD,
                                         SourceLocation TypeLoc,
                                         SourceLocation LParenLoc,
                                         ArrayRef<const IdentifierInfo *> Labels,
                                         ArrayRef<SourceLocation> LabelLocs,
                                         MultiExprArg Args,
                                         SourceLocation RParenLoc) {
  // An initializer is code that runs, so the construction is a statement
  // expression and needs a function to run in.
  if (!CurContext->isFunctionOrMethod() || !getCurFunction()) {
    Diag(TypeLoc, diag::err_cx_init_at_file_scope) << T;
    return ExprError();
  }

  // The object under construction. Declaration defaults still participate,
  // so it starts out holding them.
  VarDecl *Object = VarDecl::Create(
      Context, CurContext, TypeLoc, TypeLoc, &Context.Idents.get("__cx_object"),
      T, Context.getTrivialTypeSourceInfo(T, TypeLoc), SC_None);
  Object->setImplicit();

  SmallVector<Expr *, 8> Defaults;
  for (FieldDecl *FD : RD->fields()) {
    if (FD->getType()->isIncompleteArrayType() || FD->isUnnamedBitField())
      continue;
    if (Expr *Default = FD->getInClassInitializer())
      Defaults.push_back(Default);
    else if (hasCxFieldDefaults(FD->getType())) {
      // A member with defaults of its own starts out holding them. The list
      // is typed by the initialization, as the parser's lists are.
      auto *Empty = new (Context)
          InitListExpr(Context, TypeLoc, {}, TypeLoc, /*isExplicit=*/false);
      Empty->setType(Context.VoidTy);
      Defaults.push_back(Empty);
    }
    else
      // Every initializer writes this field before it can be read; zero
      // keeps the padding and the bytes around it deterministic.
      Defaults.push_back(new (Context) ImplicitValueInitExpr(FD->getType()));
  }
  if (!Defaults.empty()) {
    // Access was not checked here: none of these values is written by the
    // call, they are the type's own defaults.
    llvm::SaveAndRestore<bool> Building(CxBuildingConstruction, true);
    ExprResult Init = ActOnInitList(LParenLoc, Defaults, RParenLoc);
    if (Init.isInvalid())
      return ExprError();
    AddInitializerToDecl(Object, Init.get(), /*DirectInit=*/false);
    if (Object->isInvalidDecl())
      return ExprError();
  }

  ExprResult Receiver = BuildDeclRefExpr(Object, T, VK_LValue, TypeLoc);
  if (Receiver.isInvalid())
    return ExprError();
  ExprResult SelfArg = CreateBuiltinUnaryOp(TypeLoc, UO_AddrOf, Receiver.get());
  if (SelfArg.isInvalid())
    return ExprError();

  SmallVector<Expr *, 8> AllArgs;
  AllArgs.push_back(SelfArg.get());
  AllArgs.append(Args.begin(), Args.end());

  FunctionDecl *Init = resolveCxInit(RD, T, TypeLoc, Labels, AllArgs);
  if (!Init)
    return ExprError();

  ExprResult Fn = BuildDeclRefExpr(
      Init, Init->getType(), VK_PRValue,
      DeclarationNameInfo(Init->getDeclName(), TypeLoc),
      NestedNameSpecifierLoc());
  if (Fn.isInvalid())
    return ExprError();
  ExprResult Call =
      BuildResolvedCallExpr(Fn.get(), Init, LParenLoc, AllArgs, RParenLoc);
  if (Call.isInvalid())
    return ExprError();
  CheckCxArgumentLabels(Call.get(), Labels, LabelLocs);

  // The object is read once, into the construction's value: a move.
  llvm::SaveAndRestore<bool> Moving(CxMovingValue, true);
  ExprResult Value = ActOnStmtExprResult(
      BuildDeclRefExpr(Object, T, VK_LValue, RParenLoc));
  if (Value.isInvalid())
    return ExprError();

  // Declare the object, run the initializer over it, produce it. A statement
  // expression is what C already has for "these statements, then this value",
  // so no new AST node and no new lowering are needed.
  Stmt *Body[] = {new (Context) DeclStmt(DeclGroupRef(Object), TypeLoc,
                                         RParenLoc),
                  Call.get(), Value.get()};
  ActOnStartStmtExpr();
  return BuildStmtExpr(LParenLoc,
                       CompoundStmt::Create(Context, Body, FPOptionsOverride(),
                                            LParenLoc, RParenLoc),
                       RParenLoc, /*TemplateDepth=*/0);
}

//===----------------------------------------------------------------------===//
// Resource types
//===----------------------------------------------------------------------===//

FunctionDecl *Sema::getCxDeinit(const RecordDecl *RD) const {
  if (!RD)
    return nullptr;
  for (Decl *D : RD->decls())
    if (auto *FD = dyn_cast<FunctionDecl>(D))
      if (isCxDeinitDecl(FD))
        return FD;
  return nullptr;
}

bool Sema::isCxResourceType(QualType T) const {
  if (!getLangOpts().CX || T.isNull())
    return false;
  const RecordDecl *RD = Context.getBaseElementType(T)->getAsRecordDecl();
  if (!RD || !(RD = RD->getDefinition()))
    return false;
  return getCxDeinit(RD);
}

bool Sema::CheckCxResourceStorage(QualType T, SourceLocation Loc,
                                  unsigned Kind) {
  if (!isCxResourceType(T))
    return false;
  Diag(Loc, diag::err_cx_resource_storage)
      << Kind << Context.getBaseElementType(T).getUnqualifiedType();
  return true;
}

void Sema::completeCxRecordValueOperations(RecordDecl *RD) {
  if (!getLangOpts().CX || RD->isInvalidDecl())
    return;
  bool ResourceField = false;
  for (FieldDecl *FD : RD->fields()) {
    if (!isCxResourceType(FD->getType()))
      continue;
    // A union does not know which member is alive, so it cannot destroy one.
    if (RD->isUnion()) {
      Diag(FD->getLocation(), diag::err_cx_resource_in_union)
          << Context.getBaseElementType(FD->getType()).getUnqualifiedType();
      FD->setInvalidDecl();
      continue;
    }
    ResourceField = true;
  }
  if (!ResourceField || getCxDeinit(RD) || RD->isUnion())
    return;

  // Destroying the fields is all an implicit deinit does; CodeGen adds that
  // to every deinit.
  QualType SelfTy = Context.getPointerType(Context.getCanonicalTagType(RD));
  FunctionProtoType::ExtProtoInfo EPI;
  QualType FnTy = Context.getFunctionType(Context.VoidTy, {SelfTy}, EPI);
  SourceLocation Loc = RD->getEndLoc();
  FunctionDecl *FD = FunctionDecl::Create(
      Context, RD, Loc, Loc, &Context.Idents.get("deinit"), FnTy,
      Context.getTrivialTypeSourceInfo(FnTy, Loc), SC_None,
      getCurFPFeatures().isFPConstrained(), /*isInlineSpecified=*/false,
      /*hasWrittenPrototype=*/true);
  ParmVarDecl *Self = ParmVarDecl::Create(
      Context, FD, Loc, Loc, &Context.Idents.get("self"), SelfTy,
      Context.getTrivialTypeSourceInfo(SelfTy, Loc), SC_None, nullptr);
  Self->setImplicit();
  Self->setScopeInfo(0, 0);
  FD->setParams({Self});
  FD->setImplicit();
  FD->setAccess(AS_public);
  FD->addAttr(CxMethodAttr::CreateImplicit(Context));
  const IdentifierInfo *Owner = Context.getCxModuleOwner(RD->getLocation());
  FD->addAttr(CxLinkageAttr::CreateImplicit(
      Context, const_cast<IdentifierInfo *>(Owner)));
  FD->addAttr(OverloadableAttr::CreateImplicit(Context));
  FD->setBody(CompoundStmt::Create(Context, {}, FPOptionsOverride(), Loc, Loc));
  RD->addDecl(FD);
  // Defined in its record, like a method with a body.
  Consumer.HandleInlineFunctionDefinition(FD);
}

void Sema::CheckCxResourceVar(VarDecl *VD) {
  if (!getLangOpts().CX || VD->isInvalidDecl() || isa<ParmVarDecl>(VD) ||
      !isCxResourceType(VD->getType()))
    return;
  // Static storage was diagnosed when the variable was declared.
  if (!VD->hasLocalStorage())
    return;
  // Its deinit runs when the scope ends, so it must hold a value by then.
  if (!VD->getInit()) {
    CheckCxResourceStorage(VD->getType(), VD->getLocation(), 1);
    VD->setInvalidDecl();
  }
}

ExprResult Sema::ActOnCxInitInPlace(Expr *Ptr, SourceLocation InitLoc,
                                    SourceLocation LParenLoc,
                                    ArrayRef<const IdentifierInfo *> Labels,
                                    ArrayRef<SourceLocation> LabelLocs,
                                    MultiExprArg Args,
                                    SourceLocation RParenLoc) {
  ExprResult P = DefaultFunctionArrayLvalueConversion(Ptr);
  if (P.isInvalid())
    return ExprError();
  QualType PT = P.get()->getType();
  if (!PT->isPointerType()) {
    Diag(InitLoc, diag::err_cx_init_in_place_value);
    return ExprError();
  }
  QualType T = PT->getPointeeType().getUnqualifiedType();
  if (!CurContext->isFunctionOrMethod() || !getCurFunction()) {
    Diag(InitLoc, diag::err_cx_init_at_file_scope) << T;
    return ExprError();
  }

  // `({ T *__cx_place = p; *__cx_place = T(...); })`: the value is built as a
  // construction is and moved into the storage, whose old bytes are not a
  // value, so nothing is destroyed.
  QualType PlaceTy = Context.getPointerType(T);
  VarDecl *Place = VarDecl::Create(
      Context, CurContext, InitLoc, InitLoc, &Context.Idents.get("__cx_place"),
      PlaceTy, Context.getTrivialTypeSourceInfo(PlaceTy, InitLoc), SC_None);
  Place->setImplicit();
  ExprResult PInit = ImpCastExprToType(P.get(), PlaceTy, CK_NoOp);
  AddInitializerToDecl(Place, PInit.get(), /*DirectInit=*/false);
  if (Place->isInvalidDecl())
    return ExprError();

  ExprResult Value = ActOnCxConstruction(ParsedType::make(T), InitLoc,
                                         LParenLoc, Labels, LabelLocs, Args,
                                         RParenLoc);
  if (Value.isInvalid())
    return ExprError();
  ExprResult Ref = BuildDeclRefExpr(Place, PlaceTy, VK_LValue, InitLoc);
  ExprResult Target = CreateBuiltinUnaryOp(InitLoc, UO_Deref, Ref.get());
  if (Target.isInvalid())
    return ExprError();
  ExprResult Store =
      CreateBuiltinBinOp(InitLoc, BO_Assign, Target.get(), Value.get());
  if (Store.isInvalid())
    return ExprError();

  // The storage is the value; the expression produces none.
  Store = ImpCastExprToType(Store.get(), Context.VoidTy, CK_ToVoid);
  Stmt *Body[] = {new (Context) DeclStmt(DeclGroupRef(Place), InitLoc,
                                         RParenLoc),
                  Store.get()};
  ActOnStartStmtExpr();
  return BuildStmtExpr(LParenLoc,
                       CompoundStmt::Create(Context, Body, FPOptionsOverride(),
                                            LParenLoc, RParenLoc),
                       RParenLoc, /*TemplateDepth=*/0);
}

ExprResult Sema::ActOnCxInitDelegation(Expr *Self, SourceLocation InitLoc,
                                       SourceLocation LParenLoc,
                                       ArrayRef<const IdentifierInfo *> Labels,
                                       ArrayRef<SourceLocation> LabelLocs,
                                       MultiExprArg Args,
                                       SourceLocation RParenLoc) {
  if (!isCxInitializer(getCurFunctionDecl())) {
    Diag(InitLoc, diag::err_cx_delegation_outside_init);
    return ExprError();
  }
  RecordDecl *RD = getCxReceiverRecord();
  if (!RD)
    return ExprError();

  // The delegated initializer runs over this one's receiver; defaults were
  // applied once, by the construction that called this one.
  SmallVector<Expr *, 8> AllArgs;
  AllArgs.push_back(Self);
  AllArgs.append(Args.begin(), Args.end());
  FunctionDecl *Init =
      resolveCxInit(RD, Context.getCanonicalTagType(RD), InitLoc, Labels,
                    AllArgs);
  if (!Init)
    return ExprError();

  ExprResult Fn = BuildDeclRefExpr(
      Init, Init->getType(), VK_PRValue,
      DeclarationNameInfo(Init->getDeclName(), InitLoc),
      NestedNameSpecifierLoc());
  if (Fn.isInvalid())
    return ExprError();
  ExprResult Call =
      BuildResolvedCallExpr(Fn.get(), Init, LParenLoc, AllArgs, RParenLoc);
  if (!Call.isInvalid())
    CheckCxArgumentLabels(Call.get(), Labels, LabelLocs);
  return Call;
}

bool Sema::isCxInitConstFieldWrite(const Expr *E) {
  if (!getLangOpts().CX || !isCxInitializer(getCurFunctionDecl()))
    return false;
  const auto *ME = dyn_cast<MemberExpr>(E->IgnoreParens());
  if (!ME || !isa<FieldDecl>(ME->getMemberDecl()) ||
      !ME->getType().isConstQualified())
    return false;
  // The field of an anonymous member is reached through it.
  const Expr *Base = ME->getBase()->IgnoreParenImpCasts();
  while (const auto *Inner = dyn_cast<MemberExpr>(Base)) {
    const auto *FD = dyn_cast<FieldDecl>(Inner->getMemberDecl());
    if (!FD || !FD->isAnonymousStructOrUnion())
      return false;
    Base = Inner->getBase()->IgnoreParenImpCasts();
  }
  if (const auto *UO = dyn_cast<UnaryOperator>(Base);
      UO && UO->getOpcode() == UO_Deref)
    Base = UO->getSubExpr();
  return isCxSelfReference(Base);
}

ExprResult Sema::ActOnCxConstruction(ParsedType Ty, SourceLocation TypeLoc,
                                     SourceLocation LParenLoc,
                                     ArrayRef<const IdentifierInfo *> Labels,
                                     ArrayRef<SourceLocation> LabelLocs,
                                     MultiExprArg Args,
                                     SourceLocation RParenLoc) {
  TypeSourceInfo *TInfo = nullptr;
  QualType T = GetTypeFromParser(Ty, &TInfo);
  if (!TInfo)
    TInfo = Context.getTrivialTypeSourceInfo(T, TypeLoc);

  const auto *RT = T->getAs<RecordType>();
  if (!RT || RT->getDecl()->isUnion()) {
    Diag(TypeLoc, diag::err_cx_construction_not_struct) << T;
    return ExprError();
  }
  if (RequireCompleteType(TypeLoc, T, diag::err_cx_construction_incomplete))
    return ExprError();

  RecordDecl *RD = RT->getDecl()->getDefinition();

  // A custom initializer replaces the generated surface entirely: the
  // initializers are the complete construction interface.
  if (hasCxInitializer(RD))
    return BuildCxInitConstruction(T, RD, TypeLoc, LParenLoc, Labels, LabelLocs,
                                   Args, RParenLoc);

  // The generated surface is one labelled value per stored field, in
  // declaration order. Its labels are the field names.
  SmallVector<FieldDecl *, 8> Fields;
  for (FieldDecl *FD : RD->fields()) {
    // A flexible array member has no storage in a value, and an unnamed
    // bit-field is padding: neither is something construction can write.
    if (FD->getType()->isIncompleteArrayType() || FD->isUnnamedBitField())
      continue;
    if (!FD->getIdentifier()) {
      Diag(TypeLoc, diag::err_cx_construction_unnamed_field) << T;
      return ExprError();
    }
    Fields.push_back(FD);
  }

  // The written values are matched against the fields in declaration order.
  // A field with a declaration-site default may be skipped; one without a
  // default has to be given.
  bool Invalid = false;
  SmallVector<Expr *, 8> FieldInits;
  unsigned Next = 0;
  for (FieldDecl *FD : Fields) {
    bool Supplied = Next < Args.size() && Labels[Next] == FD->getIdentifier();
    if (!Supplied) {
      if (Expr *Default = FD->getInClassInitializer()) {
        // A defaulted field is not part of the construction surface, so its
        // access does not bound where the type can be constructed.
        FieldInits.push_back(Default);
        continue;
      }
      if (Next < Args.size())
        // There is a value in this position; it just is not this field's.
        Diag(LabelLocs[Next], diag::err_cx_construction_label)
            << T << FD->getIdentifier()->getName();
      else
        Diag(RParenLoc, diag::err_cx_construction_missing_field) << FD << T;
      Diag(FD->getLocation(), diag::note_cx_construction_field) << FD;
      Invalid = true;
      continue;
    }
    // Construction writes this field, so it needs write access here.
    if (CheckCxMemberAccess(FD, LabelLocs[Next], /*ForWrite=*/true))
      Invalid = true;
    FieldInits.push_back(Args[Next]);
    ++Next;
  }

  if (!Invalid && Next < Args.size()) {
    // Whatever is left over names no field, or names one out of order.
    const IdentifierInfo *Written = Labels[Next];
    if (Written && llvm::any_of(Fields, [&](const FieldDecl *FD) {
          return FD->getIdentifier() == Written;
        }))
      Diag(LabelLocs[Next], diag::err_cx_construction_label)
          << T << Written->getName();
    else
      Diag(LabelLocs[Next], diag::err_cx_construction_unexpected_arg) << T;
    Invalid = true;
  }
  if (Invalid)
    return ExprError();

  // The value is an ordinary initialization of the record, so the usual C
  // conversions and diagnostics apply to each field. Access was checked per
  // field above, on exactly the ones this call writes.
  llvm::SaveAndRestore<bool> Building(CxBuildingConstruction, true);
  Expr *Init = ActOnInitList(LParenLoc, FieldInits, RParenLoc).getAs<Expr>();
  if (!Init)
    return ExprError();
  return BuildCompoundLiteralExpr(LParenLoc, TInfo, RParenLoc, Init);
}

//===----------------------------------------------------------------------===//
// Tuples
//===----------------------------------------------------------------------===//

bool Sema::isCxTupleType(QualType T) const {
  const auto *RT = T.isNull() ? nullptr : T->getAs<RecordType>();
  return RT && RT->getDecl()->hasAttr<CxTupleAttr>();
}

const CxTupleLabelsAttr *Sema::getCxTupleLabels(QualType T) const {
  // Labels are sugar: the first labelled typedef in the chain governs, so
  // `typedef (int id, int) User` keeps them.
  while (!T.isNull()) {
    if (const auto *TT = dyn_cast<TypedefType>(T.getTypePtr()))
      if (const auto *A = TT->getDecl()->getAttr<CxTupleLabelsAttr>())
        return A;
    QualType Next = T.getSingleStepDesugaredType(Context);
    if (Next == T)
      return nullptr;
    T = Next;
  }
  return nullptr;
}

/// Whether the tuple struct \p RD stores exactly \p Elems.
static bool hasCxTupleElements(ASTContext &Ctx, const RecordDecl *RD,
                               ArrayRef<QualType> Elems) {
  unsigned I = 0;
  for (const FieldDecl *FD : RD->fields()) {
    if (I == Elems.size() || !Ctx.hasSameType(FD->getType(), Elems[I]))
      return false;
    ++I;
  }
  return I == Elems.size();
}

QualType Sema::BuildCxTupleType(ArrayRef<QualType> Elems,
                                ArrayRef<const IdentifierInfo *> Labels,
                                ArrayRef<SourceLocation> Locs,
                                SourceLocation Loc) {
  if (Elems.size() < 2) {
    Diag(Loc, diag::err_cx_tuple_one_element);
    return QualType();
  }
  bool HasLabels = false;
  for (unsigned I = 0, E = Elems.size(); I != E; ++I) {
    QualType T = Elems[I];
    if (CheckCxResourceStorage(T, Locs[I], 4))
      return QualType();
    if (T->isVoidType() || T->isFunctionType() ||
        T->isVariablyModifiedType()) {
      Diag(Locs[I], diag::err_cx_tuple_element_type) << T;
      return QualType();
    }
    if (RequireCompleteType(Locs[I], T,
                            diag::err_cx_tuple_element_incomplete))
      return QualType();
    const IdentifierInfo *L = Labels[I];
    if (!L)
      continue;
    HasLabels = true;
    if (L->getName().contains('$')) {
      Diag(Locs[I], diag::err_cx_tuple_label_dollar);
      return QualType();
    }
    for (unsigned J = 0; J != I; ++J)
      if (Labels[J] == L) {
        Diag(Locs[I], diag::err_cx_tuple_duplicate_label) << L;
        return QualType();
      }
  }

  // The struct is shared by every tuple of these element types, and found
  // again by a name no source can spell. That name is only a bucket: two
  // distinct types can print alike, so the elements are compared too. The
  // name goes through the identifier resolver, which is also how a PCH or
  // a preamble brings back the struct it already made.
  TranslationUnitDecl *TU = Context.getTranslationUnitDecl();
  PrintingPolicy Policy = getPrintingPolicy();
  auto TypeName = [&](QualType T) {
    return T.getCanonicalType().getAsString(Policy);
  };
  auto Register = [&](NamedDecl *D) {
    D->setImplicit();
    TU->addDecl(D);
    IdResolver.AddDecl(D);
    if (TUScope)
      TUScope->AddDecl(D);
  };

  std::string Name = "(";
  for (unsigned I = 0, E = Elems.size(); I != E; ++I)
    Name += (I ? ", " : "") + TypeName(Elems[I]);
  Name += ")";
  IdentifierInfo *II = &Context.Idents.get(Name);
  RecordDecl *RD = nullptr;
  for (auto It = IdResolver.begin(II), End = IdResolver.end(); It != End; ++It)
    if (auto *R = dyn_cast<RecordDecl>(*It);
        R && R->hasAttr<CxTupleAttr>() &&
        hasCxTupleElements(Context, R, Elems)) {
      RD = R;
      break;
    }
  if (!RD) {
    RD = RecordDecl::Create(Context, TagTypeKind::Struct, TU, SourceLocation(),
                            SourceLocation(), II);
    RD->addAttr(CxTupleAttr::CreateImplicit(Context));
    RD->startDefinition();
    for (unsigned I = 0, E = Elems.size(); I != E; ++I) {
      QualType T = Elems[I].getCanonicalType();
      auto *FD = FieldDecl::Create(
          Context, RD, SourceLocation(), SourceLocation(),
          &Context.Idents.get("$" + std::to_string(I)), T,
          Context.getTrivialTypeSourceInfo(T), /*BitWidth=*/nullptr,
          /*Mutable=*/false, ICIS_NoInit);
      FD->setAccess(AS_public);
      RD->addDecl(FD);
    }
    RD->completeDefinition();
    Register(RD);
  }
  QualType TupleTy = Context.getTagType(ElaboratedTypeKeyword::None,
                                        /*Qualifier=*/std::nullopt, RD,
                                        /*OwnsTag=*/false);
  if (!HasLabels)
    return TupleTy;

  // The labels: an implicit typedef of the struct, found again the same way.
  SmallVector<const IdentifierInfo *, 4> Written;
  std::string LabelledName = "(";
  for (unsigned I = 0, E = Elems.size(); I != E; ++I) {
    Written.push_back(Labels[I] ? Labels[I] : &Context.Idents.get("_"));
    LabelledName += (I ? ", " : "") + TypeName(Elems[I]);
    if (Labels[I])
      LabelledName += " " + Labels[I]->getName().str();
  }
  LabelledName += ")";
  IdentifierInfo *LabelledII = &Context.Idents.get(LabelledName);
  TypedefDecl *TD = nullptr;
  for (auto It = IdResolver.begin(LabelledII), End = IdResolver.end();
       It != End; ++It) {
    auto *T = dyn_cast<TypedefDecl>(*It);
    const auto *A = T ? T->getAttr<CxTupleLabelsAttr>() : nullptr;
    if (A && Context.hasSameType(T->getUnderlyingType(), TupleTy) &&
        llvm::equal(A->labels(), Written)) {
      TD = T;
      break;
    }
  }
  if (!TD) {
    TD = TypedefDecl::Create(Context, TU, SourceLocation(), SourceLocation(),
                             LabelledII,
                             Context.getTrivialTypeSourceInfo(TupleTy));
    TD->addAttr(
        CxTupleLabelsAttr::CreateImplicit(Context, Written.data(),
                                          Written.size()));
    Register(TD);
  }
  return Context.getTypedefType(ElaboratedTypeKeyword::None,
                                /*Qualifier=*/std::nullopt, TD);
}

/// A literal of tuple type \p T from the elements as written.
static ExprResult BuildCxTupleLiteral(Sema &S, QualType T,
                                      ArrayRef<Expr *> Written,
                                      SourceLocation LParenLoc,
                                      SourceLocation RParenLoc) {
  SmallVector<Expr *, 4> Inits(Written);
  ExprResult List = S.ActOnInitList(LParenLoc, Inits, RParenLoc);
  if (List.isInvalid())
    return ExprError();
  ExprResult Lit = S.BuildCompoundLiteralExpr(
      LParenLoc, S.Context.getTrivialTypeSourceInfo(T, LParenLoc), RParenLoc,
      List.get());
  if (Lit.isUsable())
    S.CxTupleLiteralElems[Lit.get()].assign(Written.begin(), Written.end());
  return Lit;
}

ExprResult Sema::ActOnCxTupleLiteral(SourceLocation LParenLoc,
                                     MultiExprArg Elems,
                                     ArrayRef<const IdentifierInfo *> Labels,
                                     ArrayRef<SourceLocation> Locs,
                                     SourceLocation RParenLoc) {
  SmallVector<Expr *, 4> Written;
  SmallVector<QualType, 4> Types;
  for (Expr *E : Elems) {
    ExprResult R = CheckPlaceholderExpr(E);
    if (R.isInvalid())
      return ExprError();
    Written.push_back(R.get());
    // Each element's type is deduced as for `var`: the value's type after
    // decay, without qualifiers.
    QualType T = R.get()->getType();
    if (T->isArrayType())
      T = Context.getArrayDecayedType(T);
    else if (T->isFunctionType())
      T = Context.getPointerType(T);
    Types.push_back(T.getAtomicUnqualifiedType());
  }
  QualType TupleTy = BuildCxTupleType(Types, Labels, Locs, LParenLoc);
  if (TupleTy.isNull())
    return ExprError();
  return BuildCxTupleLiteral(*this, TupleTy, Written, LParenLoc, RParenLoc);
}

Expr *Sema::retargetCxTupleLiteral(Expr *E, QualType Dest) {
  if (!getLangOpts().CX || !E || !isCxTupleType(Dest))
    return E;
  auto It = CxTupleLiteralElems.find(E->IgnoreParens());
  if (It == CxTupleLiteralElems.end())
    return E;
  SmallVector<Expr *, 4> Written(It->second);
  const RecordDecl *RD = Dest->getAs<RecordType>()->getDecl();
  if (Written.size() != size_t(std::distance(RD->field_begin(),
                                             RD->field_end())))
    return E; // Diagnosed as the incompatible types they are.

  // Labels do not convert anything, but a literal labelled differently from
  // its destination is a mistake, most likely a swap.
  const auto *From = getCxTupleLabels(E->getType());
  const auto *To = getCxTupleLabels(Dest);
  if (From && To)
    for (unsigned I = 0, N = Written.size(); I != N; ++I) {
      const IdentifierInfo *F = From->labels_begin()[I];
      const IdentifierInfo *T = To->labels_begin()[I];
      if (F != T && !F->isStr("_") && !T->isStr("_"))
        Diag(Written[I]->getBeginLoc(), diag::err_cx_tuple_label_mismatch)
            << I << F << Dest << T;
    }
  if (Context.hasSameUnqualifiedType(E->getType(), Dest))
    return E;

  // Each element initializes its field of the destination.
  ExprResult R = BuildCxTupleLiteral(*this, Dest.getUnqualifiedType(),
                                     Written, E->getBeginLoc(), E->getEndLoc());
  if (R.isInvalid())
    return CreateRecoveryExpr(E->getBeginLoc(), E->getEndLoc(), Written, Dest)
        .get();
  return R.get();
}

bool Sema::isCxTupleInitContext(Decl *D) {
  auto *VD = dyn_cast_or_null<VarDecl>(D);
  if (!VD)
    return false;
  QualType T = VD->getType();
  if (const auto *AT = dyn_cast<AutoType>(T.getTypePtr()))
    return AT->isCxInference();
  return isCxTupleType(T);
}

bool Sema::isCxTupleArgument(Expr *Callee, unsigned Index) {
  if (!getLangOpts().CX || !Callee)
    return false;
  Callee = Callee->IgnoreParenImpCasts();
  auto HasTupleParam = [&](const NamedDecl *ND, bool ThroughReceiver) {
    const auto *FD = dyn_cast<FunctionDecl>(ND->getUnderlyingDecl());
    if (!FD)
      return false;
    unsigned I = Index + (ThroughReceiver ? getCxReceiverOffset(FD) : 0);
    return I < FD->getNumParams() &&
           isCxTupleType(FD->getParamDecl(I)->getType());
  };
  if (auto *DRE = dyn_cast<DeclRefExpr>(Callee);
      DRE && isa<FunctionDecl>(DRE->getDecl()))
    return HasTupleParam(DRE->getDecl(), false);
  if (auto *ME = dyn_cast<MemberExpr>(Callee);
      ME && isa<FunctionDecl>(ME->getMemberDecl()))
    return HasTupleParam(ME->getMemberDecl(), true);
  if (auto *OE = dyn_cast<OverloadExpr>(Callee))
    return llvm::any_of(OE->decls(), [&](const NamedDecl *ND) {
      return HasTupleParam(ND, false);
    });
  // A call through a function or block pointer.
  QualType T = Callee->getType();
  if (T.isNull())
    return false;
  if (const auto *PT = T->getAs<PointerType>())
    T = PT->getPointeeType();
  else if (const auto *BPT = T->getAs<BlockPointerType>())
    T = BPT->getPointeeType();
  if (const auto *FPT = T->getAs<FunctionProtoType>())
    return Index < FPT->getNumParams() &&
           isCxTupleType(FPT->getParamType(Index));
  return false;
}

void Sema::TranslateCxTupleLabel(QualType BaseTy,
                                 DeclarationNameInfo &NameInfo) {
  const IdentifierInfo *II = NameInfo.getName().getAsIdentifierInfo();
  const CxTupleLabelsAttr *A = getCxTupleLabels(BaseTy);
  if (!II || !A || II->isStr("_"))
    return;
  unsigned I = 0;
  for (const IdentifierInfo *L : A->labels()) {
    if (L == II) {
      NameInfo.setName(&Context.Idents.get("$" + std::to_string(I)));
      return;
    }
    ++I;
  }
}

Sema::DeclGroupPtrTy Sema::ActOnCxDestructuring(
    Scope *S, DeclSpec &DS, ArrayRef<IdentifierInfo *> Names,
    ArrayRef<SourceLocation> Locs, SourceLocation LParenLoc, Expr *Init) {
  if (!CurContext->isFunctionOrMethod()) {
    Diag(LParenLoc, diag::err_cx_destructure_file_scope);
    return nullptr;
  }
  QualType T = Init->getType();
  if (!isCxTupleType(T)) {
    Diag(Init->getBeginLoc(), diag::err_cx_destructure_not_tuple) << T;
    return nullptr;
  }
  SmallVector<FieldDecl *, 4> Fields(T->getAs<RecordType>()->getDecl()->fields());
  if (Fields.size() != Names.size()) {
    Diag(LParenLoc, diag::err_cx_destructure_count)
        << unsigned(Fields.size()) << unsigned(Names.size());
    return nullptr;
  }

  // The initializer is evaluated once, into a variable no source can name.
  SmallVector<Decl *, 4> Decls;
  QualType HiddenTy = T.getUnqualifiedType();
  auto *Hidden = VarDecl::Create(
      Context, CurContext, LParenLoc, LParenLoc, /*Id=*/nullptr, HiddenTy,
      Context.getTrivialTypeSourceInfo(HiddenTy, LParenLoc), SC_None);
  Hidden->setImplicit();
  Hidden->setReferenced();
  CurContext->addHiddenDecl(Hidden);
  AddInitializerToDecl(Hidden, Init, /*DirectInit=*/false);
  if (Hidden->isInvalidDecl())
    return nullptr;
  Decls.push_back(Hidden);

  bool IsLet = DS.getCxInferenceKind() == DeclSpec::CxInf_let;
  for (unsigned I = 0, E = Names.size(); I != E; ++I) {
    if (!Names[I])
      continue; // `_` discards the element.
    QualType ET = Fields[I]->getType();
    if (IsLet)
      ET.addConst();
    auto *VD = VarDecl::Create(Context, CurContext, Locs[I], Locs[I],
                               Names[I], ET,
                               Context.getTrivialTypeSourceInfo(ET, Locs[I]),
                               SC_None);
    LookupResult Previous(*this, Names[I], Locs[I], LookupOrdinaryName,
                          forRedeclarationInCurContext());
    LookupName(Previous, S);
    FilterLookupForScope(Previous, CurContext, S, /*ConsiderLinkage=*/false,
                         /*AllowInlineNamespace=*/false);
    if (!Previous.empty()) {
      Diag(Locs[I], diag::err_redefinition) << Names[I];
      Diag(Previous.getRepresentativeDecl()->getLocation(),
           diag::note_previous_definition);
      VD->setInvalidDecl();
    }
    ExprResult Ref = BuildDeclRefExpr(Hidden, HiddenTy, VK_LValue, Locs[I]);
    CXXScopeSpec SS;
    ExprResult Elem = BuildMemberReferenceExpr(
        Ref.get(), HiddenTy, Locs[I], /*IsArrow=*/false, SS, SourceLocation(),
        /*FirstQualifierInScope=*/nullptr,
        DeclarationNameInfo(Fields[I]->getDeclName(), Locs[I]),
        /*TemplateArgs=*/nullptr, S);
    PushOnScopeChains(VD, S);
    if (Elem.isUsable())
      AddInitializerToDecl(VD, Elem.get(), /*DirectInit=*/false);
    else
      VD->setInvalidDecl();
    FinalizeDeclaration(VD);
    Decls.push_back(VD);
  }
  return BuildDeclaratorGroup(Decls);
}

/// The case \p Name of Cx enum \p ED, marked referenced; null after a
/// diagnostic.
static EnumConstantDecl *lookupCxEnumCase(Sema &S, EnumDecl *ED,
                                          IdentifierInfo *Name,
                                          SourceLocation NameLoc) {
  for (NamedDecl *D : ED->lookup(Name))
    if (auto *ECD = dyn_cast<EnumConstantDecl>(D)) {
      if (S.DiagnoseUseOfDecl(ECD, NameLoc))
        return nullptr;
      S.MarkAnyDeclReferenced(NameLoc, ECD, /*OdrUse=*/false);
      return ECD;
    }
  S.Diag(NameLoc, diag::err_cx_enum_no_case)
      << Name << S.Context.getCanonicalTagType(ED);
  return nullptr;
}

SmallVector<QualType, 4> Sema::getCxPayloadElementTypes(EnumDecl *ED,
                                                      IdentifierInfo *Name) {
  SmallVector<QualType, 4> Types;
  for (NamedDecl *D : ED->lookup(Name)) {
    const auto *ECD = dyn_cast<EnumConstantDecl>(D);
    const auto *A = ECD ? ECD->getAttr<CxEnumPayloadAttr>() : nullptr;
    if (!A)
      continue;
    if (A->labels_size() == 1) {
      Types.push_back(A->getPayload());
    } else if (const RecordDecl *RD = A->getPayload()->getAsRecordDecl()) {
      for (const FieldDecl *FD : RD->fields())
        Types.push_back(FD->getType());
    }
    break;
  }
  return Types;
}

/// A value of payload enum \p ED holding case \p ECD: `(Token){ tag,
/// .$payload.case = Payload }`, or only the tag when \p Payload is null.
static ExprResult buildCxEnumValue(Sema &S, EnumDecl *ED, EnumConstantDecl *ECD,
                                   SourceLocation Loc, Expr *Payload,
                                   SourceLocation EndLoc) {
  ASTContext &Ctx = S.Context;
  QualType T = Ctx.getTagType(ElaboratedTypeKeyword::None, std::nullopt,
                              S.getCxPayloadRecord(ED), /*OwnsTag=*/false);
  SmallVector<Expr *, 2> Inits;
  Inits.push_back(S.BuildDeclRefExpr(ECD, ECD->getType(), VK_PRValue, Loc));
  if (Payload) {
    DesignatedInitExpr::Designator Path[] = {
        DesignatedInitExpr::Designator::CreateFieldDesignator(
            &Ctx.Idents.get("$payload"), Loc, Loc),
        DesignatedInitExpr::Designator::CreateFieldDesignator(
            ECD->getIdentifier(), Loc, Loc)};
    Inits.push_back(DesignatedInitExpr::Create(Ctx, Path, {}, Loc,
                                               /*GNUSyntax=*/false, Payload));
  }
  ExprResult List = S.ActOnInitList(Loc, Inits, EndLoc);
  if (List.isInvalid())
    return ExprError();
  return S.BuildCompoundLiteralExpr(Loc, Ctx.getTrivialTypeSourceInfo(T, Loc),
                                    EndLoc, List.get());
}

/// Add the public field \p Name of type \p T to \p RD, as tuples do.
static void addCxField(ASTContext &Ctx, RecordDecl *RD, IdentifierInfo *Name,
                       QualType T) {
  auto *FD = FieldDecl::Create(Ctx, RD, SourceLocation(), SourceLocation(),
                               Name, T, Ctx.getTrivialTypeSourceInfo(T),
                               /*BitWidth=*/nullptr, /*Mutable=*/false,
                               ICIS_NoInit);
  FD->setAccess(AS_public);
  RD->addDecl(FD);
}

bool Sema::isCxPayloadEnum(const EnumDecl *ED) const {
  return ED && ED->hasAttr<CxPayloadRecordAttr>();
}

RecordDecl *Sema::getCxPayloadRecord(const EnumDecl *ED) const {
  const auto *A = ED ? ED->getAttr<CxPayloadRecordAttr>() : nullptr;
  return A ? A->getRecord() : nullptr;
}

QualType Sema::getCxTagSpellingType(const TagDecl *TD, QualType T) {
  if (!getLangOpts().CX)
    return T;
  const auto *ED = dyn_cast_or_null<EnumDecl>(TD);
  if (const EnumDecl *Def = ED ? ED->getDefinition() : nullptr)
    ED = Def;
  if (RecordDecl *RD = getCxPayloadRecord(ED))
    return Context.getTagType(ElaboratedTypeKeyword::None, std::nullopt, RD,
                              /*OwnsTag=*/false);
  return T;
}

void Sema::ActOnCxPayloadEnumStart(EnumDecl *ED) {
  // The values are a struct with a name no source can spell, next to the
  // enum. It is implicit, so -ast-print shows the enum instead.
  auto *RD = RecordDecl::Create(
      Context, TagTypeKind::Struct, ED->getDeclContext(), ED->getBeginLoc(),
      ED->getLocation(), &Context.Idents.get("enum " + ED->getName().str()));
  RD->setImplicit();
  RD->addAttr(CxPayloadEnumAttr::CreateImplicit(Context, ED));
  ED->addAttr(CxPayloadRecordAttr::CreateImplicit(Context, RD));
  ED->getDeclContext()->addDecl(RD);
  RD->startDefinition();
}

void Sema::ActOnCxEnumPayload(Decl *D, ArrayRef<QualType> Types,
                              ArrayRef<const IdentifierInfo *> Labels,
                              ArrayRef<SourceLocation> Locs,
                              SourceLocation LParen) {
  auto *ECD = dyn_cast_or_null<EnumConstantDecl>(D);
  auto *ED = ECD ? dyn_cast<EnumDecl>(ECD->getDeclContext()) : nullptr;
  if (ED && isCxOptionSet(ED)) {
    Diag(LParen, diag::err_cx_optionset_payload)
        << Context.getCanonicalTagType(ED);
    return;
  }
  if (!ED || !isCxPayloadEnum(ED) || Types.empty())
    return;
  QualType EnumTy = Context.getCanonicalTagType(ED);
  if (ED->getIntegerTypeSourceInfo() && !ED->isInvalidDecl()) {
    Diag(ED->getLocation(), diag::err_cx_enum_payload_backing) << EnumTy;
    ED->setInvalidDecl();
  }
  QualType Values = getCxTagSpellingType(ED, EnumTy);
  for (auto [T, Loc] : llvm::zip(Types, Locs))
    if (CheckCxResourceStorage(T, Loc, 5))
      return;
  for (auto [T, Loc] : llvm::zip(Types, Locs))
    if (Context.hasSameUnqualifiedType(T, Values)) {
      Diag(Loc, diag::err_cx_enum_recursive_payload) << EnumTy;
      return;
    }
  QualType Payload;
  if (Types.size() > 1) {
    Payload = BuildCxTupleType(Types, Labels, Locs, LParen);
    if (Payload.isNull())
      return;
  } else {
    // One element is the payload itself, checked as a tuple element is.
    Payload = Types[0];
    if (Payload->isVoidType() || Payload->isFunctionType() ||
        Payload->isVariablyModifiedType()) {
      Diag(Locs[0], diag::err_cx_tuple_element_type) << Payload;
      return;
    }
    if (RequireCompleteType(Locs[0], Payload,
                            diag::err_cx_tuple_element_incomplete))
      return;
  }
  SmallVector<const IdentifierInfo *, 4> Written;
  for (const IdentifierInfo *L : Labels)
    Written.push_back(L ? L : &Context.Idents.get("_"));
  ECD->addAttr(CxEnumPayloadAttr::CreateImplicit(
      Context, Context.getTrivialTypeSourceInfo(Payload, LParen),
      Written.data(), Written.size()));
}

void Sema::completeCxPayloadEnum(EnumDecl *ED) {
  RecordDecl *RD = getCxPayloadRecord(ED);
  if (!RD || RD->isCompleteDefinition())
    return;
  // One union member per case with a payload, named by the case.
  auto *U = RecordDecl::Create(Context, TagTypeKind::Union, RD,
                               SourceLocation(), SourceLocation(), nullptr);
  U->setImplicit();
  U->startDefinition();
  for (EnumConstantDecl *ECD : ED->enumerators())
    if (const auto *A = ECD->getAttr<CxEnumPayloadAttr>())
      addCxField(Context, U, ECD->getIdentifier(), A->getPayload());
  U->completeDefinition();
  RD->addDecl(U);
  addCxField(Context, RD, &Context.Idents.get("$tag"),
             Context.getCanonicalTagType(ED));
  addCxField(Context, RD, &Context.Idents.get("$payload"),
             Context.getTagType(ElaboratedTypeKeyword::None, std::nullopt, U,
                                /*OwnsTag=*/false));
  RD->completeDefinition();
}

ExprResult Sema::ActOnCxEnumCaseCall(EnumDecl *ED, IdentifierInfo *Name,
                                     SourceLocation NameLoc,
                                     SourceLocation LParen,
                                     ArrayRef<const IdentifierInfo *> Labels,
                                     ArrayRef<SourceLocation> LabelLocs,
                                     MultiExprArg Args, SourceLocation RParen) {
  EnumConstantDecl *ECD = lookupCxEnumCase(*this, ED, Name, NameLoc);
  if (!ECD)
    return ExprError();
  QualType EnumTy = Context.getCanonicalTagType(ED);
  const auto *A = ECD->getAttr<CxEnumPayloadAttr>();
  if (!A) {
    Diag(LParen, diag::err_cx_enum_no_payload) << Name << EnumTy;
    return ExprError();
  }
  unsigned Count = A->labels_size();
  if (Args.size() != Count) {
    Diag(LParen, diag::err_cx_enum_payload_count)
        << Name << EnumTy << Count << unsigned(Args.size());
    return ExprError();
  }
  QualType PayloadTy = A->getPayload();
  Expr *Payload;
  if (Count == 1) {
    // The one label, when both sides write one, must agree.
    const IdentifierInfo *Declared = *A->labels_begin();
    if (Labels[0] && !Declared->isStr("_") && Labels[0] != Declared) {
      Diag(LabelLocs[0], diag::err_cx_enum_payload_label)
          << Name << Declared << Labels[0];
      return ExprError();
    }
    Payload = Args[0];
  } else {
    // A label mismatch is diagnosed once, here; the value is not built.
    DiagnosticErrorTrap Trap(Diags);
    ExprResult Lit = ActOnCxTupleLiteral(LParen, Args, Labels, LabelLocs, RParen);
    if (Lit.isInvalid())
      return ExprError();
    Payload = retargetCxTupleLiteral(Lit.get(), PayloadTy);
    if (!Payload || isa<RecoveryExpr>(Payload) || Trap.hasErrorOccurred())
      return ExprError();
  }
  return buildCxEnumValue(*this, ED, ECD, NameLoc, Payload, RParen);
}

EnumDecl *Sema::getCxEnum(QualType T) {
  if (!getLangOpts().CX || T.isNull())
    return nullptr;
  // The struct of a payload enum's values stands for the enum.
  if (const auto *RT = T->getAs<RecordType>()) {
    const auto *A = RT->getDecl()->getAttr<CxPayloadEnumAttr>();
    return A ? A->getPayloadEnum() : nullptr;
  }
  const auto *ET = T->getAs<EnumType>();
  if (!ET)
    return nullptr;
  // A Cx enum is defined by its `case` body; a definition still being parsed
  // counts, so a later case can name an earlier one as `Enum.case`.
  EnumDecl *ED = ET->getDecl();
  if (EnumDecl *Def = ED->getDefinition())
    ED = Def;
  return ED->isScoped() ? ED : nullptr;
}

EnumDecl *Sema::getCxEnumQualifier(IdentifierInfo *Name, SourceLocation Loc,
                                   Scope *S) {
  if (!getLangOpts().CX || !Name)
    return nullptr;
  // A typedef or a bare tag; any other ordinary declaration of the name, such
  // as a variable, keeps C's reading of `name.member`.
  ParsedType T = getTypeName(*Name, Loc, S);
  if (!T)
    T = getCxImplicitTagType(*Name, Loc, S);
  return T ? getCxEnum(GetTypeFromParser(T)) : nullptr;
}

ExprResult Sema::ActOnCxEnumCase(EnumDecl *ED, IdentifierInfo *Name,
                                 SourceLocation NameLoc) {
  EnumConstantDecl *ECD = lookupCxEnumCase(*this, ED, Name, NameLoc);
  if (!ECD)
    return ExprError();
  if (!isCxPayloadEnum(ED))
    return BuildDeclRefExpr(ECD, ECD->getType(), VK_PRValue, NameLoc);
  if (ECD->hasAttr<CxEnumPayloadAttr>()) {
    Diag(NameLoc, diag::err_cx_enum_needs_payload)
        << Name << Context.getCanonicalTagType(ED) << Name->getName();
    return ExprError();
  }
  return buildCxEnumValue(*this, ED, ECD, NameLoc, nullptr, NameLoc);
}

/// The type argument \p Index of a call to \p Callee has when \p Accept
/// takes it, in every function \p Callee may name; null otherwise.
static QualType
getCxArgumentType(Sema &S, Expr *Callee, unsigned Index,
                  llvm::function_ref<bool(QualType)> Accept) {
  if (!S.getLangOpts().CX || !Callee)
    return QualType();
  Callee = Callee->IgnoreParenImpCasts();
  QualType Found;
  bool Conflict = false;
  auto Consider = [&](QualType T) {
    if (!Accept(T))
      return;
    T = S.Context.getCanonicalType(T).getUnqualifiedType();
    if (Found.isNull())
      Found = T;
    else if (Found != T)
      Conflict = true;
  };
  auto ConsiderDecl = [&](const NamedDecl *ND, bool ThroughReceiver) {
    const auto *FD = dyn_cast<FunctionDecl>(ND->getUnderlyingDecl());
    if (!FD)
      return;
    unsigned I = Index + (ThroughReceiver ? S.getCxReceiverOffset(FD) : 0);
    if (I < FD->getNumParams())
      Consider(FD->getParamDecl(I)->getType());
  };
  if (auto *DRE = dyn_cast<DeclRefExpr>(Callee);
      DRE && isa<FunctionDecl>(DRE->getDecl())) {
    ConsiderDecl(DRE->getDecl(), false);
  } else if (auto *ME = dyn_cast<MemberExpr>(Callee);
             ME && isa<FunctionDecl>(ME->getMemberDecl())) {
    ConsiderDecl(ME->getMemberDecl(), true);
  } else if (auto *OE = dyn_cast<OverloadExpr>(Callee)) {
    for (const NamedDecl *ND : OE->decls())
      ConsiderDecl(ND, false);
  } else {
    // A call through a function or block pointer.
    QualType T = Callee->getType();
    if (T.isNull())
      return QualType();
    if (const auto *PT = T->getAs<PointerType>())
      T = PT->getPointeeType();
    else if (const auto *BPT = T->getAs<BlockPointerType>())
      T = BPT->getPointeeType();
    if (const auto *FPT = T->getAs<FunctionProtoType>();
        FPT && Index < FPT->getNumParams())
      Consider(FPT->getParamType(Index));
  }
  return Conflict ? QualType() : Found;
}

QualType Sema::getCxCaseArgumentType(Expr *Callee, unsigned Index) {
  return getCxArgumentType(*this, Callee, Index,
                           [&](QualType T) { return getCxEnum(T); });
}

QualType Sema::getCxTupleArgumentType(Expr *Callee, unsigned Index) {
  // Keep the labels the parameter is written with; they check the literal.
  return getCxArgumentType(*this, Callee, Index,
                           [&](QualType T) { return isCxTupleType(T); });
}

QualType Sema::getCxElementType(QualType T, unsigned Index) {
  if (!getLangOpts().CX || T.isNull())
    return QualType();
  if (const ArrayType *AT = Context.getAsArrayType(T))
    return AT->getElementType();
  if (const auto *RT = T->getAs<RecordType>()) {
    const RecordDecl *RD = RT->getDecl()->getDefinition();
    // A payload enum's values are built through its cases.
    if (!RD || RD->hasAttr<CxPayloadEnumAttr>())
      return QualType();
    for (const FieldDecl *FD : RD->fields()) {
      if (FD->isUnnamedBitField())
        continue;
      if (Index-- == 0)
        return FD->getType();
      if (RD->isUnion())
        break; // a union takes one element
    }
    return QualType();
  }
  // A scalar in braces, `Color c = { .red }`.
  return T->isScalarType() && Index == 0 ? T : QualType();
}

QualType Sema::getCxDesignatedType(QualType T, const Designation &D,
                                   unsigned &Next) {
  Next = ~0u;
  QualType Cur = T;
  for (unsigned I = 0, N = D.getNumDesignators(); I != N && !Cur.isNull();
       ++I) {
    const Designator &Des = D.getDesignator(I);
    if (Des.isArrayDesignator() || Des.isArrayRangeDesignator()) {
      const ArrayType *AT = Context.getAsArrayType(Cur);
      Cur = AT ? AT->getElementType() : QualType();
      // Every element of an array has one type; positions go on from any.
      if (I == 0)
        Next = 0;
      continue;
    }
    const auto *RT = Cur->getAs<RecordType>();
    const RecordDecl *RD = RT ? RT->getDecl()->getDefinition() : nullptr;
    Cur = QualType();
    if (!RD)
      break;
    unsigned Position = 0;
    for (const FieldDecl *FD : RD->fields()) {
      if (FD->isUnnamedBitField())
        continue;
      if (FD->getIdentifier() == Des.getFieldDecl()) {
        Cur = FD->getType();
        // C goes on with the field after the designated one.
        if (I == 0 && !RD->isUnion())
          Next = Position + 1;
        break;
      }
      ++Position;
    }
  }
  return Cur;
}

ExprResult Sema::BuildCxEnumMember(Expr *Base,
                                   const DeclarationNameInfo &Name) {
  EnumDecl *ED = getCxEnum(Base->getType());
  QualType EnumTy = Base->getType().getUnqualifiedType();
  const IdentifierInfo *II = Name.getName().getAsIdentifierInfo();
  if (!II || !II->isStr("rawValue")) {
    Diag(Name.getLoc(), diag::err_cx_enum_no_member)
        << Name.getName() << EnumTy;
    return ExprError();
  }
  if (!ED->getIntegerTypeSourceInfo() && !isCxOptionSet(ED)) {
    Diag(Name.getLoc(), diag::err_cx_enum_no_raw_value) << EnumTy;
    return ExprError();
  }
  // The only conversion out of a Cx enum; the printers show it as rawValue.
  ExprResult R = DefaultLvalueConversion(Base);
  if (R.isInvalid())
    return ExprError();
  return ImpCastExprToType(R.get(), ED->getIntegerType(), CK_IntegralCast);
}

bool Sema::diagnoseCxEnumCondition(const Expr *E) {
  if (!E || !getCxEnum(E->getType()))
    return false;
  Diag(E->getExprLoc(), diag::err_cx_enum_condition)
      << E->getType().getUnqualifiedType() << E->getSourceRange();
  return true;
}

/// `Base.Name`, reaching the implicit fields of Cx lowerings: this does not
/// pass through the Cx member hook that hides them from source.
static ExprResult buildCxFieldRef(Sema &S, Expr *Base, StringRef Name,
                                  SourceLocation Loc) {
  CXXScopeSpec SS;
  return S.BuildMemberReferenceExpr(
      Base, Base->getType(), Loc, /*IsArrow=*/false, SS, SourceLocation(),
      nullptr, DeclarationNameInfo(&S.Context.Idents.get(Name), Loc), nullptr,
      nullptr);
}

StmtResult Sema::ActOnCxSwitchStart(SourceLocation SwitchLoc,
                                    SourceLocation LParen, Expr *Cond,
                                    SourceLocation RParen, CxSwitchInfo &Info,
                                    bool &IsCx) {
  EnumDecl *ED = getCxEnum(Cond->getType());
  IsCx = ED != nullptr;
  if (!ED)
    return StmtError();
  if (isCxOptionSet(ED)) {
    Diag(Cond->getExprLoc(), diag::err_cx_optionset_switch)
        << Cond->getSourceRange();
    return StmtError();
  }
  Info.Enum = ED;
  // The condition is evaluated once, into a variable no source can name, held
  // by the switch's init-statement.
  QualType T = Cond->getType().getUnqualifiedType();
  auto *Match = VarDecl::Create(Context, CurContext, LParen, LParen,
                                /*Id=*/nullptr, T,
                                Context.getTrivialTypeSourceInfo(T, LParen),
                                SC_None);
  Match->setImplicit();
  Match->setReferenced();
  Match->addAttr(CxMatchAttr::CreateImplicit(Context));
  CurContext->addHiddenDecl(Match);
  AddInitializerToDecl(Match, Cond, /*DirectInit=*/false);
  if (Match->isInvalidDecl())
    return StmtError();
  Info.Match = Match;
  StmtResult Init =
      ActOnDeclStmt(ConvertDeclToDeclGroup(Match), LParen, RParen);

  // The switch tests the tag, as its backing integer, so C's integer cases
  // apply.
  ExprResult Tag = BuildDeclRefExpr(Match, T, VK_LValue, LParen);
  if (isCxPayloadEnum(ED))
    Tag = buildCxFieldRef(*this, Tag.get(), "$tag", LParen);
  if (Tag.isInvalid())
    return StmtError();
  Tag = DefaultLvalueConversion(Tag.get());
  if (Tag.isInvalid())
    return StmtError();
  Tag = ImpCastExprToType(Tag.get(), ED->getIntegerType(), CK_IntegralCast);
  ConditionResult C = ActOnCondition(getCurScope(), SwitchLoc, Tag.get(),
                                     ConditionKind::Switch);
  if (C.isInvalid())
    return StmtError();
  return ActOnStartOfSwitchStmt(SwitchLoc, LParen, Init.get(), C, RParen);
}

ExprResult Sema::ActOnCxSwitchCase(CxSwitchInfo &Info, EnumDecl *Qualifier,
                                   IdentifierInfo *Name, SourceLocation NameLoc,
                                   EnumConstantDecl *&Case) {
  Case = nullptr;
  QualType EnumTy = Context.getCanonicalTagType(Info.Enum);
  if (Qualifier && Qualifier->getCanonicalDecl() !=
                       Info.Enum->getCanonicalDecl()) {
    Diag(NameLoc, diag::err_cx_switch_wrong_enum) << Name << EnumTy;
    return ExprError();
  }
  EnumConstantDecl *ECD = lookupCxEnumCase(*this, Info.Enum, Name, NameLoc);
  if (!ECD)
    return ExprError();
  auto [It, New] = Info.Seen.insert({ECD, NameLoc});
  if (!New) {
    Diag(NameLoc, diag::err_cx_switch_duplicate) << Name;
    Diag(It->second, diag::note_cx_switch_previous);
    return ExprError();
  }
  Case = ECD;
  QualType IntTy = Info.Enum->getIntegerType();
  llvm::APInt Value = ECD->getInitVal().extOrTrunc(Context.getIntWidth(IntTy));
  return ActOnCaseExpr(NameLoc,
                       IntegerLiteral::Create(Context, Value, IntTy, NameLoc));
}

StmtResult Sema::ActOnCxSwitchBindings(CxSwitchInfo &Info,
                                       EnumConstantDecl *Case,
                                       ArrayRef<IdentifierInfo *> Names,
                                       ArrayRef<SourceLocation> Locs,
                                       SourceLocation LParen) {
  const auto *A = Case->getAttr<CxEnumPayloadAttr>();
  if (!A) {
    Diag(LParen, diag::err_cx_switch_bind_no_payload) << Case;
    return StmtError();
  }
  unsigned Count = A->labels_size();
  if (Names.size() != Count) {
    Diag(LParen, diag::err_cx_switch_bind_count)
        << Case << Count << unsigned(Names.size());
    return StmtError();
  }
  // Each name is a const copy of its element of the active payload.
  ExprResult Payload =
      BuildDeclRefExpr(Info.Match, Info.Match->getType(), VK_LValue, LParen);
  Payload = buildCxFieldRef(*this, Payload.get(), "$payload", LParen);
  if (Payload.isInvalid())
    return StmtError();
  Payload = buildCxFieldRef(*this, Payload.get(), Case->getName(), LParen);
  if (Payload.isInvalid())
    return StmtError();
  SmallVector<Decl *, 4> Decls;
  for (unsigned I = 0; I != Count; ++I) {
    if (!Names[I])
      continue; // `_` discards the element.
    if (llvm::is_contained(ArrayRef(Names).take_front(I), Names[I])) {
      Diag(Locs[I], diag::err_redefinition) << Names[I];
      continue;
    }
    ExprResult Elem = Count == 1 ? Payload
                                 : buildCxFieldRef(*this, Payload.get(),
                                                   "$" + std::to_string(I),
                                                   Locs[I]);
    if (Elem.isInvalid())
      continue;
    QualType ET = Elem.get()->getType().getUnqualifiedType();
    ET.addConst();
    auto *VD = VarDecl::Create(Context, CurContext, Locs[I], Locs[I], Names[I],
                               ET, Context.getTrivialTypeSourceInfo(ET, Locs[I]),
                               SC_None);
    VD->addAttr(CxPatternBindingAttr::CreateImplicit(Context, I));
    PushOnScopeChains(VD, getCurScope());
    AddInitializerToDecl(VD, Elem.get(), /*DirectInit=*/false);
    Decls.push_back(VD);
  }
  if (Decls.empty())
    return StmtEmpty();
  return ActOnDeclStmt(BuildDeclaratorGroup(Decls), LParen, LParen);
}

StmtResult Sema::ActOnCxSwitchFinish(SourceLocation SwitchLoc, Stmt *Switch,
                                     CxSwitchInfo &Info,
                                     SmallVectorImpl<Stmt *> &Clauses,
                                     SourceLocation LBrace,
                                     SourceLocation RBrace) {
  QualType EnumTy = Context.getCanonicalTagType(Info.Enum);
  std::string Missing;
  unsigned NumMissing = 0;
  for (EnumConstantDecl *ECD : Info.Enum->enumerators())
    if (!Info.Seen.count(ECD)) {
      Missing += (NumMissing++ ? ", '" : "'") + ECD->getName().str() + "'";
    }
  if (Info.DefaultLoc.isValid()) {
    if (!NumMissing)
      Diag(Info.DefaultLoc, diag::warn_cx_switch_default_unreachable) << EnumTy;
  } else {
    if (NumMissing)
      Diag(SwitchLoc, diag::err_cx_switch_missing)
          << EnumTy << Missing << NumMissing;
    // A value that matches no case, which only memory reinterpretation can
    // make, stops the program. The default has no location, so printers and
    // tools know it is implicit.
    UnqualifiedId Name;
    Name.setIdentifier(&Context.Idents.get("__builtin_trap"), SwitchLoc);
    CXXScopeSpec SS;
    ExprResult Fn = ActOnIdExpression(TUScope, SS, SourceLocation(), Name,
                                      /*HasTrailingLParen=*/true,
                                      /*IsAddressOfOperand=*/false);
    ExprResult Trap = Fn.isInvalid() ? ExprError()
                                     : BuildCallExpr(TUScope, Fn.get(), SwitchLoc,
                                                     {}, SwitchLoc);
    if (Trap.isUsable()) {
      StmtResult Default = ActOnDefaultStmt(SourceLocation(), SourceLocation(),
                                            Trap.get(), getCurScope());
      if (Default.isUsable())
        Clauses.push_back(Default.get());
    }
  }
  StmtResult Body = ActOnCompoundStmt(LBrace, RBrace, Clauses,
                                      /*isStmtExpr=*/false);
  return ActOnFinishSwitchStmt(SwitchLoc, Switch,
                               Body.isInvalid() ? nullptr : Body.get());
}

bool Sema::isCxSwitch(const SwitchStmt *S) {
  const auto *DS = S ? dyn_cast_or_null<DeclStmt>(S->getInit()) : nullptr;
  return DS && DS->isSingleDecl() && DS->getSingleDecl()->hasAttr<CxMatchAttr>();
}

bool Sema::diagnoseCxNestedSwitchLabel(SourceLocation Loc) {
  if (!getLangOpts().CX || getCurFunction() == nullptr ||
      getCurFunction()->SwitchStack.empty() ||
      !isCxSwitch(getCurFunction()->SwitchStack.back().getPointer()))
    return false;
  Diag(Loc, diag::err_cx_switch_nested_case);
  return true;
}

bool Sema::isCxOptionSet(const EnumDecl *ED) const {
  return ED && ED->hasAttr<CxOptionSetAttr>();
}

void Sema::ActOnCxOptionSetStart(EnumDecl *ED) {
  ED->addAttr(CxOptionSetAttr::CreateImplicit(Context));
}

/// The declared bits of option set \p ED.
static llvm::APInt getCxOptionSetMask(const EnumDecl *ED, unsigned Width) {
  llvm::APInt Mask(Width, 0);
  for (const EnumConstantDecl *ECD : ED->enumerators())
    Mask |= ECD->getInitVal().zextOrTrunc(Width);
  return Mask;
}

/// A constant set of option set \p ED holding \p Bits. The printers show
/// it as a literal of the cases it holds.
static Expr *buildCxOptionSetConstant(Sema &S, EnumDecl *ED,
                                      const llvm::APInt &Bits,
                                      SourceLocation Loc) {
  QualType IntTy = ED->getIntegerType();
  auto *Lit = IntegerLiteral::Create(S.Context, Bits, IntTy, Loc);
  return ImplicitCastExpr::Create(S.Context, S.Context.getCanonicalTagType(ED),
                                  CK_IntegralCast, Lit, nullptr, VK_PRValue,
                                  FPOptionsOverride());
}

/// \p E as a value of option set \p ED, or null after a diagnostic.
static Expr *checkCxOptionSetOperand(Sema &S, EnumDecl *ED, Expr *E,
                                     QualType Other) {
  ExprResult R = S.DefaultLvalueConversion(E);
  if (R.isInvalid())
    return nullptr;
  if (S.getCxEnum(R.get()->getType()) != ED) {
    S.Diag(E->getExprLoc(), diag::err_cx_optionset_mixed)
        << Other << R.get()->getType().getUnqualifiedType()
        << E->getSourceRange();
    return nullptr;
  }
  return R.get();
}

ExprResult Sema::ActOnCxOptionSetLiteral(QualType Expected,
                                         SourceLocation LBracket,
                                         MultiExprArg Elems,
                                         SourceLocation RBracket) {
  EnumDecl *ED = getCxEnum(Expected);
  QualType SetTy = Context.getCanonicalTagType(ED);
  if (Elems.empty())
    return buildCxOptionSetConstant(
        *this, ED, llvm::APInt(Context.getIntWidth(ED->getIntegerType()), 0),
        LBracket);
  // The union of the elements, `a | b | ...`.
  Expr *Set = nullptr;
  for (Expr *E : Elems) {
    Expr *V = checkCxOptionSetOperand(*this, ED, E, SetTy);
    if (!V)
      return ExprError();
    Set = !Set ? V
               : BinaryOperator::Create(Context, Set, V, BO_Or, SetTy,
                                        VK_PRValue, OK_Ordinary, LBracket,
                                        CurFPFeatureOverrides());
  }
  // A literal is one operand wherever it stands, and prints as one.
  if (Elems.size() > 1)
    Set = new (Context) ParenExpr(LBracket, RBracket, Set);
  return Set;
}

ExprResult Sema::BuildCxOptionSetNot(SourceLocation OpLoc, Expr *A) {
  EnumDecl *ED = getCxEnum(A->getType());
  QualType SetTy = Context.getCanonicalTagType(ED);
  Expr *V = checkCxOptionSetOperand(*this, ED, A, SetTy);
  if (!V)
    return ExprError();
  // Within the declared bits: `a ^ [every case]`.
  llvm::APInt Mask =
      getCxOptionSetMask(ED, Context.getIntWidth(ED->getIntegerType()));
  Expr *X = BinaryOperator::Create(
      Context, V, buildCxOptionSetConstant(*this, ED, Mask, OpLoc), BO_Xor,
      SetTy, VK_PRValue, OK_Ordinary, OpLoc, CurFPFeatureOverrides());
  return new (Context) ParenExpr(OpLoc, A->getEndLoc(), X);
}

ExprResult Sema::BuildCxOptionSetBinOp(SourceLocation OpLoc,
                                       BinaryOperatorKind Opc, Expr *LHS,
                                       Expr *RHS, bool &Handled) {
  EnumDecl *ED = getCxEnum(LHS->getType());
  if (!isCxOptionSet(ED))
    ED = getCxEnum(RHS->getType());
  Handled = isCxOptionSet(ED);
  if (!Handled)
    return ExprError();
  QualType SetTy = Context.getCanonicalTagType(ED);
  bool Compound = BinaryOperator::isCompoundAssignmentOp(Opc);
  BinaryOperatorKind Op = Compound ? BinaryOperator::getOpForCompoundAssignment(Opc)
                                   : Opc;
  // Only arithmetic and bitwise operators are the set's; equality, logic,
  // assignment and the comma keep the rules of any Cx enum.
  if (!BinaryOperator::isMultiplicativeOp(Op) &&
      !BinaryOperator::isAdditiveOp(Op) && !BinaryOperator::isShiftOp(Op) &&
      !BinaryOperator::isBitwiseOp(Op)) {
    Handled = false;
    return ExprError();
  }
  if (Op != BO_Or && Op != BO_And && Op != BO_Xor && Op != BO_Sub) {
    Diag(OpLoc, diag::err_cx_optionset_operator)
        << SetTy << BinaryOperator::getOpcodeStr(Opc)
        << LHS->getSourceRange() << RHS->getSourceRange();
    return ExprError();
  }
  Expr *R = checkCxOptionSetOperand(*this, ED, RHS, SetTy);
  if (!R)
    return ExprError();
  // `a - b` is `a & ~b`, the complement within the declared bits.
  if (Op == BO_Sub) {
    ExprResult NotR = BuildCxOptionSetNot(OpLoc, R);
    if (NotR.isInvalid())
      return ExprError();
    R = NotR.get();
    Op = BO_And;
    Opc = Compound ? BO_AndAssign : BO_And;
  }
  if (Compound) {
    // The left operand is updated once, as a C compound assignment does.
    if (getCxEnum(LHS->getType()) != ED) {
      Diag(LHS->getExprLoc(), diag::err_cx_optionset_mixed)
          << SetTy << LHS->getType().getUnqualifiedType();
      return ExprError();
    }
    QualType LHSTy = LHS->getType();
    ExprResult Value = R;
    if (CheckAssignmentOperands(LHS, Value, OpLoc, SetTy, Opc).isNull())
      return ExprError();
    return CompoundAssignOperator::Create(
        Context, LHS, R, Opc, LHSTy.getUnqualifiedType(), VK_PRValue,
        OK_Ordinary, OpLoc, CurFPFeatureOverrides(), SetTy, SetTy);
  }
  Expr *L = checkCxOptionSetOperand(*this, ED, LHS, SetTy);
  if (!L)
    return ExprError();
  return BinaryOperator::Create(Context, L, R, Opc, SetTy, VK_PRValue,
                                OK_Ordinary, OpLoc, CurFPFeatureOverrides());
}

bool Sema::isCxOptionSetMethod(Expr *Base, const IdentifierInfo *Name) {
  return Name && isCxOptionSet(getCxEnum(Base->getType())) &&
         (Name->isStr("contains") || Name->isStr("isSubset") ||
          Name->isStr("isSuperset") || Name->isStr("isDisjoint"));
}

ExprResult Sema::BuildCxOptionSetMethod(Expr *Base, const IdentifierInfo *Name,
                                        SourceLocation NameLoc,
                                        ArrayRef<const IdentifierInfo *> Labels,
                                        MultiExprArg Args,
                                        SourceLocation RParen) {
  EnumDecl *ED = getCxEnum(Base->getType());
  QualType SetTy = Context.getCanonicalTagType(ED);
  StringRef Label = Name->isStr("isSubset") || Name->isStr("isSuperset") ? "of"
                    : Name->isStr("isDisjoint")                          ? "with"
                                                                         : "";
  if (Args.size() != 1 ||
      (Label.empty() ? Labels[0] != nullptr
                     : !Labels[0] || Labels[0]->getName() != Label)) {
    Diag(NameLoc, diag::err_cx_optionset_method_args)
        << Name->getName() << !Label.empty() << Label;
    return ExprError();
  }
  Expr *A = checkCxOptionSetOperand(*this, ED, Base, SetTy);
  Expr *B = A ? checkCxOptionSetOperand(*this, ED, Args[0], SetTy) : nullptr;
  if (!B)
    return ExprError();
  // Each operand is evaluated once: `b` in `a` is `(b & ~a) == []`.
  auto And = [&](Expr *X, Expr *Y) -> Expr * {
    return new (Context) ParenExpr(
        NameLoc, RParen,
        BinaryOperator::Create(Context, X, Y, BO_And, SetTy, VK_PRValue,
                               OK_Ordinary, NameLoc, CurFPFeatureOverrides()));
  };
  auto Not = [&](Expr *X) { return BuildCxOptionSetNot(NameLoc, X).get(); };
  Expr *Rest;
  if (Name->isStr("contains") || Name->isStr("isSuperset"))
    Rest = And(B, Not(A));
  else if (Name->isStr("isSubset"))
    Rest = And(A, Not(B));
  else
    Rest = And(A, B);
  Expr *Empty = buildCxOptionSetConstant(
      *this, ED, llvm::APInt(Context.getIntWidth(ED->getIntegerType()), 0),
      NameLoc);
  return BinaryOperator::Create(Context, Rest, Empty, BO_EQ, Context.IntTy,
                                VK_PRValue, OK_Ordinary, NameLoc,
                                CurFPFeatureOverrides());
}
