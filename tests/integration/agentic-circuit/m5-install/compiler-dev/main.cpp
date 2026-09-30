#include "acir/Dialect/ACIR/ACIRDialect.h"
#include "Compiler/FinalCppSourceParts.h"
#include "Compiler/SourceUnit.h"
#include "mlir/IR/Diagnostics.h"
#include "mlir/IR/DialectRegistry.h"
#include "mlir/IR/MLIRContext.h"

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
      headers, diagnostic);

  acir::compiler::SourceUnitArtifacts unitArtifacts;
  acir::compiler::FinalCppSourceParts cppParts;
  return dialect == nullptr || mlir::failed(sourceHeaders) ||
                 !cppParts.sourceGroups.empty() || unitArtifacts.body
             ? 1
             : 0;
}
