//===--- SemaCxConsume.cpp - Cx consumed resources ------------------------===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//
//
// A resource local or by-value parameter is consumed when it is passed by
// value or returned. On no path may it be used after that, until it is
// assigned again; a variable consumed anywhere is marked, so that CodeGen
// destroys it at the end of its scope only when it still holds its value.
//
//===----------------------------------------------------------------------===//

#include "clang/AST/Attr.h"
#include "clang/AST/RecursiveASTVisitor.h"
#include "clang/Analysis/Analyses/PostOrderCFGView.h"
#include "clang/Analysis/CFG.h"
#include "clang/Analysis/FlowSensitive/DataflowWorklist.h"
#include "clang/Basic/DiagnosticSema.h"
#include "clang/Sema/Sema.h"
#include "llvm/ADT/BitVector.h"
#include "llvm/ADT/DenseMap.h"

using namespace clang;

namespace {

enum class RefKind { Consume, Use, Assign };

struct Ref {
  RefKind Kind;
  unsigned Var;
};

/// Finds every consume of a resource variable, then labels every reference
/// to a consumed variable.
class Classifier : public RecursiveASTVisitor<Classifier> {
  Sema &S;
  llvm::DenseSet<const Stmt *> Claimed;
  unsigned DeferDepth = 0;

public:
  llvm::DenseMap<const Stmt *, Ref> Refs;
  SmallVector<const VarDecl *, 4> Vars;
  llvm::DenseMap<const VarDecl *, unsigned> Index;
  llvm::DenseMap<const VarDecl *, const DeclRefExpr *> FirstConsume;
  llvm::DenseMap<const VarDecl *, const DeferStmt *> DeferUse;
  const DeferStmt *CurDefer = nullptr;
  bool Diagnosed = false;

  explicit Classifier(Sema &S) : S(S) {}

  /// The variable a consume reads: an lvalue-to-rvalue conversion is the only
  /// read of a resource local that Sema lets through.
  static const DeclRefExpr *consumed(const ImplicitCastExpr *ICE) {
    if (ICE->getCastKind() != CK_LValueToRValue)
      return nullptr;
    return dyn_cast<DeclRefExpr>(ICE->getSubExpr()->IgnoreParens());
  }

  unsigned var(const VarDecl *VD) {
    auto [It, New] = Index.try_emplace(VD, Vars.size());
    if (New)
      Vars.push_back(VD);
    return It->second;
  }

  bool TraverseDeferStmt(DeferStmt *D) {
    ++DeferDepth;
    const DeferStmt *Outer = CurDefer;
    CurDefer = D;
    bool Result = RecursiveASTVisitor::TraverseDeferStmt(D);
    CurDefer = Outer;
    --DeferDepth;
    return Result;
  }

  bool VisitImplicitCastExpr(ImplicitCastExpr *ICE) {
    const DeclRefExpr *DRE = consumed(ICE);
    if (!DRE || !S.isCxConsumable(DRE))
      return true;
    const auto *VD = cast<VarDecl>(DRE->getDecl());
    if (DeferDepth) {
      S.Diag(DRE->getLocation(), diag::err_cx_consume_in_defer) << VD;
      Diagnosed = true;
    }
    Claimed.insert(DRE);
    Refs[DRE] = Ref{RefKind::Consume, var(VD)};
    FirstConsume.try_emplace(VD, DRE);
    return true;
  }

  bool VisitDeclRefExpr(DeclRefExpr *DRE) {
    const auto *VD = dyn_cast<VarDecl>(DRE->getDecl());
    if (!VD || Claimed.count(DRE) || !S.isCxConsumable(DRE))
      return true;
    if (DeferDepth)
      DeferUse.try_emplace(VD, CurDefer);
    Refs.try_emplace(DRE, Ref{RefKind::Use, var(VD)});
    return true;
  }

  bool VisitBinaryOperator(BinaryOperator *BO) {
    if (BO->getOpcode() != BO_Assign)
      return true;
    const auto *DRE = dyn_cast<DeclRefExpr>(BO->getLHS()->IgnoreParens());
    if (!DRE || !S.isCxConsumable(DRE))
      return true;
    // Assigning gives the variable a value again.
    Claimed.insert(DRE);
    Refs[BO] = Ref{RefKind::Assign, var(cast<VarDecl>(DRE->getDecl()))};
    return true;
  }
};

struct State {
  llvm::BitVector May;
  bool Reached = false;

  void merge(const State &O) {
    if (!O.Reached)
      return;
    if (!Reached) {
      *this = O;
      return;
    }
    May |= O.May;
  }
  bool operator==(const State &O) const {
    return Reached == O.Reached && May == O.May;
  }
};

} // namespace

void Sema::CheckCxConsumes(FunctionDecl *FD, Stmt *Body) {
  Classifier C(*this);
  C.TraverseStmt(Body);
  if (C.FirstConsume.empty() || C.Diagnosed)
    return;

  // A deferred block runs after whatever consumes the variable.
  for (auto [VD, Consume] : C.FirstConsume)
    if (const DeferStmt *D = C.DeferUse.lookup(VD)) {
      Diag(Consume->getLocation(), diag::err_cx_consume_deferred_use) << VD;
      Diag(D->getDeferLoc(), diag::note_cx_deferred_use);
      return;
    }

  CFG::BuildOptions Options;
  Options.setAllAlwaysAdd();
  std::unique_ptr<CFG> G = CFG::buildCFG(FD, Body, &Context, Options);
  if (!G)
    return;
  PostOrderCFGView Order(G.get());
  unsigned NumVars = C.Vars.size();

  // Declaring a variable gives it a value; each loop iteration declares it
  // anew.
  llvm::DenseMap<const Stmt *, unsigned> Declares;
  for (const CFGBlock *B : *G)
    for (const CFGElement &E : *B)
      if (std::optional<CFGStmt> CS = E.getAs<CFGStmt>())
        if (const auto *DS = dyn_cast<DeclStmt>(CS->getStmt()))
          for (const Decl *D : DS->decls())
            if (auto It = C.Index.find(dyn_cast<VarDecl>(D));
                It != C.Index.end())
              Declares[DS] = It->second;

  auto Apply = [&](const Stmt *S, State &St, bool Report) {
    if (auto It = Declares.find(S); It != Declares.end()) {
      St.May.reset(It->second);
      return;
    }
    auto It = C.Refs.find(S);
    if (It == C.Refs.end())
      return;
    const Ref &R = It->second;
    const VarDecl *VD = C.Vars[R.Var];
    switch (R.Kind) {
    case RefKind::Assign:
      St.May.reset(R.Var);
      return;
    case RefKind::Use:
    case RefKind::Consume:
      if (Report && St.May[R.Var]) {
        Diag(S->getBeginLoc(), diag::err_cx_used_after_consume) << VD;
        Diag(C.FirstConsume.lookup(VD)->getLocation(),
             diag::note_cx_consumed_here);
      }
      if (R.Kind == RefKind::Consume)
        St.May.set(R.Var);
      return;
    }
  };

  SmallVector<State, 16> In(G->getNumBlockIDs()), Out(G->getNumBlockIDs());
  State Entry;
  Entry.Reached = true;
  Entry.May.resize(NumVars);
  ForwardDataflowWorklist Worklist(*G, &Order);
  Worklist.enqueueBlock(&G->getEntry());
  while (const CFGBlock *B = Worklist.dequeue()) {
    State St;
    if (B == &G->getEntry())
      St = Entry;
    for (const CFGBlock::AdjacentBlock &P : B->preds())
      if (P)
        St.merge(Out[P->getBlockID()]);
    if (!St.Reached)
      continue;
    In[B->getBlockID()] = St;
    for (const CFGElement &E : *B)
      if (std::optional<CFGStmt> CS = E.getAs<CFGStmt>())
        Apply(CS->getStmt(), St, /*Report=*/false);
    if (St == Out[B->getBlockID()])
      continue;
    Out[B->getBlockID()] = St;
    Worklist.enqueueSuccessors(B);
  }

  for (const CFGBlock *B : *G) {
    State St = In[B->getBlockID()];
    if (!St.Reached)
      continue;
    for (const CFGElement &E : *B)
      if (std::optional<CFGStmt> CS = E.getAs<CFGStmt>())
        Apply(CS->getStmt(), St, /*Report=*/true);
  }

  // A consumed variable is destroyed at the end of its scope only when it
  // still holds a value there.
  for (const VarDecl *VD : C.Vars)
    if (C.FirstConsume.count(VD) && !VD->hasAttr<CxConsumedAttr>())
      const_cast<VarDecl *>(VD)->addAttr(CxConsumedAttr::CreateImplicit(Context));
}
