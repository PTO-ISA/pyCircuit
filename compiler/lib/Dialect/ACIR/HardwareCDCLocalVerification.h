#ifndef PYCIRCUIT_HARDWARE_CDC_LOCAL_VERIFICATION_H
#define PYCIRCUIT_HARDWARE_CDC_LOCAL_VERIFICATION_H
#include "mlir/IR/Operation.h"
#include "mlir/Support/LogicalResult.h"
namespace acir::ac {
mlir::LogicalResult verifyCDCRefinementContract(mlir::Operation *witness);
}
#endif
