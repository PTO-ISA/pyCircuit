#ifndef PYCIRCUIT_LIB_IR_STATEDECLARATIONCONTRACTS_H
#define PYCIRCUIT_LIB_IR_STATEDECLARATIONCONTRACTS_H

#include "mlir/IR/BuiltinAttributes.h"
#include "mlir/IR/Operation.h"

namespace acir::ac::detail {
// Strip origin/location provenance recursively; the single semantic
// equivalence implementation behind areEquivalentHardwareTypes.
mlir::Attribute semanticAttr(mlir::Attribute attribute);
} // namespace acir::ac::detail

namespace acir::ac {
/// Provenance-aware hardware payload equivalence (declaration moved here from
/// HardwareAnalysis.h so common-IR and GFIR checks can use it without the
/// staged analysis layer; HardwareAnalysis.h keeps a matching declaration).
bool areEquivalentHardwareTypes(mlir::Type lhs, mlir::Type rhs);

// Named direct module placement with a valid instance identifier and a
// well-formed occurrence whose origin names the owning module.
mlir::LogicalResult verifyNamedLogicalDeclaration(mlir::Operation *declaration,
                                                  mlir::StringAttr instanceName);

// Direct ac.module graph placement inside a routed package carrying the
// current routed contract marker. This is the structural placement header
// of routed logical-state operations, not stage admission.
mlir::LogicalResult
verifyRoutedLogicalStatePlacement(mlir::Operation *operation);

// Bits control carrier whose width, when a literal integer is directly
// available without binding evaluation, is exactly one. Symbolic widths
// defer to the owning stage's bound check.
mlir::LogicalResult verifyLiteralControlWidth(mlir::Operation *site,
                                              mlir::Value control,
                                              llvm::StringRef description);

// Bits payload carrier whose width, when a literal integer is directly
// available without binding evaluation, resolves as a positive u64 natural
// width. Any literal width is accepted (design review r3 F6: the
// transactional resource contract fixes no carrier width, unlike the ==1
// control check); symbolic widths defer to the staged width closure.
mlir::LogicalResult verifyLiteralWidthResolves(mlir::Operation *site,
                                               mlir::Type payload);

// Structural dictionary shape of an ac.payload_constraint attribute: closed
// field set, hardware kind, payload type agreement and known source kind.
// Semantic width, scope and symbolic-template obligations stay with the
// bound stage verifier.
mlir::LogicalResult verifyPayloadConstraintShape(mlir::Operation *site,
                                                 mlir::Attribute raw,
                                                 mlir::Type payload);
} // namespace acir::ac


#endif // PYCIRCUIT_LIB_IR_STATEDECLARATIONCONTRACTS_H
