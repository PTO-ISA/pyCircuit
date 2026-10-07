#ifndef ACIR_LIB_COMPILER_SOURCEUNIT_H
#define ACIR_LIB_COMPILER_SOURCEUNIT_H

#include "Dialect/ACIR/ACIRSourceContracts.h"
#include "pycircuit/Dialect/ACIR/ACIROps.h"

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

// One immutable module declaration admitted by the source interface registry.
// Ordinary declarations are owned by their published source header. Standard
// leaves are owned by the registry's closed builtin catalog; callers must use
// `isTrustedBuiltin` rather than trusting primitive-looking attributes.
struct ModuleInterfaceView {
  ac::ModuleImportOp declaration;
  mlir::FlatSymbolRefAttr symbol;
  mlir::DictionaryAttr sourceOwner;
  mlir::ArrayAttr parameters;
  mlir::FunctionType functionType;
  mlir::ArrayAttr typeParameters;
  mlir::ArrayAttr inputNames;
  mlir::ArrayAttr outputNames;
  mlir::ArrayAttr outputDependencies;
  mlir::StringAttr builtinKind;
  mlir::StringAttr sourceReturnForm;
  mlir::ArrayAttr sourceParameters;
  mlir::ArrayAttr sourceResultConstraints;
  mlir::DictionaryAttr domainInputs;

  explicit operator bool() const { return static_cast<bool>(declaration); }
  bool isBuiltin() const { return static_cast<bool>(builtinKind); }
  bool hasSourceCallContract() const {
    return static_cast<bool>(sourceReturnForm);
  }
};

// Check the integrity of one retained source body/interface pair without
// admitting its interface as link-wide authority. Full source-link admission
// still validates declarations against the complete supplied header set.
mlir::LogicalResult verifyIntrinsicSourceUnitOwnerPair(
    mlir::ModuleOp body, mlir::ModuleOp interface, bool &ownerMismatch,
    ac::detail::EmitError emitError);

mlir::LogicalResult
verifyIntrinsicSourceUnitPair(mlir::ModuleOp body, mlir::ModuleOp interface,
                              ac::detail::EmitError emitError);

class SourceHeaderRegistry {
public:
  static mlir::FailureOr<SourceHeaderRegistry>
  create(llvm::ArrayRef<mlir::ModuleOp> headers,
         ac::detail::EmitError emitError);
  static mlir::FailureOr<SourceHeaderRegistry>
  create(mlir::MLIRContext *context, llvm::ArrayRef<mlir::ModuleOp> headers,
         ac::detail::EmitError emitError);

  mlir::FailureOr<ac::detail::ResolvedRecordView>
  resolveRecord(mlir::FlatSymbolRefAttr symbol) const;

  ac::TypeAliasOp lookupAlias(mlir::FlatSymbolRefAttr symbol) const;
  ac::StructOp lookupRecord(mlir::FlatSymbolRefAttr symbol) const;
  mlir::func::FuncOp lookupHelper(mlir::FlatSymbolRefAttr symbol) const;
  mlir::Operation *lookupDeclaration(mlir::FlatSymbolRefAttr canonical) const;
  ModuleInterfaceView lookupModule(mlir::FlatSymbolRefAttr canonical) const;
  ModuleInterfaceView lookupBuiltin(llvm::StringRef kind) const;
  bool isTrustedBuiltin(ac::ModuleImportOp declaration) const;
  mlir::LogicalResult verifyModuleImport(ac::ModuleImportOp declaration,
                                         ac::detail::EmitError emitError) const;
  mlir::DictionaryAttr ownerForModule(llvm::StringRef moduleName) const;
  mlir::FlatSymbolRefAttr lookupExport(llvm::StringRef moduleName,
                                       llvm::StringRef sourceName) const;
  mlir::ArrayAttr interfacesForModule(llvm::StringRef moduleName) const;
  llvm::ArrayRef<mlir::ModuleOp> suppliedHeaders() const { return headers_; }

  mlir::LogicalResult
  verifyBodySnapshots(mlir::ModuleOp body, mlir::ModuleOp owningHeader,
                      ac::detail::EmitError emitError) const;

private:
  mlir::OwningOpRef<mlir::ModuleOp> builtinCatalog_;
  llvm::SmallVector<mlir::ModuleOp> headers_;
  llvm::DenseMap<mlir::Attribute, mlir::Operation *> authorities_;
  llvm::DenseMap<mlir::Attribute, ac::TypeAliasOp> aliases_;
  llvm::DenseMap<mlir::Attribute, ac::StructOp> records_;
  llvm::DenseMap<mlir::Attribute, mlir::func::FuncOp> helpers_;
  llvm::DenseMap<mlir::Attribute, ac::ModuleImportOp> modules_;
  llvm::StringMap<ac::ModuleImportOp> builtins_;
  llvm::StringMap<mlir::DictionaryAttr> moduleOwners_;
  llvm::StringMap<mlir::ArrayAttr> interfaceClosures_;
  llvm::StringMap<mlir::FlatSymbolRefAttr> exports_;
};

mlir::FailureOr<SourceUnitArtifacts> compilePythonSourceUnit(
    mlir::ModuleOp transport, mlir::DictionaryAttr sourceOwner,
    const SourceHeaderRegistry &headers, ac::detail::EmitError emitError);

mlir::LogicalResult
verifyPublishedDependencies(ac::ModuleOp definition,
                            ac::ModuleImportOp declaration,
                            ac::detail::EmitError emitError);

} // namespace acir::compiler

#endif // ACIR_LIB_COMPILER_SOURCEUNIT_H
