#ifndef PYCIRCUIT_TRANSFORMS_EXPANDSTATICFOR_H
#define PYCIRCUIT_TRANSFORMS_EXPANDSTATICFOR_H

#include "pycircuit/Dialect/ACIR/HardwareAnalysis.h"
#include "pycircuit/Dialect/ACIR/TransactionalAnalysis.h"

namespace acir {
/// Expand static source loops in an occurrence-private working body. Failure
/// leaves that working body unpublished; callers retain the source template.
mlir::LogicalResult expandStaticFor(mlir::Operation *scope,
                                    ac::HardwareAnalysis &hardware,
                                    const ac::HardwareBindings &bindings,
                                    ac::TransactionalDebit debit = {});
} // namespace acir
#endif
