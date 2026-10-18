#ifndef ACIR_DIALECT_ACIR_ACIRDIALECT_H
#define ACIR_DIALECT_ACIR_ACIRDIALECT_H

#include "mlir/IR/Dialect.h"
#include "llvm/ADT/DenseSet.h"
#include <mutex>

namespace acir::ac::detail {
struct OccurrenceGrammarMemoAccess;
}

#include "pycircuit/Dialect/ACIR/ACIROps.h"
#include "pycircuit/Dialect/ACIR/ACIRTypes.h"

#include "pycircuit/Dialect/ACIR/ACIRDialect.h.inc"

#endif // ACIR_DIALECT_ACIR_ACIRDIALECT_H
