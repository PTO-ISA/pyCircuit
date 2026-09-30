// Non-installed test transport for the private executable-source C++ emitter.
// It reads only saved final IR and writes one JSON value after full success.
#include "Compiler/FinalCppSourceParts.h"
#include "Compiler/FinalRunnerEmission.h"
#include "acir/Dialect/ACIR/ACIRDialect.h"
#include "mlir/Dialect/Arith/IR/Arith.h"
#include "mlir/Dialect/Func/IR/FuncOps.h"
#include "mlir/IR/Verifier.h"
#include "mlir/Parser/Parser.h"
#include "llvm/Support/FormatVariadic.h"
#include "llvm/Support/JSON.h"
#include "llvm/Support/raw_ostream.h"

int main(int argc, char **argv) {
  const bool withRunner = argc == 3 && llvm::StringRef(argv[2]) == "--runner";
  if (argc != 2 && !withRunner) {
    llvm::errs()
        << "usage: acir-cpp-source-parts-harness FINAL.ac [--runner]\n";
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
        {"source_path", group.sourcePath.empty()
                            ? llvm::json::Value(nullptr)
                            : llvm::json::Value(group.sourcePath)},
        {"header", group.header},
        {"implementation", group.sourcePath.empty()
                               ? llvm::json::Value(nullptr)
                               : llvm::json::Value(group.source)}});
  }
  llvm::json::Object result{{"support_header", parts->supportHeader},
                            {"system_header", parts->systemHeader},
                            {"source_groups", std::move(groups)}};
  if (withRunner) {
    auto runner = acir::compiler::emitFinalRunnerParts(*program, emitError);
    auto rtl = acir::compiler::emitFinalVerilogParts(*program, emitError);
    if (mlir::failed(runner) || mlir::failed(rtl))
      return 1;
    const auto *root = program->modules().root;
    auto owner =
        root->module->getAttrOfType<mlir::DictionaryAttr>("ac.source_owner");
    result["entry"] = llvm::json::Object{
        {"definition", "@\"" + root->definition.getValue().str() + "\""},
        {"arguments", llvm::json::Array{}}};
    result["entry_source"] = llvm::json::Object{
        {"package", owner.getAs<mlir::StringAttr>("package").getValue()},
        {"path", owner.getAs<mlir::StringAttr>("path").getValue()}};
    result["runner"] =
        llvm::json::Object{{"metadata_header", runner->metadataHeader},
                           {"main_source", runner->mainSource},
                           {"abi_header", runner->abiHeader},
                           {"abi_source", runner->abiSource},
                           {"rtl_hardware", rtl->rtl},
                           {"rtl_bridge", runner->rtlBridge},
                           {"rtl_adapter", runner->rtlAdapter}};
  }
  llvm::outs() << llvm::formatv("{0:2}\n",
                                llvm::json::Value(std::move(result)));
  return 0;
}
