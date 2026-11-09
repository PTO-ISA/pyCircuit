#include "Compiler/SourceUnit.h"
#include "Compiler/StructuralCppBackend.h"
#include "mlir/IR/Diagnostics.h"
#include "mlir/IR/DialectRegistry.h"
#include "mlir/IR/MLIRContext.h"
#include "pycircuit/Dialect/ACIR/ACIRDialect.h"

int main() {
  mlir::DialectRegistry registry;
  registry.insert<acir::ac::ACIRDialect>();
  mlir::MLIRContext context(registry);
  auto *dialect = context.getOrLoadDialect<acir::ac::ACIRDialect>();

  // Exercise an exported CompilerDev symbol so the consumer proves that the
  // source compiler library and its public header closure link together.
  auto diagnostic = [&]() {
    return mlir::emitError(mlir::UnknownLoc::get(&context));
  };
  llvm::SmallVector<mlir::ModuleOp> headers;
  auto sourceHeaders = acir::compiler::SourceHeaderRegistry::create(
      &context, headers, diagnostic);

  acir::compiler::SourceUnitArtifacts unitArtifacts;
  using StructuralEmitter = mlir::LogicalResult (*)(
      mlir::ModuleOp, llvm::raw_ostream &);
  StructuralEmitter structuralEmitter = &acir::compiler::emitStructuralCpp;
  return dialect == nullptr || mlir::failed(sourceHeaders) ||
                 structuralEmitter == nullptr || unitArtifacts.body
             ? 1
             : 0;
}
