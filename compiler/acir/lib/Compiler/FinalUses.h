#ifndef ACIR_LIB_COMPILER_FINALUSES_H
#define ACIR_LIB_COMPILER_FINALUSES_H
#include "acir/Dialect/ACIR/ACIROps.h"
#include "llvm/ADT/DenseMap.h"
namespace acir::compiler {
mlir::LogicalResult
retainGenericSourceUses(ac::RuleOp rule,
                        llvm::DenseMap<mlir::Value, mlir::Value> &replacements);
}
#endif
