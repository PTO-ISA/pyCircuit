// Non-installed test transport for the private executable-source C++ emitter.
// It reads only saved final IR and writes one JSON value after full success.
#include "Compiler/FinalCppSourceParts.h"
#include "acir/Dialect/ACIR/ACIRDialect.h"
#include "mlir/Dialect/Arith/IR/Arith.h"
#include "mlir/Dialect/Func/IR/FuncOps.h"
#include "mlir/IR/Verifier.h"
#include "mlir/Parser/Parser.h"
#include "llvm/Support/FormatVariadic.h"
#include "llvm/Support/JSON.h"
#include "llvm/Support/raw_ostream.h"

int main(int argc, char **argv) {
  if (argc != 2) {
    llvm::errs() << "usage: acir-cpp-source-parts-harness FINAL.ac\n";
    return 2;
  }
  mlir::DialectRegistry dialects;
  dialects.insert<acir::ac::ACIRDialect, mlir::arith::ArithDialect,
                  mlir::func::FuncDialect>();
  mlir::MLIRContext context(dialects);
  context.loadAllAvailableDialects();
  auto emitError = [&] {
    return mlir::emitError(mlir::UnknownLoc::get(&context));
  };
  auto input = mlir::parseSourceFile<mlir::ModuleOp>(argv[1], &context);
  if (!input || mlir::failed(mlir::verify(*input)))
    return 1;
  auto program =
      acir::compiler::buildFinalProgramFromHardware(*input, emitError);
  if (mlir::failed(program))
    return 1;
  auto parts = acir::compiler::emitFinalCppSourceParts(*program, emitError);
  if (mlir::failed(parts))
    return 1;

  llvm::json::Array groups;
  for (const auto &group : parts->sourceGroups) {
    auto package = group.sourceOwner
                       ? group.sourceOwner.getAs<mlir::StringAttr>("package")
                       : mlir::StringAttr();
    auto path = group.sourceOwner
                    ? group.sourceOwner.getAs<mlir::StringAttr>("path")
                    : mlir::StringAttr();
    if (!package || !path) {
      emitError() << "C++ source group lacks a validated SourceOwner";
      return 1;
    }
    groups.emplace_back(llvm::json::Object{
        {"source", llvm::json::Object{{"package", package.getValue()},
                                      {"path", path.getValue()}}},
        {"header_path", group.headerPath},
        {"source_path", group.sourcePath},
        {"header", group.header},
        {"implementation", group.source}});
  }
  llvm::json::Object result{{"support_header", parts->supportHeader},
                            {"system_header", parts->systemHeader},
                            {"source_groups", std::move(groups)}};
  llvm::outs() << llvm::formatv("{0:2}\n",
                                llvm::json::Value(std::move(result)));
  return 0;
}
