#ifndef PYCIRCUIT_INTERFACES_LOGICALSTATEINTERFACES_H
#define PYCIRCUIT_INTERFACES_LOGICALSTATEINTERFACES_H

#include "mlir/IR/OpDefinition.h"

namespace mlir {
class Operation;
} // namespace mlir

namespace acir {

mlir::LogicalResult verifyLogicalStateStructure(mlir::Operation *);
mlir::LogicalResult verifyLogicalStateUpdateStructure(mlir::Operation *);

#include "pycircuit/Interfaces/LogicalStateInterfaces.h.inc"

/// Native op trait wiring the fixed structural contract into automatic
/// operation verification: every op carrying this trait has its interface
/// structure verified by the standard MLIR verifier, independent of the
/// op's own verify() choosing to call the free functions. Attach it to
/// every implementer of the logical-state interfaces.
template <typename ConcreteType>
class LogicalStateContractTrait
    : public mlir::OpTrait::TraitBase<ConcreteType,
                                      LogicalStateContractTrait> {
public:
  static mlir::LogicalResult verifyTrait(mlir::Operation *operation) {
    if (isa<LogicalStateOpInterface>(operation)) {
      if (failed(verifyLogicalStateStructure(operation)))
        return mlir::failure();
    }
    if (isa<LogicalStateUpdateOpInterface>(operation)) {
      if (failed(verifyLogicalStateUpdateStructure(operation)))
        return mlir::failure();
    }
    return mlir::success();
  }
};

/// Verify the fixed structural contract of a LogicalStateOpInterface
/// operation: legacy three operands or current exact policy segments, one
/// result, no regions or successors, and getters aliasing the decoded operands
/// and the declared name/occurrence
/// attributes. Call before consuming any getter of untrusted IR.
mlir::LogicalResult verifyLogicalStateStructure(mlir::Operation *operation);

/// Verify the fixed structural contract of a LogicalStateUpdateOpInterface
/// operation: three operands, no results, regions or successors, and
/// getters aliasing exactly those operand slots.
mlir::LogicalResult
verifyLogicalStateUpdateStructure(mlir::Operation *operation);

} // namespace acir

namespace mlir::OpTrait::acir {
// NativeOpTrait splices its argument behind ::mlir::OpTrait::; alias the
// template so the ODS reference resolves to ::acir::LogicalStateContractTrait.
using ::acir::LogicalStateContractTrait;
} // namespace mlir::OpTrait::acir

#endif // PYCIRCUIT_INTERFACES_LOGICALSTATEINTERFACES_H
