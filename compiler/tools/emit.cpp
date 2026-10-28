#include "Dialect/ACIR/HardwareSourceChecks.h"
// Emit source-owned modules from verified final ACIR. Publication is
// transactional in the Python driver; this transport writes nothing before
// complete success.
#include "Compiler/FinalCppNames.h"
#include "Compiler/FinalCppSourceParts.h"
#include "Compiler/FinalSourceMap.h"
#include "Compiler/FinalVerilogSourceParts.h"
#include "mlir/IR/Verifier.h"
#include "mlir/Parser/Parser.h"
#include "pycircuit/Dialect/ACIR/ACIRDialect.h"
#include "pycircuit/Dialect/ACIR/HardwareAnalysis.h"
#include "pycircuit/Dialect/ACIR/SourceUnitValidation.h"
#include "llvm/ADT/DenseMap.h"
#include "llvm/ADT/DenseSet.h"
#include "llvm/ADT/SmallVector.h"
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
  StringRef target, format = "json", rtlMode = "managed";
  bool optionsValid = argc >= 4 && (argc % 2 == 0);
  for (int i = 2; optionsValid && i + 1 < argc; i += 2) {
    StringRef option(argv[i]);
    if (option == "--target" && target.empty())
      target = argv[i + 1];
    else if (option == "--format")
      format = argv[i + 1];
    else if (option == "--rtl-mode")
      rtlMode = argv[i + 1];
    else
      optionsValid = false;
  }
  if (!optionsValid || (target != "cpp" && target != "verilog") ||
      (format != "json" && format != "source") ||
      (rtlMode != "managed" && rtlMode != "physical") ||
      (target != "verilog" && rtlMode != "managed")) {
    llvm::errs() << "usage: pycircuit-emit FINAL.ac --target cpp|verilog "
                    "[--format json|source] [--rtl-mode managed|physical]\n";
    return 2;
  }
  bool physical = rtlMode == "physical";
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
            : acir::compiler::emitVerilog(*package, analysis, stream, physical);
    if (failed(status))
      return 1;
    llvm::outs() << text;
    return 0;
  }

  auto system = *package->getOps<acir::ac::SystemOp>().begin();
  auto callee = system.getEntry().getAs<FlatSymbolRefAttr>("callee");
  auto root =
      cast<acir::ac::ModuleOp>(analysis.lookupDefinition(callee.getValue()));
  auto logicalEntry = system.getEntry();
  if (auto boundary = system->getAttrOfType<DictionaryAttr>("ac.entry_boundary"))
    logicalEntry = boundary.getAs<DictionaryAttr>("logical_entry");
  auto logicalCallee = logicalEntry.getAs<FlatSymbolRefAttr>("callee");
  llvm::json::Array arguments;
  for (StringRef kind : {StringRef("parameters"), StringRef("type_arguments")})
    if (auto values = logicalEntry.getAs<ArrayAttr>(kind))
      for (Attribute value : values)
        arguments.emplace_back(llvm::json::Object{
            {"kind", kind}, {"value", attributeText(value)}});
  llvm::json::Object result{
      {"entry", llvm::json::Object{{"definition",
                                    "@\"" + logicalCallee.getValue().str() + "\""},
                                   {"arguments", std::move(arguments)}}},
      {"entry_source", sourceOwner(root.getSourceOwner())}};
  auto resetPolicy = acir::ac::getHardwareResetSemantics(*package);
  if (failed(resetPolicy))
    return 1;
  result["reset_semantics"] =
      *resetPolicy == acir::ac::HardwareResetSemantics::IfOne ? "if_one"
                                                              : "strict";
  auto samples = acir::ac::getSystemSamples(root);
  if (failed(samples))
    return 1;
  llvm::json::Array sampleRows;
  bool needsFourState = false;
  for (Attribute raw : *samples) {
    StringRef row = cast<StringAttr>(raw).getValue();
    sampleRows.emplace_back(row);
    needsFourState |= row.contains('x') || row.contains('z');
  }
  // Only direct known, nonzero literals discharge division's X-on-zero
  // behavior. Do not invent a runtime knownness/range proof in this driver.
  auto knownNonzeroLiteral = [](Value value) {
    if (auto constant = value.getDefiningOp<acir::ac::BitsConstantOp>()) {
      auto tree = constant.getValue().getTree();
      auto kind = tree.getAs<StringAttr>("kind");
      auto literal = tree.getAs<DictionaryAttr>("value");
      auto number = literal ? literal.getAs<acir::ac::MathIntAttr>("value")
                            : acir::ac::MathIntAttr();
      return kind && kind.getValue() == "literal" && number &&
             number.getCanonicalValue() != "0";
    }
    if (auto constant = value.getDefiningOp<acir::ac::BitsConstant4Op>()) {
      StringRef pattern = constant.getPattern();
      return !pattern.contains('x') && !pattern.contains('z') &&
             pattern.contains('1');
    }
    return false;
  };
  // Select the simulator from the executable instance closure. Unselected
  // declarations from the same published source unit do not change the mode.
  // Initialized DFF/DFFE and known-preserving arithmetic retain the binary
  // route. Observers and operations/resources with runtime X results require
  // a real four-state engine even without an explicit X/Z constant.
  llvm::DenseSet<Operation *> reached;
  llvm::SmallVector<acir::ac::ModuleOp> pending{root};
  while (!pending.empty()) {
    auto definition = pending.pop_back_val();
    if (!reached.insert(definition).second)
      continue;
    definition.walk([&](Operation *operation) {
      if (auto constant = dyn_cast<acir::ac::BitsConstant4Op>(operation)) {
        StringRef pattern = constant.getPattern();
        needsFourState |= pattern.contains('x') || pattern.contains('z');
      }
      if (operation->hasAttr("ac.verification_observer"))
        needsFourState = true;
      if (auto compare = dyn_cast<acir::ac::BitsCompareOp>(operation))
        needsFourState |= compare.getPredicate() == "case_eq";
      if (auto binary = dyn_cast<acir::ac::BitsBinaryOp>(operation))
        if ((binary.getOpcode() == "udiv" || binary.getOpcode() == "urem") &&
            !knownNonzeroLiteral(binary.getRhs()))
          needsFourState = true;
      if (isa<acir::ac::TableGetOp>(operation))
        needsFourState = true;
      Operation *callee = nullptr;
      if (auto instance = dyn_cast<acir::ac::InstanceOp>(operation))
        callee = analysis.resolveCallee(instance);
      else if (auto collection = dyn_cast<acir::ac::CollectionOp>(operation))
        callee = analysis.resolveCallee(collection);
      if (auto child = dyn_cast_or_null<acir::ac::ModuleOp>(callee))
        pending.push_back(child);
      else if (callee) {
        StringRef primitive = analysis.getPrimitiveKind(callee);
        if (!primitive.empty() && primitive != "dff" && primitive != "dffe")
          needsFourState = true;
      }
    });
  }
  result["system_samples"] = std::move(sampleRows);
  result["needs_four_state"] = needsFourState;
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
    result["source_check_count"] = parts->sourceCheckCount;
    result["root_cpp_name"] = parts->rootCppName;
    auto ports = llvm::json::parse(parts->portMap);
    if (!ports) {
      llvm::consumeError(ports.takeError());
      emitError() << "invalid generated root port map";
      return 1;
    }
    result["port_availability"] = std::move(*ports);
    result["source_groups"] = std::move(groups);
  } else {
    auto parts =
        acir::compiler::emitVerilogSourceParts(*package, analysis, physical);
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
    result["rtl_mode"] = rtlMode;
    result["simulation_top"] = parts->simulationTop;
    result["source_check_count"] = parts->sourceCheckCount;
    result["root_rtl_name"] = parts->rootRtlName;
    auto ports = llvm::json::parse(parts->portMap);
    if (!ports) {
      llvm::consumeError(ports.takeError());
      emitError() << "invalid generated root port map";
      return 1;
    }
    result["port_availability"] = std::move(*ports);
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
