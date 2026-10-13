// Standalone source/transaction/RTL MLIR compiler and cycle executor.
#include "Dialect.h"
#include "Execution.h"
#include "Passes.h"
#include "mlir/Dialect/Arith/IR/Arith.h"
#include "mlir/Dialect/ControlFlow/IR/ControlFlowOps.h"
#include "mlir/Dialect/Func/IR/FuncOps.h"
#include "mlir/Dialect/SCF/IR/SCF.h"
#include "mlir/IR/Builders.h"
#include "mlir/IR/Verifier.h"
#include "mlir/Parser/Parser.h"
#include "mlir/Pass/PassManager.h"
#include "mlir/Transforms/Passes.h"
#include "llvm/Support/InitLLVM.h"
#include "llvm/Support/JSON.h"
#include "llvm/Support/raw_ostream.h"
#include <filesystem>
#include <set>
#include <stdexcept>
using namespace mlir;
namespace {
// Parsing an operation without a location supplies its position in the input
// MLIR. These temporary positions are not Python source provenance.
Location sourceLocation(Location loc, llvm::StringRef input) {
  auto unknown = UnknownLoc::get(loc.getContext());
  if (auto file = dyn_cast<FileLineColRange>(loc)) {
    auto name = file.getFilename().getValue();
    if (name == input ||
        (name.contains("/.analysis-") && name.ends_with("/input.mlir")))
      return unknown;
    return loc;
  }
  if (auto named = dyn_cast<NameLoc>(loc)) {
    if (named.getName().getValue().starts_with("acir.type:"))
      return unknown;
    auto child = sourceLocation(named.getChildLoc(), input);
    return NameLoc::get(named.getName(), child);
  }
  if (auto fused = dyn_cast<FusedLoc>(loc)) {
    SmallVector<Location> sources;
    for (auto item : fused.getLocations()) {
      auto source = sourceLocation(item, input);
      if (!isa<UnknownLoc>(source))
        sources.push_back(source);
    }
    return FusedLoc::get(sources, fused.getMetadata(), loc.getContext());
  }
  if (auto call = dyn_cast<CallSiteLoc>(loc)) {
    auto callee = sourceLocation(call.getCallee(), input);
    auto caller = sourceLocation(call.getCaller(), input);
    if (isa<UnknownLoc>(callee))
      return caller;
    if (isa<UnknownLoc>(caller))
      return callee;
    return CallSiteLoc::get(callee, caller);
  }
  return loc;
}
void cleanLocations(mlir::ModuleOp module, llvm::StringRef input) {
  module.walk([&](Operation *op) {
    op->setLoc(sourceLocation(op->getLoc(), input));
    for (auto &region : op->getRegions())
      for (auto &block : region)
        for (auto arg : block.getArguments())
          arg.setLoc(sourceLocation(arg.getLoc(), input));
  });
}

// The native debug printer emits UnknownLoc on every generated op/argument.
// Elide only those annotations and their alias definitions. Respect quoted
// attributes and comments so user strings containing "loc(unknown)" survive.
std::string withoutUnknownLocations(llvm::StringRef assembly) {
  SmallVector<llvm::StringRef> lines;
  assembly.split(lines, '\n');
  while (!lines.empty() && lines.back().empty())
    lines.pop_back();
  std::set<std::string> aliases;
  for (auto line : lines) {
    if (line.starts_with("#") && line.ends_with(" = loc(unknown)"))
      aliases.insert(line.take_front(line.find(" = ")).str());
  }
  std::string result;
  std::set<std::string> usedAliases;
  for (auto line : lines) {
    if (line.starts_with("#") && line.ends_with(" = loc(unknown)"))
      continue;
    bool quoted = false;
    for (size_t i = 0; i < line.size();) {
      if (quoted && line[i] == '\\' && i + 1 < line.size()) {
        result.append(line.substr(i, 2));
        i += 2;
        continue;
      }
      if (line[i] == '"') {
        quoted = !quoted;
        result += line[i++];
        continue;
      }
      if (!quoted && line.substr(i).starts_with("//")) {
        result += line.substr(i).str();
        break;
      }
      if (!quoted && line.substr(i).starts_with(" loc(") && i &&
          llvm::StringRef("=,[(").find(line[i - 1]) == llvm::StringRef::npos) {
        auto end = line.find(')', i + 5);
        if (end != llvm::StringRef::npos) {
          auto name = line.slice(i + 5, end);
          if (name == "unknown" || aliases.count(name.str())) {
            i = end + 1;
            continue;
          }
        }
      }
      if (!quoted && line[i] == '#') {
        auto end = line.find_first_not_of("abcdefghijklmnopqrstuvwxyzABCDEFGHIJ"
                                          "KLMNOPQRSTUVWXYZ0123456789_$.-",
                                          i + 1);
        auto name =
            line.slice(i, end == llvm::StringRef::npos ? line.size() : end);
        if (aliases.count(name.str()))
          usedAliases.insert(name.str());
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

void save(mlir::ModuleOp module, const std::filesystem::path &path) {
  std::error_code ec;
  llvm::raw_fd_ostream out(path.string(), ec);
  if (ec)
    throw std::runtime_error(ec.message());
  std::string assembly;
  llvm::raw_string_ostream buffer(assembly);
  module.print(buffer, OpPrintingFlags().enableDebugInfo());
  out << withoutUnknownLocations(assembly);
}
void json(const llvm::json::Value &value, const std::filesystem::path &path) {
  std::error_code ec;
  llvm::raw_fd_ostream out(path.string(), ec);
  if (ec)
    throw std::runtime_error(ec.message());
  out << llvm::formatv("{0:2}\n", value);
}
llvm::json::Object graph(mlir::ModuleOp module) {
  auto circuit = *module.getOps<acir::CircuitOp>().begin();
  llvm::json::Array nodes, edges, conflicts;
  for (auto fire : circuit.getBody().getOps<acir::FireOp>())
    nodes.push_back(llvm::json::Object{
        {"id", fire.getName().str()},
        {"txn", fire->getAttrOfType<IntegerAttr>("txn").getInt()}});
  auto report = [&](StringRef key) {
    return acir::jsonAttribute(circuit->getAttr(key));
  };
  return llvm::json::Object{{"nodes", std::move(nodes)},
                            {"edges", report("edges")},
                            {"conflicts", report("conflicts")},
                            {"schedule", report("schedule")},
                            {"diagnostics", llvm::json::Array{}}};
}
} // namespace
int acir::compileMain(int argc, char **argv) {
  llvm::InitLLVM init(argc, argv);
  try {
    if (argc < 3)
      throw std::runtime_error(
          "usage: acir-compile INPUT.mlir OUTPUT_DIR [--opt] [--stop-after "
          "source|transaction|rtl] [--cycles N] [--config NAME=VALUE]");
    bool optimize = false;
    std::string stop = "rtl";
    unsigned cycles = 0;
    std::map<std::string, int64_t> config;
    for (int i = 3; i < argc; ++i) {
      std::string arg = argv[i];
      if (arg == "--opt")
        optimize = true;
      else if (arg == "--cycles" && i + 1 < argc)
        cycles = std::stoul(argv[++i]);
      else if (arg == "--stop-after" && i + 1 < argc)
        stop = argv[++i];
      else if (arg == "--config" && i + 1 < argc) {
        std::string c = argv[++i];
        auto eq = c.find('=');
        if (eq == std::string::npos)
          throw std::runtime_error("configuration requires NAME=VALUE");
        config[c.substr(0, eq)] = std::stoll(c.substr(eq + 1), nullptr, 0);
      } else
        throw std::runtime_error("unknown command argument: " + arg);
    }
    if (stop != "source" && stop != "transaction" && stop != "rtl")
      throw std::runtime_error("invalid stop stage");
    DialectRegistry registry;
    registry
        .insert<acir::ACIRDialect, arith::ArithDialect, cf::ControlFlowDialect,
                func::FuncDialect, scf::SCFDialect>();
    MLIRContext context(registry);
    context.loadAllAvailableDialects();
    Builder b(&context);
    auto module = parseSourceFile<mlir::ModuleOp>(argv[1], &context);
    if (!module || failed(verify(*module)))
      return 1;
    auto meta = (*module)->getAttrOfType<DictionaryAttr>("acir.model");
    if (!meta || meta.getAs<IntegerAttr>("format_version").getInt() != 8)
      throw std::runtime_error(
          "expected source format_version 8; recompile ACPy");
    auto inputStage = (*module)->getAttrOfType<StringAttr>("acir.stage");
    if (inputStage && !config.empty())
      throw std::runtime_error(
          "constructor configuration is frozen in transaction/RTL MLIR; reload "
          "source MLIR to change it");
    if (inputStage &&
        ((inputStage.getValue() == "rtl" && stop != "rtl") ||
         (inputStage.getValue() == "transaction" && stop == "source")))
      throw std::runtime_error("requested stage precedes input stage");
    NamedAttrList defaults(
        (*module)->getAttrOfType<DictionaryAttr>("acir.config"));
    for (auto c : meta.getAs<ArrayAttr>("config")) {
      auto d = cast<DictionaryAttr>(c);
      auto name = d.getAs<StringAttr>("name").getValue();
      if (auto value = d.getAs<IntegerAttr>("default"))
        if (!defaults.get(name)) {
          auto type = d.getAs<StringAttr>("type").getValue();
          auto width = type == "bool" ? 1 : std::stoul(type.drop_front().str());
          defaults.set(
              name, IntegerAttr::get(b.getIntegerType(width), value.getInt()));
        }
    }
    if (!defaults.empty())
      (*module)->setAttr("acir.config", defaults.getDictionary(&context));
    if (!config.empty()) {
      NamedAttrList attrs(
          (*module)->getAttrOfType<DictionaryAttr>("acir.config"));
      for (auto c : meta.getAs<ArrayAttr>("config")) {
        auto d = cast<DictionaryAttr>(c);
        auto name = d.getAs<StringAttr>("name").getValue();
        if (config.count(name.str())) {
          auto type = d.getAs<StringAttr>("type").getValue();
          unsigned width =
              type == "bool" ? 1 : std::stoul(type.drop_front().str());
          attrs.set(name, IntegerAttr::get(b.getIntegerType(width),
                                           config[name.str()]));
          config.erase(name.str());
        }
      }
      if (!config.empty())
        throw std::runtime_error("unknown configuration: " +
                                 config.begin()->first);
      (*module)->setAttr("acir.config", attrs.getDictionary(&context));
    }
    if (optimize) {
      PassManager pm(&context);
      pm.addPass(createCanonicalizerPass());
      pm.addPass(createCSEPass());
      if (failed(pm.run(*module)))
        return 1;
    }
    cleanLocations(*module, argv[1]);
    std::filesystem::path directory(argv[2]);
    std::filesystem::create_directories(directory);
    auto stage = (*module)->getAttrOfType<StringAttr>("acir.stage");
    std::string current = stage ? stage.getValue().str() : "source";
    if (current == "source") {
      save(*module, directory / "model.source.mlir");
      save(*module, directory / "model.acir.mlir");
      if (stop == "source") {
        if (cycles)
          throw std::runtime_error("execute transaction or RTL stage");
        return 0;
      }
      PassManager pm(&context);
      pm.addPass(createTransactionPass());
      if (failed(pm.run(*module)))
        return 1;
      current = "transaction";
    }
    if (current == "transaction") {
      save(*module, directory / "model.transaction.mlir");
      json(graph(*module), directory / "rule-graph.json");
      if (cycles)
        json(acir::execute(*module, cycles),
             directory / "transaction-trace.json");
      if (stop == "transaction")
        return 0;
      PassManager pm(&context);
      pm.addPass(createRTLPass());
      if (failed(pm.run(*module)))
        return 1;
    }
    save(*module, directory / "model.rtl.mlir");
    if (cycles)
      json(acir::execute(*module, cycles), directory / "rtl-trace.json");
    return 0;
  } catch (const std::exception &e) {
    llvm::errs() << "acir-compile: " << e.what() << '\n';
    return 1;
  }
}
