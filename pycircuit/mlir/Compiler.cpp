// Parse and verify the saved program. Export actual SSA/CFG to the graph analysis;
// neither Python source nor a serialized AST is used by the reload path.
#include "Dialect.h"
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
Attribute jsonAttribute(MLIRContext *ctx, const llvm::json::Value &value) {
  if (auto b = value.getAsBoolean()) return BoolAttr::get(ctx, *b);
  if (auto n = value.getAsInteger()) return IntegerAttr::get(IntegerType::get(ctx, 64), *n);
  if (auto s = value.getAsString()) return StringAttr::get(ctx, *s);
  if (auto array = value.getAsArray()) {
    SmallVector<Attribute> items;
    for (const auto &item : *array) items.push_back(jsonAttribute(ctx, item));
    return ArrayAttr::get(ctx, items);
  }
  if (auto object = value.getAsObject()) {
    SmallVector<NamedAttribute> items;
    for (const auto &item : *object) items.emplace_back(StringAttr::get(ctx, llvm::StringRef(item.first)), jsonAttribute(ctx, item.second));
    return DictionaryAttr::get(ctx, items);
  }
  return UnitAttr::get(ctx);
}
std::string type(Type t) {
  if (auto i = dyn_cast<IntegerType>(t)) return i.getWidth() == 1 ? "bool" : "u" + std::to_string(i.getWidth());
  if (auto q = dyn_cast<acir::QueueType>(t)) return "queue<" + type(q.getElementType()) + ">";
  if (auto q = dyn_cast<acir::SignalType>(t)) return "signal<" + type(q.getElementType()) + ">";
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
  result["model"] = attribute(module->getAttr("acir.model"));
  llvm::json::Array resources, functions;
  for (auto r : module.getOps<acir::ResourceOp>()) resources.push_back(attribute(r->getAttrDictionary()));
  for (auto f : module.getOps<func::FuncOp>()) {
    if (f.isDeclaration()) continue;
    llvm::json::Object fn;
    if (!f->getAttrOfType<DictionaryAttr>("acir.function"))
      throw std::runtime_error("missing acir.function metadata: " + f.getSymName().str());
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
        o["op"] = op.getName().getStringRef().str();
        o["args"] = values(op.getOperands());
        llvm::json::Array results;
        for (auto v : op.getResults()) results.push_back(llvm::json::Object{{"id", ids.lookup(v)}, {"type", sourceType(v)}});
        o["results"] = std::move(results);
        o["attrs"] = attribute(op.getAttrDictionary());
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
    if (argc < 3) { llvm::errs() << "usage: acir-compile INPUT.mlir OUTPUT_DIR [--opt | --bindings FILE]\n"; return 2; }
    DialectRegistry registry;
    registry.insert<acir::ACIRDialect, arith::ArithDialect, cf::ControlFlowDialect, func::FuncDialect>();
    MLIRContext context(registry); context.loadAllAvailableDialects();
    auto module = parseSourceFile<ModuleOp>(argv[1], &context);
    if (!module || failed(verify(*module))) return 1;
    auto meta = (*module)->getAttrOfType<DictionaryAttr>("acir.model");
    if (!meta || !meta.getAs<IntegerAttr>("format_version") || meta.getAs<IntegerAttr>("format_version").getInt() != 6)
      throw std::runtime_error("expected ACIR format_version 6; recompile Python sources (older formats used removed Rule-var semantics)");
    // Legacy v6 files may contain full analysis caches. Rebuild from the model
    // declarations and actual SSA/CFG instead of carrying those caches forward.
    (*module)->removeAttr("acir.analysis");
    (*module)->removeAttr("acir.contracts");
    (*module)->removeAttr("acir.ehr");
    module->walk([](Operation *op) { op->removeAttr("port"); op->removeAttr("ports"); });
    if (argc > 3 && std::string(argv[3]) == "--opt") {
      PassManager pm(&context); pm.addPass(createCanonicalizerPass()); pm.addPass(createCSEPass());
      if (failed(pm.run(*module))) return 1;
    }
    cleanLocations(*module, argv[1]);
    if (argc > 4 && std::string(argv[3]) == "--bindings") {
      std::ifstream input(argv[4]);
      std::stringstream text; text << input.rdbuf();
      auto parsed = llvm::json::parse(text.str());
      if (!parsed || !parsed->getAsObject()) throw std::runtime_error("invalid EHR bindings JSON");
      auto root = parsed->getAsObject();
      if (!root->getArray("views")) throw std::runtime_error("missing EHR views");
      (*module)->setAttr("acir.ehr", jsonAttribute(&context, *root->get("views")));
      auto functions = root->getObject("functions");
      if (!functions) throw std::runtime_error("missing EHR function bindings");
      for (auto fn : module->getOps<func::FuncOp>()) {
        auto bindings = functions->getObject(fn.getSymName());
        if (!bindings) continue;
        fn.walk([&](Operation *op) {
          auto id = op->getAttrOfType<StringAttr>("acir.id");
          if (!id) return;
          if (auto ports = bindings->getArray(id.getValue())) {
            op->setAttr("ports", jsonAttribute(&context, *bindings->get(id.getValue())));
            if (ports->size() == 1)
              op->setAttr("port", jsonAttribute(&context, *ports->front().getAsObject()->get("port")));
          }
        });
      }
    }
    std::filesystem::path directory(argv[2]); std::filesystem::create_directories(directory);
    std::error_code ec;
    llvm::raw_fd_ostream json((directory / "program.json").string(), ec);
    if (ec) throw std::runtime_error(ec.message());
    json << llvm::formatv("{0:2}\n", llvm::json::Value(program(*module)));
    llvm::raw_fd_ostream ir((directory / "model.acir.mlir").string(), ec);
    if (ec) throw std::runtime_error(ec.message());
    std::string assembly;
    llvm::raw_string_ostream debug(assembly);
    module->print(debug, OpPrintingFlags().enableDebugInfo());
    ir << withoutUnknownLocations(assembly);
    llvm::raw_fd_ostream readable((directory / "model.readable.mlir").string(), ec);
    if (ec) throw std::runtime_error(ec.message());
    module->print(readable); readable << '\n';
    return 0;
  } catch (const std::exception &e) { llvm::errs() << "acir-compile: " << e.what() << '\n'; return 1; }
}
