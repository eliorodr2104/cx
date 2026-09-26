//===--- CGCx.cpp - Cx resource values ------------------------------------===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//
//
// A Cx resource type has a deinit. Its values are destroyed when the local
// or the value holding them ends, when they are replaced, and when they are
// discarded; all of it runs on the cleanup stack that `defer` uses.
//
//===----------------------------------------------------------------------===//

#include "CGCall.h"
#include "CodeGenFunction.h"
#include "CodeGenModule.h"
#include "clang/AST/Attr.h"

using namespace clang;
using namespace CodeGen;

static bool isCxDeinit(const FunctionDecl *FD) {
  return FD->hasAttr<CxMethodAttr>() && FD->getDeclName().isIdentifier() &&
         FD->getName() == "deinit";
}

static bool isCxInit(const FunctionDecl *FD) {
  return FD->hasAttr<CxMethodAttr>() && FD->getDeclName().isIdentifier() &&
         FD->getName() == "init";
}

/// The deinit that destroys a value of \p T, or of its array elements.
static const FunctionDecl *getCxDeinit(const ASTContext &Ctx, QualType T) {
  const RecordDecl *RD = Ctx.getBaseElementType(T)->getAsRecordDecl();
  if (!RD || !(RD = RD->getDefinition()))
    return nullptr;
  for (const Decl *D : RD->decls())
    if (const auto *FD = dyn_cast<FunctionDecl>(D))
      if (isCxDeinit(FD))
        return FD;
  return nullptr;
}

bool CodeGenFunction::isCxResourceType(QualType T) const {
  return getLangOpts().CX && getCxDeinit(getContext(), T);
}

/// Destroys the one value at \p Addr; arrays are looped over by the caller.
static void destroyCxResource(CodeGenFunction &CGF, Address Addr, QualType T) {
  const FunctionDecl *FD = getCxDeinit(CGF.getContext(), T);
  llvm::Constant *Fn = CGF.CGM.GetAddrOfFunction(FD);
  const CGFunctionInfo &Info = CGF.CGM.getTypes().arrangeFunctionDeclaration(FD);
  CallArgList Args;
  Args.add(RValue::get(Addr.emitRawPointer(CGF)),
           FD->getParamDecl(0)->getType());
  CGF.EmitCall(Info, CGCallee::forDirect(Fn, GlobalDecl(FD)), ReturnValueSlot(),
               Args);
}

bool CodeGenFunction::pushCxResourceDestroy(Address Addr, QualType T) {
  if (!isCxResourceType(T))
    return false;
  pushDestroy(NormalAndEHCleanup, Addr, T, destroyCxResource,
              /*useEHCleanupForArray=*/true);
  return true;
}

void CodeGenFunction::emitCxResourceDestroy(Address Addr, QualType T) {
  emitDestroy(Addr, T, destroyCxResource, /*useEHCleanupForArray=*/false);
}

namespace {
/// Destroys a consumed variable only if it still holds its value.
struct DestroyCxIfAlive final : EHScopeStack::Cleanup {
  Address Addr;
  QualType T;
  llvm::Value *Flag;
  DestroyCxIfAlive(Address Addr, QualType T, llvm::Value *Flag)
      : Addr(Addr), T(T), Flag(Flag) {}
  void Emit(CodeGenFunction &CGF, Flags) override {
    llvm::BasicBlock *Destroy = CGF.createBasicBlock("cx.destroy");
    llvm::BasicBlock *Done = CGF.createBasicBlock("cx.destroy.done");
    CGF.Builder.CreateCondBr(CGF.Builder.CreateFlagLoad(Flag, "cx.alive"),
                             Destroy, Done);
    CGF.EmitBlock(Destroy);
    CGF.emitCxResourceDestroy(Addr, T);
    CGF.EmitBlock(Done);
  }
};
} // namespace

void CodeGenFunction::pushCxVarDestroy(const VarDecl &D, Address Addr) {
  if (!isCxResourceType(D.getType()))
    return;
  if (!D.hasAttr<CxConsumedAttr>()) {
    pushCxResourceDestroy(Addr, D.getType());
    return;
  }
  RawAddress Flag =
      CreateTempAlloca(Builder.getInt1Ty(), CharUnits::One(), "cx.alive");
  Builder.CreateFlagStore(true, Flag.getPointer());
  CxAliveFlags[&D] = Flag.getPointer();
  EHStack.pushCleanup<DestroyCxIfAlive>(NormalAndEHCleanup, Addr, D.getType(),
                                        Flag.getPointer());
}

void CodeGenFunction::markCxConsumed(const Expr *E) {
  if (CxAliveFlags.empty())
    return;
  const auto *DRE = dyn_cast<DeclRefExpr>(E->IgnoreParens());
  const auto *VD = DRE ? dyn_cast<VarDecl>(DRE->getDecl()) : nullptr;
  if (llvm::Value *Flag = VD ? CxAliveFlags.lookup(VD) : nullptr)
    Builder.CreateFlagStore(false, Flag);
}

void CodeGenFunction::EmitCxDeinitFieldCleanups(const FunctionDecl *FD) {
  if (!getLangOpts().CX || !isCxDeinit(FD) || !FD->getNumParams())
    return;
  // The body runs first; then each field that has a deinit is destroyed, in
  // reverse declaration order, which is the order the cleanups unwind in.
  const ParmVarDecl *Self = FD->getParamDecl(0);
  QualType RecTy = Self->getType()->getPointeeType();
  llvm::Value *Ptr = Builder.CreateLoad(GetAddrOfLocalVar(Self));
  LValue Base = MakeNaturalAlignAddrLValue(Ptr, RecTy);
  for (const FieldDecl *Field : RecTy->getAsRecordDecl()->fields())
    if (isCxResourceType(Field->getType()))
      pushCxResourceDestroy(EmitLValueForField(Base, Field).getAddress(),
                            Field->getType());
}

bool CodeGenFunction::isCxRawResourceStore(const Expr *LHS) const {
  LHS = LHS->IgnoreParens();
  // `p->init(...)` stores into `*__cx_place`, which holds no value yet.
  if (const auto *UO = dyn_cast<UnaryOperator>(LHS);
      UO && UO->getOpcode() == UO_Deref)
    if (const auto *DRE =
            dyn_cast<DeclRefExpr>(UO->getSubExpr()->IgnoreParenImpCasts()))
      if (const auto *VD = dyn_cast<VarDecl>(DRE->getDecl());
          VD && VD->isImplicit() && VD->getName() == "__cx_place")
        return true;
  // A construction's own object holds no value in a field it assigns.
  if (const auto *ME = dyn_cast<MemberExpr>(LHS))
    if (const auto *DRE =
            dyn_cast<DeclRefExpr>(ME->getBase()->IgnoreParenImpCasts()))
      if (const auto *VD = dyn_cast<VarDecl>(DRE->getDecl());
          VD && VD->isImplicit() && VD->getName() == "__cx_object")
        return true;
  // An initializer's assignment to a field of `self` is its initialization;
  // definite initialization makes it the only one on its path.
  const auto *FD = dyn_cast_or_null<FunctionDecl>(CurCodeDecl);
  if (!FD || !isCxInit(FD) || !FD->getNumParams())
    return false;
  const auto *ME = dyn_cast<MemberExpr>(LHS);
  if (!ME)
    return false;
  const Expr *Base = ME->getBase()->IgnoreParenImpCasts();
  while (const auto *Inner = dyn_cast<MemberExpr>(Base)) {
    const auto *IFD = dyn_cast<FieldDecl>(Inner->getMemberDecl());
    if (!IFD || !IFD->isAnonymousStructOrUnion())
      return false;
    Base = Inner->getBase()->IgnoreParenImpCasts();
  }
  const auto *DRE = dyn_cast<DeclRefExpr>(Base);
  return DRE && DRE->getDecl() == FD->getParamDecl(0);
}
