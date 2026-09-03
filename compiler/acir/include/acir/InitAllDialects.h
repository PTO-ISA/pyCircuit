#ifndef ACIR_INITALLDIALECTS_H
#define ACIR_INITALLDIALECTS_H

#include "acir/Dialect/ACIR/ACIRDialect.h"
#include "acir/Dialect/ACSim/ACSimDialect.h"
#include "mlir/Dialect/Arith/IR/Arith.h"
#include "mlir/Dialect/ControlFlow/IR/ControlFlow.h"
#include "mlir/Dialect/DLTI/DLTI.h"
#include "mlir/Dialect/Func/IR/FuncOps.h"
#include "mlir/Dialect/Index/IR/IndexDialect.h"
#include "mlir/Dialect/SCF/IR/SCF.h"
#include "mlir/IR/BuiltinDialect.h"
#include "mlir/IR/DialectRegistry.h"

namespace acir {

inline void registerAllDialects(mlir::DialectRegistry &registry) {
  registry.insert<ac::ACIRDialect, acsim::ACSimDialect, mlir::BuiltinDialect,
                  mlir::DLTIDialect, mlir::arith::ArithDialect,
                  mlir::func::FuncDialect, mlir::index::IndexDialect,
                  mlir::scf::SCFDialect, mlir::cf::ControlFlowDialect>();
}

} // namespace acir

#endif // ACIR_INITALLDIALECTS_H
