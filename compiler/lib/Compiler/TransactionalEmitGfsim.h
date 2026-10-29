#pragma once
#include "pycircuit/Dialect/ACIR/TransactionalAnalysis.h"
#include "llvm/Support/raw_ostream.h"
namespace acir::compiler {
mlir::LogicalResult emitTransactionalGfsim(const ac::TransactionalAnalysis &,
                                           llvm::raw_ostream &);
void emitTransactionalGfsimMain(llvm::raw_ostream &, mlir::ArrayAttr samples,
                                bool resetIfOne);
} // namespace acir::compiler
