#ifndef ACIR_LIB_COMPILER_MODULEGRAPH_H
#define ACIR_LIB_COMPILER_MODULEGRAPH_H

#include "SourceLink.h"

#include "llvm/ADT/DenseMap.h"

#include <memory>

namespace acir::compiler {

struct InstanceView {
  mlir::DictionaryAttr owner;
  mlir::FlatSymbolRefAttr definition;
  mlir::ArrayAttr staticArguments;
  ac::ModuleOp module;
  ac::ModuleImportOp header;
  ac::InstanceOp placement;
  InstanceView *parent = nullptr;
  llvm::SmallVector<InstanceView *> children;
  llvm::SmallVector<mlir::DictionaryAttr> ownedStateIDs;
  llvm::DenseMap<mlir::Attribute, mlir::DictionaryAttr> ownedStates;
  llvm::DenseMap<mlir::Attribute, mlir::DictionaryAttr> formalAliases;
};

class ModuleGraph {
public:
  InstanceView *root = nullptr;
  llvm::SmallVector<InstanceView *> views;

  mlir::FailureOr<mlir::DictionaryAttr>
  resolveState(const InstanceView &owner, mlir::DictionaryAttr state,
               ac::detail::EmitError emitError) const;
  llvm::ArrayRef<InstanceView *> postOrder() const { return postOrder_; }

private:
  friend mlir::FailureOr<ModuleGraph>
  buildSourceModuleGraph(llvm::ArrayRef<SourceLinkUnit> units,
                         const SourceHeaderRegistry &registry,
                         ac::detail::EmitError emitError);
  friend mlir::FailureOr<ModuleGraph>
  rebuildFinalModuleGraphFromHardware(mlir::ModuleOp package,
                                      ac::detail::EmitError emitError);

  llvm::SmallVector<std::unique_ptr<InstanceView>> storage_;
  llvm::SmallVector<InstanceView *> postOrder_;
  llvm::DenseMap<mlir::Attribute, mlir::DictionaryAttr> stateTypes_;
};

mlir::FailureOr<ModuleGraph>
buildSourceModuleGraph(llvm::ArrayRef<SourceLinkUnit> units,
                       const SourceHeaderRegistry &registry,
                       ac::detail::EmitError emitError);
mlir::FailureOr<ModuleGraph>
rebuildFinalModuleGraphFromHardware(mlir::ModuleOp package,
                                    ac::detail::EmitError emitError);

} // namespace acir::compiler

#endif // ACIR_LIB_COMPILER_MODULEGRAPH_H
