#include "pycircuit/Dialect/ACIR/ACIRDialect.h"
#include "pycircuit/Transforms/Passes.h"
#include "Compiler/SourceRuleWrites.h"
#include "Compiler/SourceMemoryBindings.h"

#include "mlir/Dialect/Arith/IR/Arith.h"
#include "mlir/Dialect/Func/IR/FuncOps.h"
#include "mlir/Tools/mlir-opt/MlirOptMain.h"
#include "mlir/Transforms/Passes.h"

int main(int argc, char **argv) {
  mlir::DialectRegistry dialects;
  dialects.insert<acir::ac::ACIRDialect, mlir::arith::ArithDialect,
                  mlir::func::FuncDialect>();
  acir::registerACIRPasses();
  acir::compiler::detail::registerSourceCompilerPasses();
  acir::compiler::detail::registerSourceBindingsPass();
  mlir::registerCanonicalizerPass();
  mlir::registerCSEPass();
  return mlir::asMainReturnCode(
      mlir::MlirOptMain(argc, argv, "pycircuit IR optimizer\n", dialects));
}
