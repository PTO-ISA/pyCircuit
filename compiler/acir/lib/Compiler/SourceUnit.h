#ifndef ACIR_LIB_COMPILER_SOURCEUNIT_H
#define ACIR_LIB_COMPILER_SOURCEUNIT_H

#include "Dialect/ACIR/ACIRSourceContracts.h"
#include "acir/Dialect/ACIR/ACIROps.h"

#include "mlir/Dialect/Func/IR/FuncOps.h"
#include "mlir/IR/BuiltinOps.h"
#include "llvm/ADT/ArrayRef.h"
#include "llvm/ADT/DenseMap.h"
#include "llvm/ADT/SmallVector.h"
#include "llvm/ADT/StringMap.h"

namespace acir::compiler {

struct SourceUnitArtifacts {
  mlir::OwningOpRef<mlir::ModuleOp> body;
  mlir::OwningOpRef<mlir::ModuleOp> interface;
};

class SourceHeaderRegistry {
public:
  static mlir::FailureOr<SourceHeaderRegistry>
  create(llvm::ArrayRef<mlir::ModuleOp> headers, ac::detail::EmitError emitError);

  mlir::FailureOr<ac::detail::ResolvedRecordView>
  resolveRecord(mlir::FlatSymbolRefAttr symbol) const;

  ac::TypeAliasOp lookupAlias(mlir::FlatSymbolRefAttr symbol) const;
  ac::StructOp lookupRecord(mlir::FlatSymbolRefAttr symbol) const;
  mlir::func::FuncOp lookupHelper(mlir::FlatSymbolRefAttr symbol) const;
  mlir::DictionaryAttr ownerForModule(llvm::StringRef moduleName) const;
  mlir::FlatSymbolRefAttr lookupExport(llvm::StringRef moduleName,
                                       llvm::StringRef sourceName) const;
  llvm::ArrayRef<mlir::ModuleOp> suppliedHeaders() const { return headers_; }

private:
  llvm::SmallVector<mlir::ModuleOp> headers_;
  llvm::DenseMap<mlir::Attribute, mlir::Operation *> authorities_;
  llvm::DenseMap<mlir::Attribute, ac::TypeAliasOp> aliases_;
  llvm::DenseMap<mlir::Attribute, ac::StructOp> records_;
  llvm::DenseMap<mlir::Attribute, mlir::func::FuncOp> helpers_;
  llvm::StringMap<mlir::DictionaryAttr> moduleOwners_;
  llvm::StringMap<mlir::FlatSymbolRefAttr> exports_;
};

mlir::FailureOr<SourceUnitArtifacts> compilePythonSourceUnit(
    mlir::ModuleOp transport, mlir::DictionaryAttr sourceOwner,
    const SourceHeaderRegistry &headers, ac::detail::EmitError emitError);

} // namespace acir::compiler

#endif // ACIR_LIB_COMPILER_SOURCEUNIT_H
