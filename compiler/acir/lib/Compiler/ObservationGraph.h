#ifndef ACIR_LIB_COMPILER_OBSERVATIONGRAPH_H
#define ACIR_LIB_COMPILER_OBSERVATIONGRAPH_H

#include "ModuleGraph.h"

namespace acir::compiler {

struct ObservationBinding {
  std::uint32_t stableOrdinal = 0;
  size_t requiredIndex = 0;
  InstanceView *owner = nullptr;
  mlir::DictionaryAttr ownerRef;
  ac::RuleOp rule;
  mlir::DictionaryAttr registration;
  mlir::DictionaryAttr observationID;
  mlir::StringAttr kind;
  mlir::DictionaryAttr spec;
  mlir::ArrayAttr valueIDs;
  mlir::ArrayAttr valueConstraints;
  mlir::Value path;
  llvm::SmallVector<mlir::Value> values;
  ac::SourceObserveOp observe;
};

struct ObservationGraph {
  const ModuleGraph *modules = nullptr;
  llvm::SmallVector<ObservationBinding> bindings;
};

mlir::FailureOr<ObservationGraph>
buildSourceObservationGraph(const ModuleGraph &modules,
                            ac::detail::EmitError emitError);
mlir::LogicalResult verifySourceObservations(const ObservationGraph &graph,
                                             ac::detail::EmitError emitError);

} // namespace acir::compiler

#endif // ACIR_LIB_COMPILER_OBSERVATIONGRAPH_H
