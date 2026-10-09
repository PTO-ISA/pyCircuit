#ifndef PYCIRCUIT_COMPILER_NUMERICLOWERING_H
#define PYCIRCUIT_COMPILER_NUMERICLOWERING_H

#include "Dialect/ACIR/SourceValueSemantics.h"
#include "mlir/IR/Builders.h"
#include "pycircuit/Dialect/ACIR/ACIROps.h"
#include "llvm/ADT/APSInt.h"
#include "llvm/ADT/StringRef.h"

#include <cstdint>
#include <optional>

namespace acir::compiler {

struct IntegerInterval {
  llvm::APSInt lower, upper; // [lower, upper)
};
struct NumericValue {
  mlir::Value value;
  std::optional<ac::detail::ValueKind> sourceKind;
  std::optional<IntegerInterval> interval;
  mlir::Attribute closedSourceConstant{};
};
struct NumericLoweringSite {
  mlir::Location location;
  mlir::DictionaryAttr origin, sourceSpan;
};

// Optional result facts only; this never changes a physical select or its inputs.
std::optional<IntegerInterval> refineUnsignedSelectInterval(
    const NumericLoweringSite &, llvm::StringRef predicate, bool selectedWhenTrue,
    const std::optional<IntegerInterval> &original, uint64_t literalWidth,
    const llvm::APSInt &comparisonConstant, const llvm::APSInt &fallbackConstant);

mlir::FailureOr<NumericValue>
lowerExactIntegerBinary(mlir::OpBuilder &, const NumericLoweringSite &,
                        ac::detail::ValueOpcode, const NumericValue &,
                        const NumericValue &);
mlir::FailureOr<NumericValue>
lowerExactIntegerSelect(mlir::OpBuilder &, const NumericLoweringSite &,
                        const NumericValue &condition,
                        const NumericValue &whenTrue,
                        const NumericValue &whenFalse);
mlir::FailureOr<NumericValue>
lowerExactIntegerShiftRight(mlir::OpBuilder &, const NumericLoweringSite &,
                            const NumericValue &, const llvm::APSInt &count);
mlir::FailureOr<mlir::Value>
lowerExactIntegerBoundary(mlir::OpBuilder &, const NumericLoweringSite &,
                          const NumericValue &, ac::BitsType destination);

} // namespace acir::compiler

#endif // PYCIRCUIT_COMPILER_NUMERICLOWERING_H
