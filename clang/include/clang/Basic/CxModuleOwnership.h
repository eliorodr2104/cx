//===--- CxModuleOwnership.h - Cx per-file module ownership -----*- C++ -*-===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//
///
/// \file
/// Records which Cx module owns each physical source file.
///
/// Ownership is a property of a file, not preprocessor state that leaks across
/// an include: a file owned by one module can include a file owned by another,
/// or a plain C header owned by nothing at all, and neither acquires the
/// other's owner. The map is keyed by FileID for exactly that reason.
///
//===----------------------------------------------------------------------===//

#ifndef LLVM_CLANG_BASIC_CXMODULEOWNERSHIP_H
#define LLVM_CLANG_BASIC_CXMODULEOWNERSHIP_H

#include "clang/Basic/SourceLocation.h"
#include "llvm/ADT/DenseMap.h"
#include "llvm/ADT/StringMap.h"
#include <string>

namespace clang {

class IdentifierInfo;

class CxModuleOwnership {
public:
  struct Owner {
    /// The owning module's name.
    const IdentifierInfo *Name = nullptr;

    /// Where `#module` was written, or an invalid location when the build
    /// assigned the owner instead.
    SourceLocation DirectiveLoc;

    /// Whether this owner came from the build rather than from source.
    bool FromBuild = false;

    explicit operator bool() const { return Name != nullptr; }
  };

  /// The owner of \p FID, or an empty Owner when the file declares none.
  /// A file with no owner is an ordinary C file and stays one.
  Owner getOwner(FileID FID) const {
    auto It = Owners.find(FID);
    return It == Owners.end() ? Owner{} : It->second;
  }

  /// Record \p O as the owner of \p FID. Returns false, changing nothing, if
  /// the file already has one; the caller diagnoses the conflict.
  bool setOwner(FileID FID, Owner O) {
    return Owners.try_emplace(FID, O).second;
  }

  bool empty() const { return Owners.empty(); }

  /// Note that \p FID has begun an ordinary declaration. This is recorded by
  /// the parser rather than the preprocessor, which is what makes it
  /// transparent to directives: a header guard is not a declaration, and the
  /// preprocessor's own "read any tokens" flag cannot tell the two apart.
  void noteDeclarationIn(FileID FID, SourceLocation Loc) {
    WithDeclarations.try_emplace(FID, Loc);
  }

  /// Whether an ordinary declaration has been started in \p FID.
  bool hasDeclarations(FileID FID) const {
    return WithDeclarations.contains(FID);
  }

  /// Where the first ordinary declaration in \p FID starts.
  SourceLocation getFirstDeclarationIn(FileID FID) const {
    auto It = WithDeclarations.find(FID);
    return It == WithDeclarations.end() ? SourceLocation() : It->second;
  }

  /// Preprocessed input -- the output of `-E` or `-frewrite-includes` --
  /// holds many source files in one, separated by GNU line markers. Once
  /// \p FID has one, ownership follows the file each marker names, which is
  /// what `-E` writes a `#module` for. \p Presumed is the first such name,
  /// which is the primary source file the input was produced from.
  void notePresumedFiles(FileID FID, StringRef Presumed) {
    PresumedPrimary.try_emplace(FID, Presumed.str());
  }

  /// Whether ownership in \p FID follows line markers.
  bool usesPresumedFiles(FileID FID) const {
    return PresumedPrimary.contains(FID);
  }

  /// The owner of the source file \p Presumed, named by a line marker in
  /// \p FID. The primary file also takes an owner the build assigned to the
  /// input as a whole.
  Owner getPresumedOwner(FileID FID, StringRef Presumed) const {
    auto It = PresumedOwners.find(Presumed);
    if (It != PresumedOwners.end())
      return It->second;
    auto Primary = PresumedPrimary.find(FID);
    if (Primary != PresumedPrimary.end() && Primary->second == Presumed) {
      Owner O = getOwner(FID);
      if (O.FromBuild)
        return O;
    }
    return Owner{};
  }

  /// Record \p O as the owner of the source file \p Presumed. Returns false,
  /// changing nothing, if it already has one.
  bool setPresumedOwner(StringRef Presumed, Owner O) {
    return PresumedOwners.try_emplace(Presumed, O).second;
  }

  using const_iterator = llvm::DenseMap<FileID, Owner>::const_iterator;
  const_iterator begin() const { return Owners.begin(); }
  const_iterator end() const { return Owners.end(); }

private:
  llvm::DenseMap<FileID, Owner> Owners;
  llvm::DenseMap<FileID, SourceLocation> WithDeclarations;
  llvm::DenseMap<FileID, std::string> PresumedPrimary;
  llvm::StringMap<Owner> PresumedOwners;
};

} // namespace clang

#endif // LLVM_CLANG_BASIC_CXMODULEOWNERSHIP_H
