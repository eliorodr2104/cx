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

/// The method of \p RD that \p New redeclares, or null when it introduces a
/// new one. Identity is the name, the parameter types and the argument
/// labels; local parameter names may differ.
static FunctionDecl *findCxMethodDeclaration(ASTContext &Ctx,
                                             const RecordDecl *RD,
                                             const FunctionDecl *New) {
  for (Decl *D : RD->decls()) {
    auto *Old = dyn_cast<FunctionDecl>(D);
    if (!Old || Old == New || !Old->hasAttr<CxMethodAttr>())
      continue;
    if (Old->getDeclName() != New->getDeclName())
      continue;
    if (!Ctx.hasSameType(Old->getType(), New->getType()))
      continue;
    if (Old->getNumParams() != New->getNumParams())
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
  for (unsigned I = 0; I != FTI.NumParams; ++I) {
    if (auto *P = dyn_cast_or_null<ParmVarDecl>(FTI.Params[I].Param)) {
      P->setOwningFunction(FD);
      P->setScopeInfo(0, Params.size());
      Params.push_back(P);
    }
  }
  FD->setParams(Params);
  // Linkage for a declaration held by a record is computed the way a class
  // member's is, which needs an access specifier. Cx members are public by
  // default; M4b makes that explicit and adds the other levels.
  FD->setAccess(AS_public);
  FD->addAttr(CxMethodAttr::CreateImplicit(Context));

  // A method cannot be a C entity, so it always carries a Cx name. The module
  // may be absent when the file has no owner; the record name still keeps the
  // symbol distinct.
  const IdentifierInfo *Owner = Context.getCxModuleOwner(FD->getLocation());
  FD->addAttr(CxLinkageAttr::CreateImplicit(
      Context, const_cast<IdentifierInfo *>(Owner)));
  if (!FD->hasAttr<OverloadableAttr>())
    FD->addAttr(OverloadableAttr::CreateImplicit(Context));

  ProcessDeclAttributes(S, FD, D);

  // A continuation implements what the primary definition declared, so a
  // matching method is a redeclaration and not a second entity.
  if (FunctionDecl *Prev = findCxMethodDeclaration(Context, RD, FD)) {
    FD->setPreviousDeclaration(Prev);
    FD->setAccess(Prev->getAccess());
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

FunctionDecl *Sema::getCurrentCxMethod() {
  if (!getLangOpts().CX)
    return nullptr;
  auto *FD = dyn_cast_or_null<FunctionDecl>(CurContext);
  return FD && FD->hasAttr<CxMethodAttr>() ? FD : nullptr;
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

  ExprResult Fn = BuildDeclRefExpr(
      Method, Method->getType(),
      getLangOpts().CPlusPlus ? VK_LValue : VK_PRValue,
      DeclarationNameInfo(Method->getDeclName(), ME->getMemberLoc()),
      NestedNameSpecifierLoc());
  if (Fn.isInvalid())
    return ExprError();

  return BuildResolvedCallExpr(Fn.get(), Method, LParenLoc, AllArgs, RParenLoc);
}

ExprResult
Sema::BuildCxImplicitSelfMemberRef(const DeclarationNameInfo &NameInfo,
                                   Scope *S) {
  FunctionDecl *Method = getCurrentCxMethod();
  if (!Method || Method->getNumParams() == 0)
    return ExprEmpty();

  ParmVarDecl *Self = Method->getParamDecl(0);
  const auto *PT = Self->getType()->getAs<PointerType>();
  if (!PT)
    return ExprEmpty();
  const auto *RT = PT->getPointeeType()->getAs<RecordType>();
  if (!RT)
    return ExprEmpty();
  RecordDecl *RD = RT->getDecl();

  bool IsMember = LookupCxMethod(RD, NameInfo.getName()) != nullptr;
  if (!IsMember) {
    LookupResult Fields(*this, NameInfo, LookupMemberName);
    Fields.suppressDiagnostics();
    LookupQualifiedName(Fields, RD);
    IsMember = !Fields.empty();
  }
  if (!IsMember)
    return ExprEmpty();

  ExprResult Base = BuildDeclRefExpr(
      Self, Self->getType(), VK_LValue,
      DeclarationNameInfo(Self->getDeclName(), NameInfo.getLoc()),
      NestedNameSpecifierLoc());
  if (Base.isInvalid())
    return ExprError();

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

bool Sema::CheckCxMemberAccess(const NamedDecl *Member, SourceLocation Loc,
                               bool ForWrite) {
  if (!getLangOpts().CX || !Member)
    return false;
  const auto *A = Member->getAttr<CxAccessAttr>();
  if (!A)
    return false;
  unsigned Level = ForWrite ? A->getWrite() : A->getRead();
  if (Level == CxAccess_public)
    return false;

  const RecordDecl *RD = getCxMemberRecord(Member);
  if (!RD)
    return false;

  // Access is decided by the declaration's ownership, not by the module of
  // whoever included the header.
  const IdentifierInfo *Owner =
      Context.getCxModuleOwner(RD->getCanonicalDecl()->getLocation());

  // Anything lexically inside the type -- a method body, or a continuation --
  // is the type's own implementation.
  for (const DeclContext *DC = CurContext; DC; DC = DC->getLexicalParent()) {
    if (DC->getPrimaryContext() == RD->getPrimaryContext())
      return false;
    if (const auto *FD = dyn_cast<FunctionDecl>(DC))
      if (FD->hasAttr<CxMethodAttr>() &&
          isa<RecordDecl>(FD->getDeclContext()) &&
          cast<RecordDecl>(FD->getDeclContext())->getPrimaryContext() ==
              RD->getPrimaryContext())
        return false;
  }

  // Internal additionally admits any code in the owning module.
  if (Level == CxAccess_internal && Owner &&
      Context.getCxModuleOwner(Loc) == Owner)
    return false;

  Diag(Loc, diag::err_cx_member_inaccessible) << Member << Level << ForWrite;
  Diag(Member->getLocation(), diag::note_cx_member_declared_here)
      << Level << Member;
  return true;
}

//===----------------------------------------------------------------------===//
// Generated memberwise construction
//===----------------------------------------------------------------------===//

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

ParsedType Sema::getCxConstructionType(const IdentifierInfo *II,
                                       SourceLocation Loc, Scope *S) {
  if (!getLangOpts().CX || !II)
    return nullptr;
  ParsedType T = getTypeName(*II, Loc, S);
  if (!T)
    return nullptr;
  // Unions are accepted here and rejected in ActOnCxConstruction, so the
  // diagnostic explains the rule instead of falling back to "not an
  // expression".
  QualType QT = GetTypeFromParser(T);
  return QT->getAs<RecordType>() ? T : nullptr;
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
    if (Expr *Default = FD->getInClassInitializer())
      Defaults.push_back(Default);
    else
      // Until definite initialization exists, a field the initializer does
      // not write reads as zero rather than as garbage.
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

  // The initializers are an overload set like any other; the receiver is the
  // first argument, which is what lets the ordinary resolution apply.
  FunctionDecl *Init = nullptr;
  {
    CxCallLabelScope LabelScope(*this, Labels);
    OverloadCandidateSet Candidates(TypeLoc, OverloadCandidateSet::CSK_Normal);
    llvm::SmallPtrSet<const FunctionDecl *, 4> Seen;
    for (Decl *D : RD->decls()) {
      auto *FD = dyn_cast<FunctionDecl>(D);
      if (!FD || !isCxInit(FD) || !Seen.insert(FD->getCanonicalDecl()).second)
        continue;
      AddOverloadCandidate(FD, DeclAccessPair::make(FD, FD->getAccess()),
                           AllArgs, Candidates);
    }

    OverloadCandidateSet::iterator Best;
    switch (Candidates.BestViableFunction(*this, TypeLoc, Best)) {
    case OR_Success:
      Init = Best->Function;
      break;
    case OR_No_Viable_Function:
      Candidates.NoteCandidates(
          PartialDiagnosticAt(TypeLoc,
                              PDiag(diag::err_cx_no_viable_initializer) << T),
          *this, OCD_AllCandidates, AllArgs);
      return ExprError();
    case OR_Ambiguous:
      Candidates.NoteCandidates(
          PartialDiagnosticAt(TypeLoc,
                              PDiag(diag::err_cx_ambiguous_initializer) << T),
          *this, OCD_AmbiguousCandidates, AllArgs);
      return ExprError();
    case OR_Deleted:
      return ExprError();
    }
  }

  // Construction is bounded by the initializer it selects, not by the fields.
  if (CheckCxMemberAccess(Init, TypeLoc, /*ForWrite=*/false))
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
