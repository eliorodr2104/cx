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

private:
  llvm::DenseMap<FileID, Owner> Owners;
};

} // namespace clang

#endif // LLVM_CLANG_BASIC_CXMODULEOWNERSHIP_H
