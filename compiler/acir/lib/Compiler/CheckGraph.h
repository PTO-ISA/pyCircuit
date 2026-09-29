#ifndef ACIR_LIB_COMPILER_CHECKGRAPH_H
#define ACIR_LIB_COMPILER_CHECKGRAPH_H

#include "ModuleGraph.h"

namespace acir::compiler {

struct CheckBinding {
  std::uint32_t stableOrdinal = 0;
  size_t requiredIndex = 0;
  InstanceView *owner = nullptr;
  mlir::DictionaryAttr ownerRef;
  ac::RuleOp rule;
  mlir::DictionaryAttr registration;
  ac::SourceExpectOp expect;
  mlir::DictionaryAttr checkID;
  mlir::StringAttr kind;
  mlir::DictionaryAttr location;
  mlir::Value condition;
  mlir::Value path;
};

struct CheckGraph {
  const ModuleGraph *modules = nullptr;
  llvm::SmallVector<CheckBinding> bindings;
};

mlir::FailureOr<CheckGraph>
buildSourceCheckGraph(const ModuleGraph &modules,
                      ac::detail::EmitError emitError);
mlir::LogicalResult verifySourceChecks(const CheckGraph &graph,
                                       ac::detail::EmitError emitError);

} // namespace acir::compiler

#endif // ACIR_LIB_COMPILER_CHECKGRAPH_H
