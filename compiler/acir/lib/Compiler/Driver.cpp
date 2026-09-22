#include "CompilerInternal.h"

#include "acir/Dialect/ACIR/ACIROps.h"
#include "acir/Dialect/ACIR/GraphRegion.h"
#include "acir/InitAllDialects.h"
#include "acir/InitAllPasses.h"
#include "acir/Transforms/Passes.h"

#include "mlir/IR/Diagnostics.h"
#include "mlir/IR/Location.h"
#include "mlir/IR/SymbolTable.h"
#include "mlir/IR/Verifier.h"
#include "mlir/Parser/Parser.h"
#include "mlir/Pass/PassManager.h"
#include "llvm/ADT/STLExtras.h"
#include "llvm/ADT/SmallString.h"
#include "llvm/ADT/StringExtras.h"
#include "llvm/Support/Errc.h"
#include "llvm/Support/Path.h"
#include "llvm/Support/raw_ostream.h"

#include <algorithm>
#include <iterator>
#include <map>
#include <mutex>
#include <set>
#include <utility>

namespace acir::compiler {
namespace {

struct DriverState {
  mlir::MLIRContext context{mlir::MLIRContext::Threading::DISABLED};
  mlir::OwningOpRef<mlir::ModuleOp> module;
};

std::string severityName(mlir::DiagnosticSeverity severity) {
  switch (severity) {
  case mlir::DiagnosticSeverity::Error:
    return "error";
  case mlir::DiagnosticSeverity::Warning:
    return "warning";
  case mlir::DiagnosticSeverity::Remark:
    return "remark";
  case mlir::DiagnosticSeverity::Note:
    return "note";
  }
  return "error";
}

struct DiagnosticSourceFrame {
  std::string message;
  SourceLocation source;
};

std::vector<DiagnosticSourceFrame>
sourceLocations(mlir::Location location,
                llvm::StringRef message = "source definition") {
  if (auto file = mlir::dyn_cast<mlir::FileLineColLoc>(location))
    return {{message.str(),
             {file.getFilename().str(), file.getLine(), file.getColumn()}}};
  if (auto named = mlir::dyn_cast<mlir::NameLoc>(location))
    return sourceLocations(named.getChildLoc(), named.getName().getValue());
  if (auto call = mlir::dyn_cast<mlir::CallSiteLoc>(location)) {
    auto result = sourceLocations(call.getCallee(), message);
    auto callers = sourceLocations(call.getCaller(), "inline callsite");
    result.insert(result.end(), std::make_move_iterator(callers.begin()),
                  std::make_move_iterator(callers.end()));
    return result;
  }
  if (auto fused = mlir::dyn_cast<mlir::FusedLoc>(location)) {
    std::vector<DiagnosticSourceFrame> result;
    for (mlir::Location child : fused.getLocations()) {
      auto origins = sourceLocations(child, "alternate source origin");
      result.insert(result.end(), std::make_move_iterator(origins.begin()),
                    std::make_move_iterator(origins.end()));
    }
    return result;
  }
  return {};
}

std::string defaultDiagnosticCode(CompilerStage stage) {
  switch (stage) {
  case CompilerStage::AcirParse:
    return "ACIR-PARSE-001";
  case CompilerStage::AcirVerify:
    return "ACIR-VERIFY-001";
  case CompilerStage::AcirNormalize:
    return "ACIR-NORMALIZE-001";
  case CompilerStage::TopologyClosure:
    return "ACIR-CLOSURE-001";
  }
  return "ACIR-COMPILER-001";
}

CompilerDiagnostic makeDiagnostic(CompilerStage stage, llvm::StringRef code,
                                  llvm::StringRef message) {
  return {.stage = compilerStageName(stage).str(),
          .code = code.str(),
          .severity = "error",
          .message = message.str()};
}

llvm::Error compilerFailure(CompilerStage stage, llvm::StringRef code,
                            llvm::StringRef message) {
  return llvm::make_error<CompilerError>(
      std::vector<CompilerDiagnostic>{makeDiagnostic(stage, code, message)});
}

class DiagnosticCapture {
public:
  explicit DiagnosticCapture(mlir::MLIRContext &context)
      : handler_(&context, [&](mlir::Diagnostic &diagnostic) {
          auto sources = sourceLocations(diagnostic.getLocation());
          CompilerDiagnostic captured{
              .stage = compilerStageName(stage_).str(),
              .code = detail::diagnosticCodeFromMetadata(diagnostic)
                          .value_or(defaultDiagnosticCode(stage_)),
              .severity = severityName(diagnostic.getSeverity()),
              .message = diagnostic.str(),
              .source = sources.empty()
                            ? std::nullopt
                            : std::optional(sources.front().source)};
          for (const auto &frame : llvm::ArrayRef(sources).drop_front())
            captured.related.push_back(
                {.message = frame.message, .source = frame.source});
          diagnostics_.push_back(std::move(captured));
          return mlir::success();
        }) {}
  void setStage(CompilerStage stage) { stage_ = stage; }
  llvm::Error takeFailure(CompilerStage stage) {
    if (diagnostics_.empty())
      diagnostics_.push_back(makeDiagnostic(stage, defaultDiagnosticCode(stage),
                                            "compiler stage failed"));
    return llvm::make_error<CompilerError>(std::move(diagnostics_));
  }
  std::vector<CompilerDiagnostic> takeDiagnostics() {
    return std::move(diagnostics_);
  }

private:
  CompilerStage stage_ = CompilerStage::AcirParse;
  std::vector<CompilerDiagnostic> diagnostics_;
  mlir::ScopedDiagnosticHandler handler_;
};

std::string printModule(mlir::ModuleOp module, bool assumeVerified = false) {
  std::string bytes;
  llvm::raw_string_ostream output(bytes);
  mlir::OpPrintingFlags flags;
  if (assumeVerified)
    flags.assumeVerified();
  module.print(output, flags);
  return bytes;
}

void addArtifact(CompilerResult &result, std::string path, ArtifactKind kind,
                 std::string bytes);

template <typename Keep>
std::string printUnit(mlir::ModuleOp source, bool copyModuleAttributes,
                      Keep &&keep, llvm::StringRef unitKind = {},
                      llvm::StringRef unitSource = {}) {
  mlir::OwningOpRef<mlir::ModuleOp> unit(
      mlir::ModuleOp::create(source.getLoc()));
  if (copyModuleAttributes)
    unit->getOperation()->setAttrs(source->getAttrs());
  if (!unitKind.empty())
    unit->getOperation()->setAttr(
        "ac.unit_kind", mlir::StringAttr::get(source.getContext(), unitKind));
  if (!unitSource.empty())
    unit->getOperation()->setAttr(
        "ac.unit_source",
        mlir::StringAttr::get(source.getContext(), unitSource));
  if (unitKind == "source")
    unit->getOperation()->setAttr(
        "ac.model_kind",
        mlir::StringAttr::get(source.getContext(), "queue_graph"));
  for (mlir::Operation &operation : source.getBody()->getOperations())
    if (keep(operation))
      unit->getBody()->push_back(operation.clone());
  // AC package units are deliberately link-time fragments, so references to
  // definitions owned by sibling units are unresolved until package linking.
  return printModule(*unit, /*assumeVerified=*/true);
}

bool isPortableDefinitionName(llvm::StringRef name) {
  if (name.empty() || !(llvm::isAlpha(name.front()) || name.front() == '_'))
    return false;
  return llvm::all_of(name.drop_front(), [](char character) {
    return llvm::isAlnum(character) || character == '_';
  });
}

std::string portablePathKey(llvm::StringRef name) {
  std::string key = name.str();
  llvm::transform(key, key.begin(),
                  [](char character) { return llvm::toLower(character); });
  return key;
}

llvm::Expected<std::string> sourceDefinitionName(ac::ModuleOp definition) {
  auto sourceName =
      definition->getAttrOfType<mlir::StringAttr>("ac.definition_name");
  if (!sourceName && !definition.getBody().front().empty())
    if (auto moduleCase = mlir::dyn_cast<ac::ModuleCaseOp>(
            definition.getBody().front().front()))
      sourceName = moduleCase->getAttrOfType<mlir::StringAttr>(
          "ac.definition_name");
  if (!sourceName || sourceName.getValue().empty())
    return llvm::createStringError(
        std::make_error_code(std::errc::invalid_argument),
        "module symbol '%s' requires a non-empty string ac.definition_name",
        definition.getSymName().str().c_str());
  return sourceName.getValue().str();
}

llvm::Expected<std::string> sourceUnitPath(ac::ModuleOp definition) {
  auto source = definition.getSource().getImplementation();
  if (!source || source.getValue().empty())
    return llvm::createStringError(
        std::make_error_code(std::errc::invalid_argument),
        "module symbol '%s' requires a non-empty string ac.source_file",
        definition.getSymName().str().c_str());
  llvm::StringRef value = source.getValue();
  if (value.contains('\\') ||
      llvm::sys::path::is_absolute(value, llvm::sys::path::Style::posix))
    return llvm::createStringError(
        std::make_error_code(std::errc::invalid_argument),
        "module source '%s' must be a relative POSIX path",
        value.str().c_str());
  llvm::SmallVector<llvm::StringRef> components;
  value.split(components, '/');
  for (llvm::StringRef component : components) {
    if (component.empty() || component == "." || component == ".." ||
        !llvm::all_of(component, [](char character) {
          return llvm::isAlnum(character) || character == '_' ||
                 character == '-' || character == '.';
        }))
      return llvm::createStringError(
          std::make_error_code(std::errc::invalid_argument),
          "module source '%s' is not a safe readable AC package path",
          value.str().c_str());
  }
  if (llvm::sys::path::extension(value, llvm::sys::path::Style::posix) != ".py")
    return llvm::createStringError(
        std::make_error_code(std::errc::invalid_argument),
        "module source '%s' must end in .py", value.str().c_str());
  llvm::SmallString<256> path("sources");
  llvm::sys::path::append(path, llvm::sys::path::Style::posix, value);
  llvm::sys::path::replace_extension(path, "ac", llvm::sys::path::Style::posix);
  return path.str().str();
}

llvm::Expected<std::string> sourceInterfaceUnitPath(llvm::StringRef value) {
  if (value.contains('\\') ||
      llvm::sys::path::is_absolute(value, llvm::sys::path::Style::posix))
    return llvm::createStringError(
        std::make_error_code(std::errc::invalid_argument),
        "interface source '%s' must be a relative POSIX path",
        value.str().c_str());
  llvm::SmallVector<llvm::StringRef> components;
  value.split(components, '/');
  for (llvm::StringRef component : components) {
    if (component.empty() || component == "." || component == ".." ||
        !llvm::all_of(component, [](char character) {
          return llvm::isAlnum(character) || character == '_' ||
                 character == '-' || character == '.';
        }))
      return llvm::createStringError(
          std::make_error_code(std::errc::invalid_argument),
          "interface source '%s' is not a safe readable AC package path",
          value.str().c_str());
  }
  if (llvm::sys::path::extension(value, llvm::sys::path::Style::posix) != ".py")
    return llvm::createStringError(
        std::make_error_code(std::errc::invalid_argument),
        "interface source '%s' must end in .py", value.str().c_str());
  llvm::SmallString<256> path("interfaces");
  llvm::sys::path::append(path, llvm::sys::path::Style::posix, value);
  llvm::sys::path::replace_extension(path, "ac", llvm::sys::path::Style::posix);
  return path.str().str();
}

std::string printSourceInterfaceUnit(
    mlir::ModuleOp source, ac::TypeScopeOp typeScope, llvm::StringRef owner,
    llvm::ArrayRef<mlir::Operation *> typeDefinitions,
    llvm::ArrayRef<mlir::Operation *> moduleDefinitions) {
  mlir::OwningOpRef<mlir::ModuleOp> unit(
      mlir::ModuleOp::create(source.getLoc()));
  unit->getOperation()->setAttr(
      "ac.unit_kind", mlir::StringAttr::get(source.getContext(), "interface"));
  unit->getOperation()->setAttr(
      "ac.unit_source", mlir::StringAttr::get(source.getContext(), owner));
  unit->getOperation()->setAttr(
      "ac.interface_kind", mlir::StringAttr::get(source.getContext(), "source"));
  if (typeScope && !typeDefinitions.empty()) {
    auto cloned = mlir::cast<ac::TypeScopeOp>(typeScope->clone());
    cloned->removeAttr("dlti.dl_spec");
    std::set<llvm::StringRef> ownedSymbols;
    for (mlir::Operation *definition : typeDefinitions)
      ownedSymbols.insert(definition
                              ->getAttrOfType<mlir::StringAttr>(
                                  mlir::SymbolTable::getSymbolAttrName())
                              .getValue());
    for (mlir::Operation &operation :
         llvm::make_early_inc_range(cloned.getBody().front())) {
      auto symbol = operation.getAttrOfType<mlir::StringAttr>(
          mlir::SymbolTable::getSymbolAttrName());
      if (!symbol || !ownedSymbols.contains(symbol.getValue()))
        operation.erase();
    }
    unit->getBody()->push_back(cloned.getOperation());
  }
  for (mlir::Operation *definition : moduleDefinitions) {
    auto module = mlir::cast<ac::ModuleOp>(definition);
    mlir::OperationState state(definition->getLoc(),
                               ac::ModuleImportOp::getOperationName());
    state.addAttribute(
        mlir::SymbolTable::getSymbolAttrName(),
        definition->getAttr(mlir::SymbolTable::getSymbolAttrName()));
    state.addAttribute("source", module.getSource());
    state.addAttribute("schema", module.getSchema());
    unit->getBody()->push_back(mlir::Operation::create(state));
  }
  return printModule(*unit, /*assumeVerified=*/true);
}

std::string printLayoutUnit(mlir::ModuleOp source, ac::TypeScopeOp typeScope) {
  mlir::OwningOpRef<mlir::ModuleOp> unit(
      mlir::ModuleOp::create(source.getLoc()));
  unit->getOperation()->setAttr(
      "ac.unit_kind", mlir::StringAttr::get(source.getContext(), "interface"));
  unit->getOperation()->setAttr(
      "ac.unit_source",
      mlir::StringAttr::get(source.getContext(), "_compiler/layouts.py"));
  unit->getOperation()->setAttr(
      "ac.interface_kind",
      mlir::StringAttr::get(source.getContext(), "layouts"));
  auto cloned = mlir::cast<ac::TypeScopeOp>(typeScope->clone());
  cloned.getBody().front().getOperations().clear();
  unit->getBody()->push_back(cloned.getOperation());
  return printModule(*unit, /*assumeVerified=*/true);
}

llvm::Error emitInterfaceUnits(
    mlir::ModuleOp module, CompilerResult &result,
    const std::map<std::string, std::vector<mlir::Operation *>> &moduleGroups = {}) {
  ac::TypeScopeOp typeScope;
  for (ac::TypeScopeOp candidate : module.getOps<ac::TypeScopeOp>()) {
    if (typeScope)
      return compilerFailure(CompilerStage::AcirVerify, "ACIR-EMIT-002",
                             "AC package has multiple type scopes");
    typeScope = candidate;
  }
  if (typeScope)
    addArtifact(result, "interfaces/_compiler/layouts.ac", ArtifactKind::Acir,
                printLayoutUnit(module, typeScope));
  std::map<std::string, std::vector<mlir::Operation *>> typeGroups;
  if (typeScope) {
    for (mlir::Operation &definition : typeScope.getBody().front()) {
      auto source =
          definition.getAttrOfType<mlir::StringAttr>("ac.source_file");
      typeGroups[source && !source.getValue().empty()
                     ? source.getValue().str()
                     : std::string("_compiler/common.py")]
          .push_back(&definition);
    }
  }
  std::set<std::string> owners;
  for (const auto &[owner, _] : typeGroups)
    owners.insert(owner);
  for (const auto &[owner, _] : moduleGroups)
    owners.insert(owner);
  std::map<std::string, std::string> portablePaths;
  for (const std::string &owner : owners) {
    auto logicalPath = sourceInterfaceUnitPath(owner);
    if (!logicalPath)
      return compilerFailure(CompilerStage::AcirVerify, "ACIR-EMIT-002",
                             llvm::toString(logicalPath.takeError()));
    std::string key = portablePathKey(*logicalPath);
    auto [path, inserted] = portablePaths.emplace(key, *logicalPath);
    if (!inserted && path->second != *logicalPath)
      return compilerFailure(CompilerStage::AcirVerify, "ACIR-EMIT-002",
                             "interface sources '" + path->second + "' and '" +
                                 *logicalPath +
                                 "' collide as portable AC package paths");
    auto typeIt = typeGroups.find(owner);
    auto moduleIt = moduleGroups.find(owner);
    llvm::ArrayRef<mlir::Operation *> typeDefinitions =
        typeIt == typeGroups.end()
            ? llvm::ArrayRef<mlir::Operation *>()
            : llvm::ArrayRef<mlir::Operation *>(typeIt->second);
    llvm::ArrayRef<mlir::Operation *> moduleDefinitions =
        moduleIt == moduleGroups.end()
            ? llvm::ArrayRef<mlir::Operation *>()
            : llvm::ArrayRef<mlir::Operation *>(moduleIt->second);
    addArtifact(result, *logicalPath, ArtifactKind::Acir,
                printSourceInterfaceUnit(module, typeScope, owner,
                                         typeDefinitions, moduleDefinitions));
  }
  return llvm::Error::success();
}

llvm::Error emitAcirPackage(mlir::ModuleOp module, CompilerResult &result) {
  ac::SystemOp selectedSystem;
  for (ac::SystemOp system : module.getOps<ac::SystemOp>())
    if (system.getSelected()) {
      if (selectedSystem)
        return compilerFailure(CompilerStage::TopologyClosure, "ACIR-EMIT-002",
                               "AC package has multiple selected systems");
      selectedSystem = system;
    }

  // Flat models have no independently reusable ac.module definitions. They
  // still use the package shape, with their complete program in core.ac and an
  // empty interface unit.
  if (!selectedSystem) {
    addArtifact(result, "core.ac", ArtifactKind::Acir, printModule(module));
    return emitInterfaceUnits(module, result);
  }

  llvm::StringRef rootName = selectedSystem.getRootAttr().getValue();
  ac::ModuleOp rootDefinition;
  std::map<std::string, std::vector<mlir::Operation *>> sourceGroups;
  std::map<std::string, std::string> portablePaths;
  for (ac::ModuleOp definition : module.getOps<ac::ModuleOp>()) {
    auto sourceName = sourceDefinitionName(definition);
    if (!sourceName)
      return compilerFailure(CompilerStage::TopologyClosure, "ACIR-EMIT-002",
                             llvm::toString(sourceName.takeError()));
    if (!isPortableDefinitionName(*sourceName))
      return compilerFailure(
          CompilerStage::TopologyClosure, "ACIR-EMIT-002",
          "module definition '" + *sourceName +
              "' is not a safe readable AC package file name");
    if (definition.getSymName() == rootName) {
      rootDefinition = definition;
    }
  }
  if (!rootDefinition)
    return compilerFailure(CompilerStage::TopologyClosure, "ACIR-EMIT-002",
                           "selected system root definition is missing");

  auto rootDefinitionName = sourceDefinitionName(rootDefinition);
  if (!rootDefinitionName)
    return compilerFailure(CompilerStage::TopologyClosure, "ACIR-EMIT-002",
                           llvm::toString(rootDefinitionName.takeError()));
  bool sourceUnitCompile =
      llvm::StringRef(*rootDefinitionName).starts_with("_acc_source_unit_root");
  auto rootSource = rootDefinition.getSource().getImplementation();
  std::set<mlir::Operation *> coreOwned{selectedSystem.getOperation(),
                                        rootDefinition.getOperation()};
  for (ac::ModuleOp definition : module.getOps<ac::ModuleOp>()) {
    if (definition == rootDefinition)
      continue;
    auto source = definition.getSource().getImplementation();
    if (source != rootSource)
      return compilerFailure(
          CompilerStage::TopologyClosure, "ACIR-EMIT-002",
          "compiled AC unit contains a module body owned by another Python "
          "source; invoke acc.py directly for that implementation source");
    if (!sourceUnitCompile) {
      coreOwned.insert(definition.getOperation());
      continue;
    }
    auto logicalPath = sourceUnitPath(definition);
    if (!logicalPath)
      return compilerFailure(CompilerStage::TopologyClosure, "ACIR-EMIT-002",
                             llvm::toString(logicalPath.takeError()));
    std::string pathKey = portablePathKey(*logicalPath);
    auto [path, inserted] = portablePaths.emplace(pathKey, *logicalPath);
    if (!inserted && path->second != *logicalPath)
      return compilerFailure(CompilerStage::TopologyClosure, "ACIR-EMIT-002",
                             "Python sources '" + path->second + "' and '" +
                                 *logicalPath +
                                 "' collide as portable AC package paths");
    sourceGroups[*logicalPath].push_back(definition.getOperation());
  }

  for (mlir::Operation &operation : module.getBody()->getOperations()) {
    if (mlir::isa<ac::SystemOp, ac::ModuleOp, ac::TypeScopeOp>(operation))
      continue;
    auto source = operation.getAttrOfType<mlir::StringAttr>("ac.source_file");
    if (!source || source.getValue().empty()) {
      coreOwned.insert(&operation);
      continue;
    }
    if (source != rootSource)
      return compilerFailure(
          CompilerStage::TopologyClosure, "ACIR-EMIT-002",
          "compiled AC unit contains helper/state body owned by another "
          "Python source; invoke acc.py directly for that source");
    if (!sourceUnitCompile) {
      coreOwned.insert(&operation);
      continue;
    }
    llvm::SmallString<256> logicalPath("sources");
    llvm::sys::path::append(logicalPath, llvm::sys::path::Style::posix,
                            source.getValue());
    llvm::sys::path::replace_extension(logicalPath, "ac",
                                       llvm::sys::path::Style::posix);
    sourceGroups[logicalPath.str().str()].push_back(&operation);
  }

  if (!sourceUnitCompile)
    addArtifact(result, "core.ac", ArtifactKind::Acir,
                printUnit(
                    module, true,
                    [&](mlir::Operation &operation) {
                      return coreOwned.contains(&operation);
                    },
                    "core",
                    rootSource));
  std::map<std::string, std::vector<mlir::Operation *>> interfaceModuleGroups;
  for (const auto &[_, definitions] : sourceGroups)
    for (mlir::Operation *definition : definitions)
      if (auto family = mlir::dyn_cast<ac::ModuleOp>(definition))
        interfaceModuleGroups[family.getSource().getImplementation().str()]
            .push_back(definition);
  if (auto error = emitInterfaceUnits(module, result, interfaceModuleGroups))
    return error;
  for (auto &[logicalPath, definitions] : sourceGroups) {
    llvm::sort(definitions, [](mlir::Operation *left, mlir::Operation *right) {
      auto leftSymbol = left->getAttrOfType<mlir::StringAttr>(
          mlir::SymbolTable::getSymbolAttrName());
      auto rightSymbol = right->getAttrOfType<mlir::StringAttr>(
          mlir::SymbolTable::getSymbolAttrName());
      return std::make_pair(left->getName().getStringRef(),
                            leftSymbol ? leftSymbol.getValue()
                                       : llvm::StringRef()) <
             std::make_pair(right->getName().getStringRef(),
                            rightSymbol ? rightSymbol.getValue()
                                        : llvm::StringRef());
    });
    std::set<mlir::Operation *> owned;
    for (mlir::Operation *definition : definitions)
      owned.insert(definition);
    mlir::StringAttr source;
    if (auto module = mlir::dyn_cast<ac::ModuleOp>(definitions.front()))
      source = module.getSource().getImplementation();
    else
      source = definitions.front()->getAttrOfType<mlir::StringAttr>(
          "ac.source_file");
    if (!source)
      return compilerFailure(CompilerStage::AcirVerify, "ACIR-EMIT-002",
                             "source unit definition has no source owner");
    addArtifact(result, logicalPath, ArtifactKind::Acir,
                printUnit(
                    module, false,
                    [&](mlir::Operation &operation) {
                      return owned.contains(&operation);
                    },
                    "source", source.getValue()));
  }
  return llvm::Error::success();
}

bool requested(const CompilerRequest &request, ArtifactKind kind) {
  return llvm::is_contained(request.emits, kind);
}
bool namesStage(llvm::ArrayRef<std::string> names, CompilerStage stage) {
  return llvm::is_contained(names, compilerStageName(stage));
}
void addArtifact(CompilerResult &result, std::string path, ArtifactKind kind,
                 std::string bytes) {
  result.artifacts.push_back({std::move(path), kind, std::move(bytes)});
}

llvm::Error validateRequest(const CompilerRequest &request,
                            llvm::ArrayRef<CompilerStage> pipeline) {
  if (request.acirBytes.empty())
    return compilerFailure(CompilerStage::AcirParse, "ACIR-PARSE-001",
                           "ACIR input is empty");
  std::set<ArtifactKind> unique;
  for (ArtifactKind kind : request.emits) {
    if (!unique.insert(kind).second)
      return compilerFailure(CompilerStage::AcirParse, "ACIR-EMIT-001",
                             "artifact emission request is duplicated");
    if (kind == ArtifactKind::Acir &&
        (pipeline.empty() ||
         (pipeline.back() != CompilerStage::AcirVerify &&
          pipeline.back() != CompilerStage::TopologyClosure)))
      return compilerFailure(CompilerStage::TopologyClosure, "ACIR-EMIT-001",
                             "AC package emission requires verified ACIR");
  }
  return llvm::Error::success();
}

mlir::LogicalResult runPass(DriverState &state,
                            std::unique_ptr<mlir::Pass> pass) {
  mlir::PassManager manager(&state.context);
  manager.addPass(std::move(pass));
  return manager.run(state.module.get());
}

llvm::Error runStage(CompilerStage stage, const CompilerRequest &request,
                     DriverState &state, CompilerResult &result,
                     DiagnosticCapture &capture) {
  capture.setStage(stage);
  switch (stage) {
  case CompilerStage::AcirParse:
    state.module = mlir::parseSourceString<mlir::ModuleOp>(request.acirBytes,
                                                           &state.context);
    return state.module ? llvm::Error::success() : capture.takeFailure(stage);
  case CompilerStage::AcirVerify:
    if (mlir::failed(runPass(state, createVerifyACIRFilePass())))
      return capture.takeFailure(stage);
    if (requested(request, ArtifactKind::Acir) &&
        request.stopAfter == CompilerStage::AcirVerify) {
      // A source-owned AC unit must not retain a call to a helper definition
      // that will be published under another source path. Expand verified pure
      // helpers before package isolation while preserving rules and High ACIR.
      if (mlir::failed(runPass(state, createInlinePureHelpersPass())) ||
          mlir::failed(runPass(state, createVerifyACIRFilePass())))
        return capture.takeFailure(stage);
      // A published unit is re-parsed by the linker and the backends, which
      // only read the canonical attribute carrier, never MLIR debug locations.
      // Materialize it before publication so a source-owned unit keeps the
      // provenance of the operations it publishes.
      materializeSourceProvenance(*state.module);
      return emitAcirPackage(*state.module, result);
    }
    return llvm::Error::success();
  case CompilerStage::AcirNormalize: {
    mlir::PassManager manager(&state.context);
    if (request.profile == CompilerProfile::Custom) {
      llvm::raw_null_ostream errors;
      if (mlir::failed(mlir::parsePassPipeline(*request.customPipeline, manager,
                                               errors)))
        return compilerFailure(stage, "ACIR-PIPELINE-001",
                               "custom pipeline is invalid");
    } else {
      addRuleLoweringPipeline(manager);
      manager.addPass(createNormalizeACIRFilePass());
    }
    return mlir::succeeded(manager.run(state.module.get()))
               ? llvm::Error::success()
               : capture.takeFailure(stage);
  }
  case CompilerStage::TopologyClosure:
    if (mlir::failed(runPass(state, createFreezeTopologyPass())))
      return capture.takeFailure(stage);
    if (requested(request, ArtifactKind::Acir))
      return emitAcirPackage(*state.module, result);
    return llvm::Error::success();
  }
  llvm_unreachable("closed CompilerStage is exhaustive");
}

} // namespace

char CompilerError::ID = 0;
CompilerError::CompilerError(std::vector<CompilerDiagnostic> diagnostics)
    : diagnostics_(std::move(diagnostics)) {}
void CompilerError::log(llvm::raw_ostream &output) const {
  output << (diagnostics_.empty() ? "compiler failed without a diagnostic"
                                  : diagnostics_.front().code + ": " +
                                        diagnostics_.front().message);
}
std::error_code CompilerError::convertToErrorCode() const {
  return llvm::inconvertibleErrorCode();
}
llvm::StringRef compilerStageName(CompilerStage stage) {
  switch (stage) {
  case CompilerStage::AcirParse:
    return "acir-parse";
  case CompilerStage::AcirVerify:
    return "acir-verify";
  case CompilerStage::AcirNormalize:
    return "acir-normalize";
  case CompilerStage::TopologyClosure:
    return "topology-closure";
  }
  llvm_unreachable("closed CompilerStage is exhaustive");
}

std::optional<std::string>
detail::diagnosticCodeFromMetadata(mlir::Diagnostic &diagnostic) {
  for (mlir::DiagnosticArgument &argument : diagnostic.getMetadata()) {
    if (argument.getKind() !=
        mlir::DiagnosticArgument::DiagnosticArgumentKind::Attribute)
      continue;
    auto fields =
        mlir::dyn_cast<mlir::DictionaryAttr>(argument.getAsAttribute());
    if (fields)
      if (auto code = fields.getAs<mlir::StringAttr>("diagnostic.code");
          code && !code.getValue().empty())
        return code.getValue().str();
  }
  return std::nullopt;
}

llvm::Expected<CompilerResult> runCompiler(const CompilerRequest &request) {
  static std::once_flag passesRegistered;
  std::call_once(passesRegistered, [] { registerAllPasses(); });
  auto pipeline = detail::selectPipeline(request);
  if (!pipeline)
    return compilerFailure(CompilerStage::AcirParse, "ACIR-PIPELINE-001",
                           llvm::toString(pipeline.takeError()));
  if (auto error = validateRequest(request, *pipeline))
    return std::move(error);
  mlir::DialectRegistry registry;
  registerAllDialects(registry);
  DriverState state;
  state.context.appendDialectRegistry(registry);
  state.context.loadAllAvailableDialects();
  DiagnosticCapture capture(state.context);
  CompilerResult result;
  for (CompilerStage stage : *pipeline) {
    if (state.module && namesStage(request.dumpBefore, stage))
      addArtifact(result,
                  "dumps/" + compilerStageName(stage).str() + "-before.mlir",
                  ArtifactKind::Report, printModule(*state.module));
    if (auto error = runStage(stage, request, state, result, capture))
      return std::move(error);
    if (request.verifyAfterEach && state.module &&
        mlir::failed(mlir::verify(state.module.get())))
      return capture.takeFailure(stage);
    if (state.module &&
        (request.dumpAfterEach || namesStage(request.dumpAfter, stage)))
      addArtifact(result,
                  "dumps/" + compilerStageName(stage).str() + "-after.mlir",
                  ArtifactKind::Report, printModule(*state.module));
  }
  result.diagnostics = capture.takeDiagnostics();
  return result;
}

} // namespace acir::compiler
