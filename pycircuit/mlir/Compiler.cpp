// Parse and verify the saved program. Export actual SSA/CFG to the graph analysis;
// neither Python source nor a serialized AST is used by the reload path.
#include "Dialect.h"
#include "Components.h"
#include "mlir/Dialect/SCF/IR/SCF.h"
#include "Passes.h"
#include "mlir/Dialect/Arith/IR/Arith.h"
#include "mlir/Dialect/ControlFlow/IR/ControlFlowOps.h"
#include "mlir/Dialect/Func/IR/FuncOps.h"
#include "mlir/IR/BuiltinOps.h"
#include "mlir/IR/Verifier.h"
#include "mlir/Parser/Parser.h"
#include "mlir/Pass/PassManager.h"
#include "mlir/Transforms/Passes.h"
#include "llvm/Support/JSON.h"
#include "llvm/Support/InitLLVM.h"
#include "llvm/Support/raw_ostream.h"
#include <filesystem>
#include <fstream>
#include <set>
#include <sstream>
#include <stdexcept>
using namespace mlir;
namespace {
llvm::json::Value attribute(Attribute a) {
  if (!a) return nullptr;
  if (auto s = dyn_cast<StringAttr>(a)) return s.getValue().str();
  if (auto b = dyn_cast<BoolAttr>(a)) return b.getValue();
  if (auto i = dyn_cast<IntegerAttr>(a)) return i.getInt();
  if (isa<UnitAttr>(a)) return nullptr;
  if (auto array = dyn_cast<ArrayAttr>(a)) {
    llvm::json::Array values;
    for (auto v : array) values.push_back(attribute(v));
    return values;
  }
  if (auto dict = dyn_cast<DictionaryAttr>(a)) {
    llvm::json::Object values;
    for (auto v : dict) values[v.getName().str()] = attribute(v.getValue());
    return values;
  }
  std::string text;
  llvm::raw_string_ostream out(text);
  a.print(out);
  return text;
}
std::string type(Type t) {
  if (auto i = dyn_cast<IntegerType>(t)) return i.getWidth() == 1 ? "bool" : "u" + std::to_string(i.getWidth());
  if (auto q = dyn_cast<acir::QueueType>(t)) return "queue<" + type(q.getElementType()) + ">";
  if (auto q = dyn_cast<acir::SignalType>(t)) return "signal<" + type(q.getElementType()) + ">";
  if (auto q = dyn_cast<acir::EHRType>(t)) return "ehr<" + type(q.getElementType()) + ">";
  if (auto q = dyn_cast<acir::CellType>(t)) return "cell<" + type(q.getElementType()) + ">";
  if (auto q = dyn_cast<acir::QueueArrayType>(t)) return "qarray<" + type(q.getElementType()) + ">";
  if (auto q = dyn_cast<acir::VectorType>(t)) return "vector<" + type(q.getElementType()) + ">";
  if (auto q = dyn_cast<acir::ArrayType>(t)) return "array<" + type(q.getElementType()) + "," + std::to_string(q.getSize()) + ">";
  if (auto q = dyn_cast<acir::StructType>(t)) return q.getName().str();
  if (auto tuple = dyn_cast<TupleType>(t)) {
    std::string result = "tuple<";
    for (auto [index, item] : llvm::enumerate(tuple.getTypes())) result += (index ? "," : "") + type(item);
    return result + ">";
  }
  throw std::runtime_error("unsupported ACIR value type");
}
std::string sourceType(Value v) {
  if (auto *op = v.getDefiningOp())
    if (auto s = op->getAttrOfType<StringAttr>("acir.source_type")) return s.getValue().str();
  return type(v.getType());
}
llvm::json::Object location(Location loc) {
  if (auto file = dyn_cast<FileLineColLoc>(loc))
    return llvm::json::Object{{"file", file.getFilename().str()}, {"line", file.getLine()}, {"column", file.getColumn()}};
  if (auto fused = dyn_cast<FusedLoc>(loc))
    for (auto item : fused.getLocations()) { auto result = location(item); if (!result.empty()) return result; }
  if (auto named = dyn_cast<NameLoc>(loc)) return location(named.getChildLoc());
  if (auto call = dyn_cast<CallSiteLoc>(loc)) {
    auto result = location(call.getCallee());
    return result.empty() ? location(call.getCaller()) : result;
  }
  return {};
}

// Parsing an operation without a location supplies its position in the input
// MLIR. These temporary positions are not Python source provenance.
Location sourceLocation(Location loc, llvm::StringRef input) {
  auto unknown = UnknownLoc::get(loc.getContext());
  if (auto file = dyn_cast<FileLineColRange>(loc)) {
    auto name = file.getFilename().getValue();
    if (name == input || (name.contains("/.analysis-") && name.ends_with("/input.mlir"))) return unknown;
    return loc;
  }
  if (auto named = dyn_cast<NameLoc>(loc)) {
    if (named.getName().getValue().starts_with("acir.type:")) return unknown;
    auto child = sourceLocation(named.getChildLoc(), input);
    return NameLoc::get(named.getName(), child);
  }
  if (auto fused = dyn_cast<FusedLoc>(loc)) {
    SmallVector<Location> sources;
    for (auto item : fused.getLocations()) {
      auto source = sourceLocation(item, input);
      if (!isa<UnknownLoc>(source)) sources.push_back(source);
    }
    return FusedLoc::get(sources, fused.getMetadata(), loc.getContext());
  }
  if (auto call = dyn_cast<CallSiteLoc>(loc)) {
    auto callee = sourceLocation(call.getCallee(), input);
    auto caller = sourceLocation(call.getCaller(), input);
    if (isa<UnknownLoc>(callee)) return caller;
    if (isa<UnknownLoc>(caller)) return callee;
    return CallSiteLoc::get(callee, caller);
  }
  return loc;
}
void cleanLocations(ModuleOp module, llvm::StringRef input) {
  module.walk([&](Operation *op) {
    op->setLoc(sourceLocation(op->getLoc(), input));
    for (auto &region : op->getRegions())
      for (auto &block : region)
        for (auto arg : block.getArguments()) arg.setLoc(sourceLocation(arg.getLoc(), input));
  });
}

// The native debug printer emits UnknownLoc on every generated op/argument.
// Elide only those annotations and their alias definitions. Respect quoted
// attributes and comments so user strings containing "loc(unknown)" survive.
std::string withoutUnknownLocations(llvm::StringRef assembly) {
  SmallVector<llvm::StringRef> lines;
  assembly.split(lines, '\n');
  std::set<std::string> aliases;
  for (auto line : lines) {
    if (line.starts_with("#") && line.ends_with(" = loc(unknown)"))
      aliases.insert(line.take_front(line.find(" = ")).str());
  }
  std::string result;
  std::set<std::string> usedAliases;
  for (auto line : lines) {
    if (line.starts_with("#") && line.ends_with(" = loc(unknown)")) continue;
    bool quoted = false;
    for (size_t i = 0; i < line.size();) {
      if (quoted && line[i] == '\\' && i + 1 < line.size()) {
        result.append(line.substr(i, 2)); i += 2; continue;
      }
      if (line[i] == '"') { quoted = !quoted; result += line[i++]; continue; }
      if (!quoted && line.substr(i).starts_with("//")) { result += line.substr(i).str(); break; }
      if (!quoted && line.substr(i).starts_with(" loc(") &&
          i && llvm::StringRef("=,[(").find(line[i - 1]) == llvm::StringRef::npos) {
        auto end = line.find(')', i + 5);
        if (end != llvm::StringRef::npos) {
          auto name = line.slice(i + 5, end);
          if (name == "unknown" || aliases.count(name.str())) { i = end + 1; continue; }
        }
      }
      if (!quoted && line[i] == '#') {
        auto end = line.find_first_not_of("abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789_$.-", i + 1);
        auto name = line.slice(i, end == llvm::StringRef::npos ? line.size() : end);
        if (aliases.count(name.str())) usedAliases.insert(name.str());
      }
      result += line[i++];
    }
    result += '\n';
  }
  // A location attribute used as ordinary operation metadata is not a suffix
  // annotation: preserve its alias and its semantic attribute value.
  std::string definitions;
  for (auto line : lines)
    if (line.starts_with("#") && line.ends_with(" = loc(unknown)") &&
        usedAliases.count(line.take_front(line.find(" = ")).str()))
      definitions += line.str() + '\n';
  return definitions + result;
}
llvm::json::Object program(ModuleOp module) {
  llvm::json::Object result;
  auto model = attribute(module->getAttr("acir.model"));
  auto contracts = acir::componentContracts();
  for (auto &contract : *model.getAsObject()->getArray("contracts")) contracts.push_back(std::move(contract));
  (*model.getAsObject())["contracts"] = std::move(contracts);
  result["model"] = std::move(model);
  llvm::json::Array resources, functions;
  for (auto &op : module.getBody()->getOperations()) {
    auto name = op.getName().getStringRef();
    if (name != "acir.queue" && name != "acir.queue.array" && name != "acir.cell" && name != "acir.ehr" && name != "acir.signal") continue;
    auto data = attribute(op.getAttrDictionary()); auto &resource = *data.getAsObject();
    auto kind = name.drop_front(5);
    resource.erase("sym_name"); resource.erase("payload_type");
    resource["kind"] = kind == "queue.array" ? "qarray" : kind.str();
    resource["contract"] = kind == "ehr" ? "EHR" : kind == "cell" ? "Cell" : kind == "signal" ? "Signal" : "Queue";
    resource["visibility"] = kind == "ehr" ? "ehr" : kind == "signal" ? "current" : "old";
    if (kind == "queue" || kind == "queue.array") resource["views"] = llvm::json::Array{llvm::json::Object{{"name", "full"}, {"visibility", "ehr"}, {"initial", "old_count == capacity"}, {"commit", "queue.effects"}}};
    resource["loc"] = location(op.getLoc());
    resources.push_back(std::move(data));
  }
  for (auto f : module.getOps<func::FuncOp>()) {
    if (f.isDeclaration()) continue;
    llvm::json::Object fn;
    if (!f->getAttrOfType<DictionaryAttr>("acir.function")) {
      Builder b(module.getContext());
      f->setAttr("acir.function", b.getDictionaryAttr({b.getNamedAttr("name", f.getSymNameAttr()), b.getNamedAttr("kind", b.getStringAttr("helper"))}));
    }
    fn["name"] = f.getSymName().str();
    fn["meta"] = attribute(f->getAttr("acir.function"));
    DenseMap<Value, std::string> ids;
    DenseMap<Block *, std::string> labels;
    unsigned next = 0;
    for (auto [index, block] : llvm::enumerate(f.getBody())) {
      labels[&block] = "b" + std::to_string(index);
      for (auto arg : block.getArguments()) ids[arg] = "a" + std::to_string(next++);
      for (auto &op : block)
        for (auto v : op.getResults()) ids[v] = "v" + std::to_string(next++);
    }
    auto values = [&](ValueRange operands) {
      llvm::json::Array array;
      for (auto v : operands) array.push_back(ids.lookup(v));
      return array;
    };
    llvm::json::Array blocks;
    for (auto &block : f.getBody()) {
      llvm::json::Object b;
      b["label"] = labels.lookup(&block);
      llvm::json::Array params, ops;
      for (auto arg : block.getArguments()) params.push_back(llvm::json::Object{{"id", ids.lookup(arg)}, {"type", sourceType(arg)}});
      b["params"] = std::move(params);
      for (auto &op : block) {
        llvm::json::Object o;
        auto method = acir::componentMethod(&op);
        auto operation = op.getName().getStringRef();
        o["op"] = method.operation ? std::string(method.operation) : operation == "acir.ref" ? "acir.get" : operation == "acir.analysis.invoke" ? "acir.invoke" : operation.str();
        o["args"] = values(op.getOperands());
        llvm::json::Array results;
        for (auto v : op.getResults()) results.push_back(llvm::json::Object{{"id", ids.lookup(v)}, {"type", sourceType(v)}});
        o["results"] = std::move(results);
        auto attributes = attribute(op.getAttrDictionary());
        auto &a = *attributes.getAsObject();
        if (auto ref = dyn_cast<acir::GetOp>(&op)) a["name"] = ref.getTarget().getLeafReference().getValue().str();
        if (auto sources = op.getAttrOfType<ArrayAttr>("acir.consume_locs")) {
          llvm::json::Array locations;
          for (auto source : sources) locations.push_back(location(cast<LocationAttr>(source)));
          a["consumption_locations"] = std::move(locations); a.erase("acir.consume_locs");
        }
        if (method.operation) {
          a["interface"] = method.interface; a["view"] = method.view;
          a["readiness"] = method.readiness;
          if (operation.starts_with("acir.analysis.capacity.")) a["aspect"] = "full";
          if (operation == "acir.queue.empty") a["kind"] = "empty";
          if (operation == "acir.queue.size") a["kind"] = "size";
        }
        o["attrs"] = std::move(attributes);
        o["loc"] = location(op.getLoc());
        if (auto br = dyn_cast<cf::BranchOp>(&op)) {
          o["dest"] = labels.lookup(br.getDest());
          o["dest_args"] = values(br.getDestOperands());
        }
        if (auto br = dyn_cast<cf::CondBranchOp>(&op)) {
          o["yes"] = labels.lookup(br.getTrueDest()); o["no"] = labels.lookup(br.getFalseDest());
          o["yes_args"] = values(br.getTrueDestOperands()); o["no_args"] = values(br.getFalseDestOperands());
        }
        ops.push_back(std::move(o));
      }
      b["ops"] = std::move(ops); blocks.push_back(std::move(b));
    }
    fn["blocks"] = std::move(blocks); functions.push_back(std::move(fn));
  }
  result["resources"] = std::move(resources); result["functions"] = std::move(functions);
  return result;
}
}
int acir::compileMain(int argc, char **argv) {
  llvm::InitLLVM init(argc, argv);
  try {
    if (argc < 3) { llvm::errs() << "usage: acir-compile INPUT.mlir OUTPUT_DIR [--opt]\n"; return 2; }
    if (argc > 4 || (argc == 4 && std::string(argv[3]) != "--opt")) throw std::runtime_error("usage: acir-compile INPUT.mlir OUTPUT_DIR [--opt]");
    DialectRegistry registry;
    registry.insert<acir::ACIRDialect, arith::ArithDialect, cf::ControlFlowDialect, func::FuncDialect, scf::SCFDialect>();
    MLIRContext context(registry); context.loadAllAvailableDialects();
    auto module = parseSourceFile<mlir::ModuleOp>(argv[1], &context);
    if (!module) { llvm::errs() << "invalid or obsolete ACIR; recompile Python sources\n"; return 1; }
    if (failed(verify(*module))) return 1;
    auto meta = (*module)->getAttrOfType<DictionaryAttr>("acir.model");
    if (!meta || !meta.getAs<IntegerAttr>("format_version") || meta.getAs<IntegerAttr>("format_version").getInt() != 7)
      throw std::runtime_error("expected ACIR format_version 7; recompile Python sources");
    if (argc > 3 && std::string(argv[3]) == "--opt") {
      PassManager pm(&context); pm.addPass(createCanonicalizerPass()); pm.addPass(createCSEPass());
      if (failed(pm.run(*module))) return 1;
    }
    cleanLocations(*module, argv[1]);
    auto lowered = cast<mlir::ModuleOp>(module->clone());
    PassManager lower(&context); lower.addPass(createLowerAnalysisPass());
    if (failed(lower.run(lowered)) || failed(verify(lowered))) { lowered.erase(); return 1; }
    std::filesystem::path directory(argv[2]); std::filesystem::create_directories(directory);
    std::error_code ec;
    llvm::raw_fd_ostream json((directory / "program.json").string(), ec);
    if (ec) throw std::runtime_error(ec.message());
    json << llvm::formatv("{0:2}\n", llvm::json::Value(program(lowered)));
    llvm::raw_fd_ostream ir((directory / "model.acir.mlir").string(), ec);
    if (ec) throw std::runtime_error(ec.message());
    std::string assembly;
    llvm::raw_string_ostream debug(assembly);
    module->print(debug, OpPrintingFlags().enableDebugInfo());
    ir << withoutUnknownLocations(assembly);
    llvm::raw_fd_ostream readable((directory / "model.readable.mlir").string(), ec);
    if (ec) throw std::runtime_error(ec.message());
    module->print(readable); readable << '\n';
    lowered.erase();
    return 0;
  } catch (const std::exception &e) { llvm::errs() << "acir-compile: " << e.what() << '\n'; return 1; }
}
