// Private driver transport for verified source-owned target emission.
// It reads only saved final IR and writes one JSON value after full success.
#include "Compiler/FinalCppSourceParts.h"
#include "Compiler/FinalRunnerEmission.h"
#include "Compiler/FinalSourceMap.h"
#include "Compiler/FinalVerilogSourceParts.h"
#include "acir/Dialect/ACIR/ACIRDialect.h"
#include "mlir/Dialect/Arith/IR/Arith.h"
#include "mlir/Dialect/Func/IR/FuncOps.h"
#include "mlir/IR/Verifier.h"
#include "mlir/Parser/Parser.h"
#include "llvm/Support/FormatVariadic.h"
#include "llvm/Support/JSON.h"
#include "llvm/Support/MemoryBuffer.h"
#include "llvm/Support/raw_ostream.h"

int main(int argc, char **argv) {
  const bool verifyMap =
      argc == 3 && llvm::StringRef(argv[1]) == "--verify-source-map";
  const bool targetMode = argc == 4 && llvm::StringRef(argv[2]) == "--target" &&
                          (llvm::StringRef(argv[3]) == "cpp" ||
                           llvm::StringRef(argv[3]) == "verilog");
  const bool withRunner =
      targetMode || (argc == 3 && llvm::StringRef(argv[2]) == "--runner");
  const bool cppTarget = !targetMode || llvm::StringRef(argv[3]) == "cpp";
  const bool rtlTarget = !targetMode || llvm::StringRef(argv[3]) == "verilog";
  if (argc != 2 && !withRunner && !verifyMap) {
    llvm::errs() << "usage: acir-cpp-source-parts-harness FINAL.ac [--runner | "
                    "--target cpp|verilog]\n";
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
  if (verifyMap) {
    auto buffer = llvm::MemoryBuffer::getFile(argv[2]);
    if (!buffer) {
      emitError() << "cannot read source map";
      return 1;
    }
    std::string canonicalPath;
    if (mlir::failed(acir::compiler::validateFinalSourceMapText(
            (*buffer)->getBuffer(), context, emitError, &canonicalPath)))
      return 1;
    llvm::outs() << llvm::formatv("{0}\n", llvm::json::Value(llvm::json::Object{
                                               {"path", canonicalPath}}));
    return 0;
  }
  auto input = mlir::parseSourceFile<mlir::ModuleOp>(argv[1], &context);
  if (!input || mlir::failed(mlir::verify(*input)))
    return 1;
  auto program =
      acir::compiler::buildFinalProgramFromHardware(*input, emitError);
  if (mlir::failed(program))
    return 1;
  llvm::json::Object result;
  llvm::SmallVector<acir::compiler::FinalSourceMembership> memberships;
  if (cppTarget) {
    auto parts = acir::compiler::emitFinalCppSourceParts(*program, emitError);
    if (mlir::failed(parts))
      return 1;

    llvm::json::Array groups;
    for (const auto &group : parts->sourceGroups) {
      if (targetMode) {
        acir::compiler::FinalSourceMembership member{group.sourceOwner,
                                                     {group.headerPath}};
        if (!group.sourcePath.empty())
          member.generatedFiles.push_back(group.sourcePath);
        memberships.push_back(std::move(member));
      }
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
    result["support_header"] = parts->supportHeader;
    result["system_header"] = parts->systemHeader;
    result["source_groups"] = std::move(groups);
  }
  if (withRunner) {
    auto runner = acir::compiler::emitFinalRunnerParts(*program, emitError);
    if (mlir::failed(runner))
      return 1;
    std::string aggregateRtl;
    if (rtlTarget) {
      auto rtlParts =
          acir::compiler::emitFinalVerilogSourceParts(*program, emitError);
      if (mlir::failed(rtlParts))
        return 1;
      if (targetMode) {
        for (mlir::Operation &unit :
             program->hardware().getBody()->getOperations()) {
          if (!unit.hasAttr("ac.unit_kind"))
            continue;
          auto owner =
              unit.getAttrOfType<mlir::DictionaryAttr>("ac.source_owner");
          acir::compiler::FinalSourceMembership member{owner, {}};
          for (const auto &group : rtlParts->sourceGroups)
            if (group.sourceOwner == owner)
              member.generatedFiles.push_back(group.path);
          memberships.push_back(std::move(member));
        }
      }
      llvm::json::Array rtlGroups;
      for (const auto &group : rtlParts->sourceGroups) {
        rtlGroups.emplace_back(llvm::json::Object{
            {"source",
             llvm::json::Object{
                 {"package",
                  group.sourceOwner.getAs<mlir::StringAttr>("package")
                      .getValue()},
                 {"path", group.sourceOwner.getAs<mlir::StringAttr>("path")
                              .getValue()}}},
            {"path", group.path},
            {"module", group.moduleName},
            {"text", group.text}});
      }
      result["rtl_source_groups"] = std::move(rtlGroups);
      result["rtl_core"] = rtlParts->core;
      aggregateRtl = "/* verilator lint_off MULTITOP */\n";
      for (const auto &group : rtlParts->sourceGroups)
        aggregateRtl += group.text;
      aggregateRtl += rtlParts->core;
    }

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
                           {"rtl_hardware", aggregateRtl},
                           {"rtl_bridge", runner->rtlBridge},
                           {"rtl_adapter", runner->rtlAdapter}};
  }
  if (targetMode) {
    auto maps =
        acir::compiler::emitFinalSourceMaps(*program, memberships, emitError);
    if (mlir::failed(maps))
      return 1;
    llvm::json::Array rows;
    for (const auto &map : *maps)
      rows.emplace_back(llvm::json::Object{
          {"source",
           llvm::json::Object{
               {"package",
                map.sourceOwner.getAs<mlir::StringAttr>("package").getValue()},
               {"path",
                map.sourceOwner.getAs<mlir::StringAttr>("path").getValue()}}},
          {"path", map.path},
          {"text", map.text}});
    result["source_maps"] = std::move(rows);
  }
  llvm::outs() << llvm::formatv("{0:2}\n",
                                llvm::json::Value(std::move(result)));
  return 0;
}
