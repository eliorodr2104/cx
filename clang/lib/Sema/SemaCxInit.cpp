//===--- SemaCxInit.cpp - Cx definite initialization ----------------------===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//
//
// A Cx initializer initializes every field of `self` on every path before it
// returns, uses no field before initializing it and `self` as a whole only
// once it is complete; a delegating one calls `self.init` exactly once. The
// analysis runs on Clang's CFG, like -Wuninitialized, with a bit per field.
//
//===----------------------------------------------------------------------===//

#include "clang/AST/RecursiveASTVisitor.h"
#include "clang/Analysis/Analyses/PostOrderCFGView.h"
#include "clang/Analysis/CFG.h"
#include "clang/Analysis/FlowSensitive/DataflowWorklist.h"
#include "clang/Basic/DiagnosticSema.h"
#include "clang/Sema/Sema.h"
#include "llvm/ADT/BitVector.h"
#include "llvm/ADT/DenseMap.h"
#include "llvm/ADT/DenseSet.h"

using namespace clang;

namespace {

/// The fields of `self` as a tree: named fields are leaves with a bit each;
/// an anonymous struct is initialized when all its members are, an anonymous
/// union when any one is.
struct FieldTree {
  struct Node {
    const FieldDecl *Field = nullptr; // Null for the record itself.
    int Parent = -1;
    bool IsUnion = false;
    int Bit = -1; // Leaves only.
    SmallVector<unsigned, 4> Children;
  };
  SmallVector<Node, 8> Nodes;
  llvm::DenseMap<const FieldDecl *, unsigned> Index;
  unsigned NumBits = 0;
  llvm::BitVector Defaulted;

  explicit FieldTree(const RecordDecl *RD) {
    Nodes.emplace_back();
    add(RD, 0);
    Defaulted.resize(NumBits);
    for (const Node &N : Nodes)
      if (N.Bit >= 0 && isDefaulted(N.Field))
        Defaulted.set(N.Bit);
  }

  void add(const RecordDecl *RD, unsigned Parent) {
    for (const FieldDecl *FD : RD->fields()) {
      // A flexible array member has no storage in a value, and an unnamed
      // bit-field is padding.
      if (FD->getType()->isIncompleteArrayType() || FD->isUnnamedBitField())
        continue;
      unsigned I = Nodes.size();
      Nodes.emplace_back();
      Nodes[I].Field = FD;
      Nodes[I].Parent = Parent;
      Nodes[Parent].Children.push_back(I);
      Index[FD] = I;
      if (FD->isAnonymousStructOrUnion()) {
        const RecordDecl *Inner = FD->getType()->getAsRecordDecl();
        Nodes[I].IsUnion = Inner->isUnion();
        add(Inner, I);
      } else {
        Nodes[I].Bit = NumBits++;
      }
    }
  }

  /// A field holds a value before the initializer runs when it has a default
  /// or its type is built entirely from defaults.
  static bool isDefaulted(const FieldDecl *FD) {
    return FD->hasInClassInitializer() || isFullyDefaulted(FD->getType());
  }
  static bool isFullyDefaulted(QualType T) {
    const RecordDecl *RD = T->getBaseElementTypeUnsafe()->getAsRecordDecl();
    if (!RD || !(RD = RD->getDefinition()) || RD->field_empty())
      return false;
    // A C record cannot contain itself by value, so this terminates.
    return llvm::all_of(RD->fields(), [](const FieldDecl *FD) {
      return FD->isUnnamedBitField() || isDefaulted(FD);
    });
  }

  bool isInitialized(unsigned I, const llvm::BitVector &Must) const {
    const Node &N = Nodes[I];
    if (N.Bit >= 0)
      return Must[N.Bit];
    auto Init = [&](unsigned C) { return isInitialized(C, Must); };
    return N.IsUnion ? llvm::any_of(N.Children, Init)
                     : llvm::all_of(N.Children, Init);
  }

  /// A leaf may be read once it is written, or once a union holding it is
  /// initialized through another member: C lets a union be read through any
  /// of its members.
  bool isReadable(unsigned Leaf, const llvm::BitVector &Must) const {
    if (Must[Nodes[Leaf].Bit])
      return true;
    for (int P = Nodes[Leaf].Parent; P > 0; P = Nodes[P].Parent)
      if (Nodes[P].IsUnion)
        return isInitialized(P, Must);
    return false;
  }

  /// The first node that keeps \p I from being initialized: a leaf, or an
  /// anonymous union none of whose members is initialized.
  int firstMissing(unsigned I, const llvm::BitVector &Must) const {
    if (isInitialized(I, Must))
      return -1;
    const Node &N = Nodes[I];
    if (N.Bit >= 0 || N.IsUnion)
      return I;
    for (unsigned C : N.Children)
      if (int M = firstMissing(C, Must); M >= 0)
        return M;
    return -1;
  }

  /// Every node that keeps \p I from being initialized, in field order.
  void missing(unsigned I, const llvm::BitVector &Must,
               SmallVectorImpl<unsigned> &Out) const {
    if (isInitialized(I, Must))
      return;
    const Node &N = Nodes[I];
    if (N.Bit >= 0 || N.IsUnion) {
      Out.push_back(I);
      return;
    }
    for (unsigned C : N.Children)
      missing(C, Must, Out);
  }
};

enum class RefKind { Init, Use, SelfUse, Delegate };

struct Ref {
  RefKind Kind;
  unsigned Leaf = 0;
  bool InDefer = false;
  /// A compound assignment or increment of a const field: both a read and a
  /// second assignment.
  bool Update = false;
};

/// Labels every reference to `self` in an initializer's body with what it
/// does. The walk is pre-order, so an assignment claims its target before
/// the target is visited as a read.
class Classifier : public RecursiveASTVisitor<Classifier> {
  Sema &S;
  const FieldTree &Tree;
  llvm::DenseSet<const Stmt *> Claimed;
  unsigned DeferDepth = 0;

public:
  llvm::DenseMap<const Stmt *, Ref> Refs;
  bool Delegates = false;

  Classifier(Sema &S, const FieldTree &Tree) : S(S), Tree(Tree) {}

  /// The leaf of `self` that \p E names, claiming the member accesses and the
  /// `self` that reach it, or -1.
  int selfLeaf(const Expr *E) {
    const auto *ME = dyn_cast<MemberExpr>(E->IgnoreParens());
    if (!ME)
      return -1;
    const auto *FD = dyn_cast<FieldDecl>(ME->getMemberDecl());
    auto It = FD ? Tree.Index.find(FD) : Tree.Index.end();
    if (It == Tree.Index.end() || Tree.Nodes[It->second].Bit < 0)
      return -1;
    SmallVector<const Stmt *, 4> Chain{ME};
    const Expr *Base = ME->getBase()->IgnoreParenImpCasts();
    while (const auto *Inner = dyn_cast<MemberExpr>(Base)) {
      const auto *IFD = dyn_cast<FieldDecl>(Inner->getMemberDecl());
      if (!IFD || !IFD->isAnonymousStructOrUnion())
        return -1;
      Chain.push_back(Inner);
      Base = Inner->getBase()->IgnoreParenImpCasts();
    }
    if (const auto *UO = dyn_cast<UnaryOperator>(Base);
        UO && UO->getOpcode() == UO_Deref) {
      Chain.push_back(UO);
      Base = UO->getSubExpr()->IgnoreParenImpCasts();
    }
    if (!S.isCxSelfReference(Base))
      return -1;
    Chain.push_back(Base);
    Claimed.insert(Chain.begin(), Chain.end());
    return It->second;
  }

  void add(const Stmt *At, RefKind K, unsigned Leaf = 0, bool Update = false) {
    Refs[At] = Ref{K, Leaf, DeferDepth > 0, Update};
  }

  bool TraverseDeferStmt(DeferStmt *D) {
    ++DeferDepth;
    bool Result = RecursiveASTVisitor::TraverseDeferStmt(D);
    --DeferDepth;
    return Result;
  }

  /// A const or resource field is initialized once: a later assignment would
  /// overwrite a value that cannot be written again, or destroy one.
  bool isConstLeaf(unsigned Leaf) const {
    QualType T = Tree.Nodes[Leaf].Field->getType();
    return T.isConstQualified() || S.isCxResourceType(T);
  }

  bool VisitBinaryOperator(BinaryOperator *BO) {
    if (BO->getOpcode() == BO_Assign) {
      if (int Leaf = selfLeaf(BO->getLHS()); Leaf >= 0)
        add(BO, RefKind::Init, Leaf);
    } else if (BO->isCompoundAssignmentOp()) {
      if (int Leaf = selfLeaf(BO->getLHS()); Leaf >= 0)
        add(BO, isConstLeaf(Leaf) ? RefKind::Init : RefKind::Use, Leaf,
            /*Update=*/true);
    }
    return true;
  }

  bool VisitUnaryOperator(UnaryOperator *UO) {
    if (UO->isIncrementDecrementOp())
      if (int Leaf = selfLeaf(UO->getSubExpr()); Leaf >= 0)
        add(UO, isConstLeaf(Leaf) ? RefKind::Init : RefKind::Use, Leaf,
            /*Update=*/true);
    return true;
  }

  bool VisitCallExpr(CallExpr *CE) {
    const FunctionDecl *Callee = CE->getDirectCallee();
    if (Callee && S.isCxInitializer(Callee) && CE->getNumArgs() &&
        S.isCxSelfReference(CE->getArg(0)->IgnoreParenImpCasts())) {
      Claimed.insert(CE->getArg(0)->IgnoreParenImpCasts());
      add(CE, RefKind::Delegate);
      Delegates = true;
    }
    return true;
  }

  bool VisitMemberExpr(MemberExpr *ME) {
    if (Claimed.count(ME))
      return true;
    if (int Leaf = selfLeaf(ME); Leaf >= 0)
      add(ME, RefKind::Use, Leaf);
    return true;
  }

  bool VisitDeclRefExpr(DeclRefExpr *DRE) {
    if (!Claimed.count(DRE) && S.isCxSelfReference(DRE))
      add(DRE, RefKind::SelfUse);
    return true;
  }
};

struct State {
  llvm::BitVector Must, May;
  bool MustDel = false, MayDel = false, Reached = false;

  void merge(const State &O) {
    if (!O.Reached)
      return;
    if (!Reached) {
      *this = O;
      return;
    }
    Must &= O.Must;
    May |= O.May;
    MustDel = MustDel && O.MustDel;
    MayDel = MayDel || O.MayDel;
  }
  bool operator==(const State &O) const {
    return Reached == O.Reached && Must == O.Must && May == O.May &&
           MustDel == O.MustDel && MayDel == O.MayDel;
  }
};

class Analysis {
  Sema &S;
  const FieldTree &Tree;
  const Classifier &C;
  unsigned Root = 0;

public:
  bool Report = false;

  Analysis(Sema &S, const FieldTree &Tree, const Classifier &C)
      : S(S), Tree(Tree), C(C) {}

  const FieldDecl *field(unsigned Leaf) const {
    return Tree.Nodes[Leaf].Field;
  }

  void diagnoseSelfUse(SourceLocation Loc, const State &St) {
    int M = Tree.firstMissing(Root, St.Must);
    if (M < 0 || !Report)
      return;
    if (Tree.Nodes[M].IsUnion)
      S.Diag(Loc, diag::err_cx_init_self_use_union);
    else
      S.Diag(Loc, diag::err_cx_init_self_use) << field(M);
  }

  void apply(const Stmt *At, State &St) {
    auto It = C.Refs.find(At);
    if (It == C.Refs.end())
      return;
    const Ref &R = It->second;
    SourceLocation Loc = At->getBeginLoc();
    if (const auto *E = dyn_cast<Expr>(At))
      Loc = E->getExprLoc();
    bool BeforeDelegation = C.Delegates && !St.MustDel;

    switch (R.Kind) {
    case RefKind::Init: {
      unsigned Bit = Tree.Nodes[R.Leaf].Bit;
      QualType FT = field(R.Leaf)->getType();
      bool Resource = S.isCxResourceType(FT);
      bool Const = FT.isConstQualified() || Resource;
      if (R.InDefer) {
        // The block runs at the exit, after whatever follows it here.
        if (Report && !Tree.isReadable(R.Leaf, St.Must))
          S.Diag(Loc, diag::err_cx_init_defer_initializes) << field(R.Leaf) << 0;
        else if (Report && Const)
          S.Diag(Loc, diag::err_cx_init_defer_initializes)
              << field(R.Leaf) << (Resource ? 2 : 1);
        return;
      }
      if (BeforeDelegation) {
        if (Report)
          S.Diag(Loc, diag::err_cx_delegation_before);
      } else if (R.Update && !Tree.isReadable(R.Leaf, St.Must)) {
        if (Report)
          S.Diag(Loc, diag::err_cx_init_field_use) << field(R.Leaf);
      } else if (Const && St.May[Bit]) {
        if (Report)
          S.Diag(Loc, diag::err_cx_init_const_reassigned)
              << field(R.Leaf) << Resource;
      }
      St.Must.set(Bit);
      St.May.set(Bit);
      return;
    }
    case RefKind::Use:
      if (!Report)
        return;
      if (BeforeDelegation)
        S.Diag(Loc, diag::err_cx_delegation_before);
      else if (!Tree.isReadable(R.Leaf, St.Must))
        S.Diag(Loc, diag::err_cx_init_field_use) << field(R.Leaf);
      return;
    case RefKind::SelfUse:
      if (BeforeDelegation) {
        if (Report)
          S.Diag(Loc, diag::err_cx_delegation_before);
      } else {
        diagnoseSelfUse(Loc, St);
      }
      return;
    case RefKind::Delegate:
      if (R.InDefer) {
        if (Report)
          S.Diag(Loc, diag::err_cx_delegation_in_defer);
        return;
      }
      if (Report && St.MayDel)
        S.Diag(Loc, diag::err_cx_delegation_twice);
      St.MustDel = St.MayDel = true;
      St.Must.set();
      St.May.set();
      return;
    }
  }

  void applyBlock(const CFGBlock *B, State &St) {
    for (const CFGElement &E : *B)
      if (std::optional<CFGStmt> CS = E.getAs<CFGStmt>())
        apply(CS->getStmt(), St);
  }

  /// What is missing when a path leaves the initializer from \p B.
  void diagnoseExit(const CFGBlock *B, const State &St, const Stmt *Body) {
    SourceLocation Loc = Body->getEndLoc();
    unsigned Returns = 1;
    for (const CFGElement &E : llvm::reverse(*B))
      if (std::optional<CFGStmt> CS = E.getAs<CFGStmt>()) {
        if (const auto *RS = dyn_cast<ReturnStmt>(CS->getStmt())) {
          Loc = RS->getReturnLoc();
          Returns = 0;
        }
        break;
      }

    if (C.Delegates) {
      if (!St.MustDel)
        S.Diag(Loc, diag::err_cx_delegation_missing) << Returns;
      return;
    }
    SmallVector<unsigned, 4> Missing;
    Tree.missing(Root, St.Must, Missing);
    for (unsigned M : Missing) {
      const FieldDecl *FD = field(M);
      if (Tree.Nodes[M].IsUnion) {
        S.Diag(Loc, diag::err_cx_init_union_missing) << Returns;
        S.Diag(FD->getLocation(), diag::note_cx_init_field_declared) << 2;
        continue;
      }
      S.Diag(Loc, diag::err_cx_init_field_missing) << FD << Returns;
      S.Diag(FD->getLocation(), diag::note_cx_init_field_declared)
          << (FD->getType()->isArrayType() ? 1 : 0);
    }
  }
};

} // namespace

void Sema::CheckCxDefiniteInitialization(FunctionDecl *FD, Stmt *Body) {
  RecordDecl *RD = nullptr;
  if (FD->getNumParams())
    if (const auto *PT = FD->getParamDecl(0)->getType()->getAs<PointerType>())
      RD = PT->getPointeeType()->getAsRecordDecl();
  if (!RD || !(RD = RD->getDefinition()))
    return;

  FieldTree Tree(RD);
  Classifier C(*this, Tree);
  C.TraverseStmt(Body);

  // The body is not attached to the function yet, so the CFG is built from
  // it directly.
  CFG::BuildOptions Options;
  Options.setAllAlwaysAdd();
  std::unique_ptr<CFG> G = CFG::buildCFG(FD, Body, &Context, Options);
  if (!G)
    return;
  PostOrderCFGView Order(G.get());

  Analysis A(*this, Tree, C);
  State Entry;
  Entry.Reached = true;
  Entry.Must = Tree.Defaulted;
  Entry.May = Tree.Defaulted;
  SmallVector<State, 16> In(G->getNumBlockIDs()), Out(G->getNumBlockIDs());

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
    A.applyBlock(B, St);
    if (St == Out[B->getBlockID()])
      continue;
    Out[B->getBlockID()] = St;
    Worklist.enqueueSuccessors(B);
  }

  // Each element is visited once more, now reporting, from its fixed state.
  A.Report = true;
  for (const CFGBlock *B : *G) {
    State St = In[B->getBlockID()];
    if (!St.Reached)
      continue;
    A.applyBlock(B, St);
  }

  const CFGBlock &Exit = G->getExit();
  for (const CFGBlock::AdjacentBlock &P : Exit.preds())
    if (P && Out[P->getBlockID()].Reached && !P->hasNoReturnElement())
      A.diagnoseExit(P, Out[P->getBlockID()], Body);
}
