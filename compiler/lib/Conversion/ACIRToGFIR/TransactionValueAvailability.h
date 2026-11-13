#ifndef PYCIRCUIT_TRANSACTION_VALUE_AVAILABILITY_H
#define PYCIRCUIT_TRANSACTION_VALUE_AVAILABILITY_H
#include "pycircuit/Conversion/ACIRToGFIR.h"
namespace acir::detail {
mlir::LogicalResult materializeTransactionValueAvailability(
    const PreparedTransactionLowering &prepared,
    MaterializedTransactionLowering &candidate,
    ac::detail::SourceCheckBudget &budget);
}
#endif
