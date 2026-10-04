#include "Dialect.h"
#include "Passes.h"
#include "mlir/Dialect/Arith/IR/Arith.h"
#include "mlir/Dialect/ControlFlow/IR/ControlFlowOps.h"
#include "mlir/Dialect/EmitC/IR/EmitC.h"
#include "mlir/Dialect/Func/IR/FuncOps.h"
#include "mlir/Tools/mlir-opt/MlirOptMain.h"
#include "mlir/Transforms/Passes.h"
int main(int argc, char **argv) {
  mlir::DialectRegistry registry;
  registry.insert<acir::ACIRDialect, mlir::arith::ArithDialect,
                  mlir::cf::ControlFlowDialect, mlir::emitc::EmitCDialect,
                  mlir::func::FuncDialect>();
  acir::registerPasses();
  mlir::registerTransformsPasses();
  return mlir::asMainReturnCode(
      mlir::MlirOptMain(argc, argv, "ACPy circuit optimizer\n", registry));
}
