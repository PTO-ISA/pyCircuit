#ifndef PYCIRCUIT_ACIR_SYSTEMSAMPLING_H
#define PYCIRCUIT_ACIR_SYSTEMSAMPLING_H
#include "mlir/IR/Operation.h"
#include "mlir/Support/LogicalResult.h"
#include "llvm/ADT/SmallVector.h"
namespace acir::ac {
// Native grammar only; the analysis owner checks the closed port boundary and widths.
mlir::FailureOr<llvm::SmallVector<unsigned>> readSystemSampleInputs(mlir::Operation *declaration);
}
#endif
