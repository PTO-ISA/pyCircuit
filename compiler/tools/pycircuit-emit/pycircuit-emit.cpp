// Emit source-owned modules from verified final ACIR. Publication is
// transactional in the Python driver; this transport writes nothing before
// complete success.
#include "Compiler/FinalCppSourceParts.h"
#include "Compiler/FinalSourceMap.h"
#include "Compiler/FinalVerilogSourceParts.h"
#include "mlir/IR/Verifier.h"
#include "mlir/Parser/Parser.h"
#include "pycircuit/Dialect/ACIR/ACIRDialect.h"
#include "pycircuit/Dialect/ACIR/HardwareAnalysis.h"
#include "pycircuit/Dialect/ACIR/SourceUnitValidation.h"
#include "llvm/ADT/DenseMap.h"
#include "llvm/Support/FormatVariadic.h"
#include "llvm/Support/JSON.h"
#include "llvm/Support/MemoryBuffer.h"
#include "llvm/Support/raw_ostream.h"

using namespace mlir;

namespace {
llvm::json::Object sourceOwner(DictionaryAttr owner) {
  return llvm::json::Object{
      {"package", owner.getAs<StringAttr>("package").getValue()},
      {"path", owner.getAs<StringAttr>("path").getValue()}};
}
std::string attributeText(Attribute attr) {
  std::string text;
  llvm::raw_string_ostream stream(text);
  attr.print(stream);
  return text;
}
} // namespace

int main(int argc, char **argv) {
  DialectRegistry dialects;
  dialects.insert<acir::ac::ACIRDialect>();
  MLIRContext context(dialects);
  context.loadAllAvailableDialects();
  auto emitError = [&] { return mlir::emitError(UnknownLoc::get(&context)); };
  if (argc == 3 && llvm::StringRef(argv[1]) == "--verify-source-map") {
    auto buffer = llvm::MemoryBuffer::getFile(argv[2]);
    if (!buffer) {
      emitError() << "cannot read source map";
      return 1;
    }
    std::string path;
    if (failed(acir::compiler::validateFinalSourceMapText(
            (*buffer)->getBuffer(), context, emitError, &path)))
      return 1;
    llvm::outs() << llvm::formatv(
        "{0}\n", llvm::json::Value(llvm::json::Object{{"path", path}}));
    return 0;
  }
  StringRef target, format = "json";
  if (argc == 4 || argc == 6) {
    if (StringRef(argv[2]) == "--target")
      target = argv[3];
    if (argc == 6 && StringRef(argv[4]) == "--format")
      format = argv[5];
    else if (argc == 6)
      format = "";
  }
  if ((target != "cpp" && target != "verilog") ||
      (format != "json" && format != "source")) {
    llvm::errs() << "usage: pycircuit-emit FINAL.ac --target cpp|verilog "
                    "[--format json|source]\n";
    return 2;
  }
  auto package = parseSourceFile<ModuleOp>(argv[1], &context);
  if (!package || failed(mlir::verify(*package)))
    return 1;
  acir::ac::HardwareAnalysis analysis(*package);
  if (failed(analysis.verify()))
    return 1;
  if (format == "source") {
    std::string text;
    llvm::raw_string_ostream stream(text);
    LogicalResult status =
        target == "cpp"
            ? acir::compiler::emitCpp(*package, analysis, stream)
            : acir::compiler::emitVerilog(*package, analysis, stream);
    if (failed(status))
      return 1;
    llvm::outs() << text;
    return 0;
  }

  auto system = *package->getOps<acir::ac::SystemOp>().begin();
  auto callee = system.getEntry().getAs<FlatSymbolRefAttr>("callee");
  auto root =
      cast<acir::ac::ModuleOp>(analysis.lookupDefinition(callee.getValue()));
  llvm::json::Array arguments;
  for (StringRef kind : {StringRef("parameters"), StringRef("type_arguments")})
    if (auto values = system.getEntry().getAs<ArrayAttr>(kind))
      for (Attribute value : values)
        arguments.emplace_back(llvm::json::Object{
            {"kind", kind}, {"value", attributeText(value)}});
  llvm::json::Object result{
      {"entry", llvm::json::Object{{"definition",
                                    "@\"" + callee.getValue().str() + "\""},
                                   {"arguments", std::move(arguments)}}},
      {"entry_source", sourceOwner(root.getSourceOwner())}};
  auto units = acir::ac::collectFinalSourceUnits(*package, emitError);
  if (failed(units))
    return 1;
  llvm::SmallVector<acir::compiler::FinalSourceMembership> memberships;
  llvm::DenseMap<Attribute, unsigned> membershipIndices;
  for (const acir::ac::FinalSourceUnitView &unit : *units) {
    membershipIndices.try_emplace(unit.owner, memberships.size());
    memberships.push_back({unit.owner, {}});
  }
  if (target == "cpp") {
    auto parts = acir::compiler::emitCppSourceParts(*package, analysis);
    if (failed(parts))
      return 1;
    llvm::json::Array groups;
    for (const auto &group : parts->sourceGroups) {
      auto found = membershipIndices.find(group.sourceOwner);
      if (found == membershipIndices.end()) {
        emitError() << "C++ source group has no final SourceOwner";
        return 1;
      }
      auto &member = memberships[found->second];
      member.generatedFiles.push_back(group.headerPath);
      if (!group.sourcePath.empty())
        member.generatedFiles.push_back(group.sourcePath);
      groups.emplace_back(llvm::json::Object{
          {"source", sourceOwner(group.sourceOwner)},
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
    result["simulation_main"] = parts->simulationMain;
    result["simulation_config"] = parts->simulationConfig;
    result["root_cpp_name"] = parts->rootCppName;
    result["source_groups"] = std::move(groups);
  } else {
    auto parts = acir::compiler::emitVerilogSourceParts(*package, analysis);
    if (failed(parts))
      return 1;
    llvm::json::Array groups, standard;
    for (const auto &group : parts->sourceGroups) {
      auto found = membershipIndices.find(group.sourceOwner);
      if (found == membershipIndices.end()) {
        emitError() << "RTL source group has no final SourceOwner";
        return 1;
      }
      memberships[found->second].generatedFiles.push_back(group.path);
      groups.emplace_back(
          llvm::json::Object{{"source", sourceOwner(group.sourceOwner)},
                             {"path", group.path},
                             {"text", group.text}});
    }
    for (const auto &source : parts->standardSources)
      standard.emplace_back(source);
    result["rtl_source_groups"] = std::move(groups);
    result["rtl_standard_sources"] = std::move(standard);
    result["rtl_core"] = parts->core;
    result["simulation_top"] = parts->simulationTop;
    result["root_rtl_name"] = parts->rootRtlName;
  }
  auto maps =
      acir::compiler::emitFinalSourceMaps(*package, memberships, emitError);
  if (failed(maps))
    return 1;
  llvm::json::Array rows;
  for (const auto &map : *maps)
    rows.emplace_back(
        llvm::json::Object{{"source", sourceOwner(map.sourceOwner)},
                           {"path", map.path},
                           {"text", map.text}});
  result["source_maps"] = std::move(rows);
  llvm::outs() << llvm::formatv("{0:2}\n",
                                llvm::json::Value(std::move(result)));
  return 0;
}
