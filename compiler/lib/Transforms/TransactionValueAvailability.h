#ifndef PYCIRCUIT_TRANSACTION_VALUE_AVAILABILITY_H
#define PYCIRCUIT_TRANSACTION_VALUE_AVAILABILITY_H
#include "LowerTransactions.h"
namespace acir::detail {
mlir::LogicalResult materializeTransactionValueAvailability(
    const PreparedTransactionLowering &prepared,
    MaterializedTransactionLowering &candidate,
    ac::detail::SourceCheckBudget &budget);
}
#endif
