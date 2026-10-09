#ifndef PYCIRCUIT_COMPILER_FIXEDUNSIGNEDDIVREMLOWERING_H
#define PYCIRCUIT_COMPILER_FIXEDUNSIGNEDDIVREMLOWERING_H

#include "NumericLowering.h"
#include "pycircuit/Dialect/ACIR/HardwareAnalysis.h"

namespace acir::compiler {

enum class FixedUnsignedDivRemResult { Quotient, Remainder };

mlir::FailureOr<mlir::Value> lowerFixedUnsignedDivRem(
    mlir::OpBuilder &, ac::HardwareAnalysis &, const NumericLoweringSite &,
    const NumericValue &numerator, bool authoritativeUnsigned,
    const llvm::APSInt &divisor, FixedUnsignedDivRemResult);

} // namespace acir::compiler

#endif // PYCIRCUIT_COMPILER_FIXEDUNSIGNEDDIVREMLOWERING_H
