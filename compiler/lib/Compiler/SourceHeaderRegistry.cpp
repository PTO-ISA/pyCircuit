#include "SourceHeaderHelpers.h"
#include "SourceNamespace.h"
#include "SourceUnit.h"
#include "pycircuit/Dialect/ACIR/SourceUnitValidation.h"

#include "mlir/IR/Builders.h"
#include "mlir/IR/OperationSupport.h"
#include "mlir/IR/SymbolTable.h"
#include "llvm/ADT/APSInt.h"
#include "llvm/ADT/DenseSet.h"
#include "llvm/ADT/STLExtras.h"
#include "llvm/ADT/StringSet.h"

#include <functional>
#include <tuple>

using namespace mlir;

namespace acir::compiler {
namespace {

std::string moduleName(DictionaryAttr owner) {
  StringRef package = owner.getAs<StringAttr>("package").getValue();
  StringRef path = owner.getAs<StringAttr>("path").getValue();
  SmallVector<StringRef> parts;
  path.drop_back(3).split(parts, '/');
  if (!parts.empty() && parts.back() == "__init__")
    parts.pop_back();
  std::string result = package.str();
  for (StringRef part : parts) {
    if (!result.empty())
      result.push_back('.');
    result.append(part);
  }
  return result;
}

StringRef symbolName(Operation *operation) {
  auto name =
      operation->getAttrOfType<StringAttr>(SymbolTable::getSymbolAttrName());
  return name ? name.getValue() : StringRef();
}

std::string exportKey(StringRef module, StringRef name) {
  return (Twine(module) + "::" + name).str();
}

bool ownerLess(DictionaryAttr left, DictionaryAttr right) {
  return ac::detail::compareClosedSourceStructure(left, right) < 0;
}

constexpr StringLiteral builtinModuleName = "pycircuit.__builtins__";

DictionaryAttr builtinOwner(OpBuilder &builder) {
  return builder.getDictionaryAttr({
      builder.getNamedAttr("package", builder.getStringAttr("pycircuit")),
      builder.getNamedAttr("path", builder.getStringAttr("__builtins__.py")),
  });
}

DictionaryAttr builtinOccurrence(OpBuilder &builder, FlatSymbolRefAttr symbol) {
  auto site = builder.getDictionaryAttr({
      builder.getNamedAttr("definition", symbol),
      builder.getNamedAttr("ast_path", builder.getArrayAttr({})),
  });
  return builder.getDictionaryAttr({
      builder.getNamedAttr("site", site),
      builder.getNamedAttr("expansion", builder.getArrayAttr({})),
  });
}

DictionaryAttr builtinLocation(OpBuilder &builder) {
  return builder.getDictionaryAttr({
      builder.getNamedAttr("path", builder.getStringAttr("__builtins__.py")),
      builder.getNamedAttr("line", builder.getI64IntegerAttr(1)),
      builder.getNamedAttr("column", builder.getI64IntegerAttr(1)),
      builder.getNamedAttr("end_line", builder.getI64IntegerAttr(1)),
      builder.getNamedAttr("end_column", builder.getI64IntegerAttr(1)),
  });
}

ac::StaticExprAttr builtinLiteral(OpBuilder &builder, uint64_t value,
                                  FlatSymbolRefAttr symbol) {
  llvm::APSInt integer(llvm::APInt(64, value), true);
  auto encoded = builder.getDictionaryAttr({
      builder.getNamedAttr("kind", builder.getStringAttr("integer")),
      builder.getNamedAttr("value",
                           ac::MathIntAttr::get(builder.getContext(), integer)),
  });
  return ac::StaticExprAttr::get(
      builder.getContext(),
      builder.getDictionaryAttr({
          builder.getNamedAttr("kind", builder.getStringAttr("literal")),
          builder.getNamedAttr("value", encoded),
          builder.getNamedAttr("origin", builtinOccurrence(builder, symbol)),
          builder.getNamedAttr("location", builtinLocation(builder)),
      }));
}

ac::StaticExprAttr builtinParameter(OpBuilder &builder, StringRef name,
                                    FlatSymbolRefAttr symbol) {
  auto reference = builder.getDictionaryAttr({
      builder.getNamedAttr("kind", builder.getStringAttr("parameter")),
      builder.getNamedAttr("owner", symbol),
      builder.getNamedAttr("name", builder.getStringAttr(name)),
  });
  return ac::StaticExprAttr::get(
      builder.getContext(),
      builder.getDictionaryAttr({
          builder.getNamedAttr("kind", builder.getStringAttr("reference")),
          builder.getNamedAttr("ref", reference),
          builder.getNamedAttr("origin", builtinOccurrence(builder, symbol)),
          builder.getNamedAttr("location", builtinLocation(builder)),
      }));
}

ac::StaticExprAttr builtinBinary(OpBuilder &builder, StringRef opcode,
                                 ac::StaticExprAttr lhs, ac::StaticExprAttr rhs,
                                 FlatSymbolRefAttr symbol) {
  return ac::StaticExprAttr::get(
      builder.getContext(),
      builder.getDictionaryAttr({
          builder.getNamedAttr("kind", builder.getStringAttr("binary")),
          builder.getNamedAttr("operator", builder.getStringAttr(opcode)),
          builder.getNamedAttr("lhs", lhs),
          builder.getNamedAttr("rhs", rhs),
          builder.getNamedAttr("origin", builtinOccurrence(builder, symbol)),
          builder.getNamedAttr("location", builtinLocation(builder)),
      }));
}

FailureOr<ac::ModuleImportOp>
createBuiltin(OpBuilder &builder, StringRef kind,
              ArrayRef<StringRef> parameterNames, ArrayRef<uint64_t> defaults,
              ArrayRef<std::pair<StringRef, Type>> inputs,
              ArrayRef<std::pair<StringRef, Type>> outputs,
              ArrayRef<SmallVector<unsigned>> dependencies,
              ac::detail::EmitError emitError) {
  std::string spelling = (Twine(builtinModuleName) + "." + kind).str();
  auto symbol = FlatSymbolRefAttr::get(builder.getContext(), spelling);
  SmallVector<Attribute> parameters;
  for (auto [name, value] : llvm::zip(parameterNames, defaults))
    parameters.push_back(ac::getStaticParameterAttr(
        builder, name, builtinLiteral(builder, value, symbol)));
  SmallVector<Type> inputTypes, outputTypes;
  SmallVector<Attribute> inputNames, outputNames;
  for (auto [name, type] : inputs) {
    inputTypes.push_back(type);
    inputNames.push_back(builder.getStringAttr(name));
  }
  for (auto [name, type] : outputs) {
    outputTypes.push_back(type);
    outputNames.push_back(builder.getStringAttr(name));
  }
  SmallVector<Attribute> summary;
  for (auto [ordinal, inputs] : llvm::enumerate(dependencies))
    summary.push_back(ac::getOutputDependencyAttr(builder, ordinal, inputs));

  OperationState state(builder.getUnknownLoc(),
                       ac::ModuleImportOp::getOperationName());
  state.addAttribute(SymbolTable::getSymbolAttrName(),
                     builder.getStringAttr(spelling));
  state.addAttribute("source_owner", builtinOwner(builder));
  state.addAttribute("parameters", builder.getArrayAttr(parameters));
  state.addAttribute("function_type", TypeAttr::get(builder.getFunctionType(
                                          inputTypes, outputTypes)));
  state.addAttribute("type_parameters", builder.getStrArrayAttr({"T"}));
  state.addAttribute("input_names", builder.getArrayAttr(inputNames));
  state.addAttribute("output_names", builder.getArrayAttr(outputNames));
  state.addAttribute("dependency_summary", builder.getArrayAttr(summary));
  state.addAttribute("primitive_kind", builder.getStringAttr(kind));
  state.addAttribute("ac.declaration_role",
                     builder.getStringAttr("import_snapshot"));
  state.addAttribute("ac.origin", builtinOccurrence(builder, symbol));
  auto declaration = dyn_cast<ac::ModuleImportOp>(builder.create(state));
  if (!declaration || failed(declaration.verify()))
    return emitError() << "failed to construct canonical builtin " << kind;
  return declaration;
}

} // namespace

FailureOr<SourceHeaderRegistry>
SourceHeaderRegistry::create(ArrayRef<ModuleOp> headers,
                             ac::detail::EmitError emitError) {
  if (headers.empty())
    return emitError()
           << "empty source header registry requires an explicit MLIRContext";
  return create(headers.front()->getContext(), headers, emitError);
}

FailureOr<SourceHeaderRegistry>
SourceHeaderRegistry::create(MLIRContext *context, ArrayRef<ModuleOp> headers,
                             ac::detail::EmitError emitError) {
  SourceHeaderRegistry registry;
  if (!context)
    return emitError() << "source header registry requires an MLIRContext";
  for (ModuleOp header : headers)
    if (header.getContext() != context)
      return emitError() << "source headers must share one MLIRContext";
  registry.builtinCatalog_ =
      OwningOpRef<ModuleOp>(ModuleOp::create(UnknownLoc::get(context)));
  OpBuilder builtinBuilder =
      OpBuilder::atBlockEnd((*registry.builtinCatalog_).getBody());
  auto addCatalogEntry =
      [&](StringRef kind, ArrayRef<StringRef> parameterNames,
          ArrayRef<uint64_t> defaults,
          ArrayRef<std::pair<StringRef, Type>> inputs,
          ArrayRef<std::pair<StringRef, Type>> outputs,
          ArrayRef<SmallVector<unsigned>> dependencies) -> LogicalResult {
    auto declaration =
        createBuiltin(builtinBuilder, kind, parameterNames, defaults, inputs,
                      outputs, dependencies, emitError);
    if (failed(declaration))
      return failure();
    auto symbol = FlatSymbolRefAttr::get(context, (*declaration).getSymName());
    registry.authorities_.try_emplace(symbol, *declaration);
    registry.modules_.try_emplace(symbol, *declaration);
    registry.builtins_.try_emplace(kind, *declaration);
    return success();
  };
  auto symbolFor = [&](StringRef kind) {
    return FlatSymbolRefAttr::get(
        context, (Twine(builtinModuleName) + "." + kind).str());
  };
  auto literalWidth = [&](StringRef kind, uint64_t value) {
    return builtinLiteral(builtinBuilder, value, symbolFor(kind));
  };
  auto parameterWidth = [&](StringRef kind, StringRef name) {
    return builtinParameter(builtinBuilder, name, symbolFor(kind));
  };
  auto bits = [&](ac::StaticExprAttr width) -> Type {
    return ac::BitsType::get(context, width);
  };
  auto one = [&](StringRef kind) { return bits(literalWidth(kind, 1)); };
  auto dataType = [&](StringRef kind) -> Type {
    return ac::TypeParamType::get(context, symbolFor(kind),
                                  builtinBuilder.getStringAttr("T"));
  };

  for (StringRef kind : {"dff", "dffe"}) {
    SmallVector<std::pair<StringRef, Type>> inputs{{"clk", one(kind)},
                                                   {"rst", one(kind)}};
    if (kind == "dffe")
      inputs.push_back({"en", one(kind)});
    inputs.append({{"d", dataType(kind)}, {"init", dataType(kind)}});
    SmallVector<std::pair<StringRef, Type>> outputs{{"q", dataType(kind)}};
    SmallVector<SmallVector<unsigned>> dependencies(1);
    if (failed(addCatalogEntry(kind, {}, {}, inputs, outputs, dependencies)))
      return failure();
  }

  auto addMemory = [&](StringRef kind, bool dualPort,
                       bool asynchronousRead) -> LogicalResult {
    Type data = dataType(kind);
    Type address = bits(parameterWidth(kind, "ADDR_WIDTH"));
    auto width = ac::StaticExprAttr::get(
        context,
        builtinBuilder.getDictionaryAttr(
            {builtinBuilder.getNamedAttr(
                 "kind", builtinBuilder.getStringAttr("type_width")),
             builtinBuilder.getNamedAttr("type", TypeAttr::get(data)),
             builtinBuilder.getNamedAttr(
                 "origin", builtinOccurrence(builtinBuilder, symbolFor(kind))),
             builtinBuilder.getNamedAttr("location",
                                         builtinLocation(builtinBuilder))}));
    auto eight = literalWidth(kind, 8);
    auto numerator =
        asynchronousRead
            ? width
            : builtinBinary(builtinBuilder, "add", width, literalWidth(kind, 7),
                            symbolFor(kind));
    Type strobe = bits(builtinBinary(builtinBuilder, "floordiv", numerator,
                                     eight, symbolFor(kind)));
    SmallVector<std::pair<StringRef, Type>> inputs{{"clk", one(kind)},
                                                   {"rst", one(kind)}};
    if (dualPort)
      inputs.append({{"ren0", one(kind)},
                     {"raddr0", address},
                     {"ren1", one(kind)},
                     {"raddr1", address}});
    else if (asynchronousRead)
      inputs.push_back({"raddr", address});
    else
      inputs.append({{"ren", one(kind)}, {"raddr", address}});
    inputs.append({{"wvalid", one(kind)},
                   {"waddr", address},
                   {"wdata", data},
                   {"wstrb", strobe}});
    SmallVector<std::pair<StringRef, Type>> outputs;
    if (dualPort)
      outputs.append({{"rdata0", data}, {"rdata1", data}});
    else
      outputs.push_back({"rdata", data});
    SmallVector<SmallVector<unsigned>> dependencies(outputs.size());
    if (asynchronousRead)
      dependencies[0].push_back(2);
    return addCatalogEntry(kind, {"ADDR_WIDTH", "DEPTH"}, {64, 1024}, inputs,
                           outputs, dependencies);
  };
  if (failed(addMemory("sync_mem", false, false)) ||
      failed(addMemory("sync_mem_dp", true, false)) ||
      failed(addMemory("byte_mem", false, true)))
    return failure();

  llvm::DenseMap<Attribute, ModuleOp> ownerHeaders;
  SmallVector<Operation *> snapshots;
  for (ModuleOp header : headers) {
    auto structure = ac::verifySourceInterfaceStructure(header, emitError);
    if (failed(structure))
      return failure();
    DictionaryAttr owner = structure->owner;
    if (!ownerHeaders.try_emplace(owner, header).second)
      return emitError() << "duplicate interface SourceOwner";
    std::string owningModule = moduleName(owner);
    if (!registry.moduleOwners_.try_emplace(owningModule, owner).second)
      return emitError() << "duplicate interface module owner '" << owningModule
                         << "'";
    registry.headers_.push_back(header);
  }

  for (ModuleOp header : registry.headers_) {
    auto interfaces = header->getAttrOfType<ArrayAttr>("ac.interfaces");
    for (auto [index, rawOwner] : llvm::enumerate(interfaces)) {
      if (index == 0)
        continue;
      if (!ownerHeaders.contains(rawOwner))
        return emitError() << "ac.interfaces names a dependency without an "
                              "explicitly supplied header";
    }
  }

  for (ModuleOp header : registry.headers_) {
    DictionaryAttr owner =
        header->getAttrOfType<DictionaryAttr>("ac.source_owner");
    llvm::DenseSet<Attribute> visited;
    visited.insert(owner);
    SmallVector<DictionaryAttr> pending;
    auto firstInterfaces = header->getAttrOfType<ArrayAttr>("ac.interfaces");
    for (auto [index, rawOwner] : llvm::enumerate(firstInterfaces))
      if (index != 0)
        pending.push_back(cast<DictionaryAttr>(rawOwner));
    for (size_t index = 0; index < pending.size(); ++index) {
      DictionaryAttr dependency = pending[index];
      if (!visited.insert(dependency).second)
        continue;
      ModuleOp dependencyHeader = ownerHeaders.lookup(dependency);
      auto dependencies =
          dependencyHeader->getAttrOfType<ArrayAttr>("ac.interfaces");
      for (auto [dependencyIndex, rawDependency] :
           llvm::enumerate(dependencies))
        if (dependencyIndex != 0)
          pending.push_back(cast<DictionaryAttr>(rawDependency));
    }
    SmallVector<DictionaryAttr> sortedDependencies;
    for (Attribute dependency : visited)
      if (dependency != owner)
        sortedDependencies.push_back(cast<DictionaryAttr>(dependency));
    llvm::sort(sortedDependencies, ownerLess);
    SmallVector<Attribute> closure{owner};
    llvm::append_range(closure, sortedDependencies);
    ArrayAttr expectedClosure = ArrayAttr::get(header.getContext(), closure);
    auto declaredInterfaces = header->getAttrOfType<ArrayAttr>("ac.interfaces");
    if (declaredInterfaces != expectedClosure)
      return emitError() << "ac.interfaces must contain the complete sorted "
                            "dependency closure";
    registry.interfaceClosures_.try_emplace(moduleName(owner), expectedClosure);
  }

  for (ModuleOp header : registry.headers_) {
    for (Operation &operation : header.getBody()->getOperations()) {
      auto symbol = dyn_cast<SymbolOpInterface>(&operation);
      if (!symbol)
        return emitError() << "interface contains a non-symbol operation";
      StringRef name = symbolName(&operation);
      auto role = operation.getAttrOfType<StringAttr>("ac.declaration_role");
      FlatSymbolRefAttr flat =
          FlatSymbolRefAttr::get(header.getContext(), name);
      if (role.getValue() == "import_snapshot") {
        snapshots.push_back(&operation);
        continue;
      }
      if (registry.authorities_.contains(flat))
        return emitError() << "duplicate declaration authority " << flat;
      registry.authorities_.try_emplace(flat, &operation);
      if (auto alias = dyn_cast<ac::TypeAliasOp>(operation))
        registry.aliases_.try_emplace(flat, alias);
      else if (isa<ac::ConstantOp, ac::EnumOp>(operation))
        continue;
      else if (auto record = dyn_cast<ac::StructOp>(operation))
        registry.records_.try_emplace(flat, record);
      else if (auto helper = dyn_cast<func::FuncOp>(operation))
        registry.helpers_.try_emplace(flat, helper);
      else if (auto module = dyn_cast<ac::ModuleImportOp>(operation)) {
        if (module.getPrimitiveKindAttr())
          return emitError()
                 << "source interfaces cannot declare trusted primitive "
                    "metadata; use the canonical builtin catalog";
        registry.modules_.try_emplace(flat, module);
      } else
        return emitError() << "interface contains an unsupported declaration";
    }
  }

  for (Operation *snapshot : snapshots) {
    auto owner = ac::detail::declarationSourceOwner(snapshot);
    if (auto module = dyn_cast<ac::ModuleImportOp>(snapshot)) {
      if (module.getPrimitiveKindAttr()) {
        auto builtin =
            registry.lookupBuiltin(module.getPrimitiveKindAttr().getValue());
        if (!builtin || !detail::sameDeclaration(builtin.declaration, module))
          return emitError()
                 << "primitive snapshot differs from the canonical builtin "
                    "catalog";
        continue;
      }
    }
    auto authorityHeader = ownerHeaders.find(owner);
    if (authorityHeader == ownerHeaders.end())
      return emitError()
             << "import snapshot has no explicitly supplied owner header";
    auto name =
        snapshot->getAttrOfType<StringAttr>(SymbolTable::getSymbolAttrName());
    Operation *authority = nullptr;
    for (Operation &candidate :
         authorityHeader->second.getBody()->getOperations())
      if (symbolName(&candidate) == name.getValue() &&
          candidate.getAttrOfType<StringAttr>("ac.declaration_role")
                  .getValue() == "definition") {
        authority = &candidate;
        break;
      }
    FlatSymbolRefAttr flat =
        name ? FlatSymbolRefAttr::get(snapshot->getContext(), name.getValue())
             : FlatSymbolRefAttr();
    if (!authority || !flat || !registry.authorities_.contains(flat) ||
        registry.authorities_.lookup(flat) != authority)
      return emitError() << "import snapshot does not resolve to its explicit "
                            "owning declaration";
    if (auto helper = dyn_cast<func::FuncOp>(snapshot)) {
      if (failed(detail::verifyHeaderHelper(helper, registry, emitError)))
        return emitError() << "invalid helper snapshot metadata";
    }
    if (!detail::sameDeclaration(authority, snapshot))
      return emitError()
             << "import snapshot differs from its owning header declaration";
  }

  auto resolver = [&](FlatSymbolRefAttr symbol) {
    return registry.resolveRecord(symbol);
  };
  for (auto &[symbol, alias] : registry.aliases_) {
    auto target = alias->getAttrOfType<DictionaryAttr>("target");
    if (!target || failed(ac::detail::verifyTypeResolved(
                       target, ac::detail::ExpectedTypeKind::Logical, resolver,
                       emitError)))
      return emitError() << "invalid type alias export " << symbol;
  }
  for (auto &[symbol, declaration] : registry.authorities_) {
    auto constant = dyn_cast<ac::ConstantOp>(declaration);
    if (!constant)
      continue;
    auto type = constant->getAttrOfType<DictionaryAttr>("type");
    auto value = constant->getAttrOfType<DictionaryAttr>("value");
    if (!type || !value ||
        failed(ac::detail::verifyStaticTypeStructure(type, emitError)) ||
        failed(ac::detail::verifyStaticValueMatchesType(
            value, type, ac::detail::ExpectedTypeKind::Static, resolver,
            emitError)))
      return emitError() << "invalid constant export " << symbol;
  }
  // Packed structs were checked by the current dialect verifier during
  // interface admission. Their exact {name, type} fields and nominal source
  // authority are retained above; no source constructor/helper is required.

  for (auto &[symbol, helper] : registry.helpers_)
    if (failed(detail::verifyHeaderHelper(helper, registry, emitError)))
      return emitError() << "invalid helper export " << symbol;

  llvm::DenseSet<Attribute> completed;
  llvm::DenseSet<Attribute> active;
  std::function<LogicalResult(Attribute)> visit =
      [&](Attribute symbol) -> LogicalResult {
    if (completed.contains(symbol))
      return success();
    if (!active.insert(symbol).second) {
      emitError() << "recursive source helper call graph is not supported";
      return failure();
    }
    auto helper = registry.lookupHelper(cast<FlatSymbolRefAttr>(symbol));
    for (Operation &operation : helper.getBody().front())
      if (auto call = dyn_cast<func::CallOp>(operation))
        if (failed(visit(call.getCalleeAttr())))
          return failure();
    active.erase(symbol);
    completed.insert(symbol);
    return success();
  };
  for (auto &[symbol, helper] : registry.helpers_)
    if (failed(visit(symbol)))
      return failure();

  for (ModuleOp header : registry.headers_) {
    DictionaryAttr owner =
        header->getAttrOfType<DictionaryAttr>("ac.source_owner");
    std::string module = moduleName(owner);
    auto exports = detail::readNamespaceExports(header, registry, emitError);
    if (failed(exports))
      return failure();
    for (const detail::NamespaceExportBinding &binding : *exports) {
      std::string key = exportKey(module, binding.name.getValue());
      if (!registry.exports_.try_emplace(key, binding.target).second)
        return emitError() << "duplicate namespace export '"
                           << binding.name.getValue() << "' in " << module;
    }
  }
  for (ModuleOp header : registry.headers_)
    if (failed(detail::verifyNamespaceImports(header, registry, emitError)))
      return failure();
  return registry;
}

FailureOr<ac::detail::ResolvedRecordView>
SourceHeaderRegistry::resolveRecord(FlatSymbolRefAttr symbol) const {
  auto found = records_.find(symbol);
  if (found == records_.end())
    return failure();
  ac::StructOp record = found->second;
  ac::detail::ResolvedRecordView view;
  view.symbol = symbol;
  for (Attribute rawField : record.getFields()) {
    auto field = dyn_cast<DictionaryAttr>(rawField);
    auto type = field ? field.getAs<DictionaryAttr>("type") : DictionaryAttr();
    if (!type)
      return failure();
    view.fieldLogicalTypes.push_back(type);
  }
  return view;
}

ac::TypeAliasOp
SourceHeaderRegistry::lookupAlias(FlatSymbolRefAttr symbol) const {
  auto found = aliases_.find(symbol);
  return found == aliases_.end() ? ac::TypeAliasOp() : found->second;
}

ac::StructOp
SourceHeaderRegistry::lookupRecord(FlatSymbolRefAttr symbol) const {
  auto found = records_.find(symbol);
  return found == records_.end() ? ac::StructOp() : found->second;
}

func::FuncOp
SourceHeaderRegistry::lookupHelper(FlatSymbolRefAttr symbol) const {
  auto found = helpers_.find(symbol);
  return found == helpers_.end() ? func::FuncOp() : found->second;
}

Operation *
SourceHeaderRegistry::lookupDeclaration(FlatSymbolRefAttr canonical) const {
  auto found = authorities_.find(canonical);
  return found == authorities_.end() ? nullptr : found->second;
}

ModuleInterfaceView
SourceHeaderRegistry::lookupModule(FlatSymbolRefAttr canonical) const {
  auto found = modules_.find(canonical);
  if (found == modules_.end())
    return {};
  ac::ModuleImportOp declaration = found->second;
  auto functionType =
      dyn_cast<FunctionType>(declaration.getFunctionTypeAttr().getValue());
  return {declaration,
          canonical,
          declaration.getSourceOwner(),
          declaration.getParameters(),
          functionType,
          declaration.getTypeParameters(),
          declaration.getInputNames(),
          declaration.getOutputNames(),
          declaration.getDependencySummary(),
          declaration.getPrimitiveKindAttr(),
          declaration->getAttrOfType<StringAttr>("ac.return_form"),
          declaration->getAttrOfType<ArrayAttr>("ac.parameters"),
          declaration->getAttrOfType<ArrayAttr>("ac.result_constraints"),
          declaration->getAttrOfType<DictionaryAttr>("ac.domain_inputs")};
}

ModuleInterfaceView SourceHeaderRegistry::lookupBuiltin(StringRef kind) const {
  auto found = builtins_.find(kind);
  if (found == builtins_.end())
    return {};
  ac::ModuleImportOp declaration = found->second;
  auto symbol = FlatSymbolRefAttr::get(declaration.getContext(),
                                       declaration.getSymName());
  auto functionType =
      dyn_cast<FunctionType>(declaration.getFunctionTypeAttr().getValue());
  return {declaration,
          symbol,
          declaration.getSourceOwner(),
          declaration.getParameters(),
          functionType,
          declaration.getTypeParameters(),
          declaration.getInputNames(),
          declaration.getOutputNames(),
          declaration.getDependencySummary(),
          declaration.getPrimitiveKindAttr(),
          declaration->getAttrOfType<StringAttr>("ac.return_form"),
          declaration->getAttrOfType<ArrayAttr>("ac.parameters"),
          declaration->getAttrOfType<ArrayAttr>("ac.result_constraints"),
          declaration->getAttrOfType<DictionaryAttr>("ac.domain_inputs")};
}

bool SourceHeaderRegistry::isTrustedBuiltin(
    ac::ModuleImportOp declaration) const {
  if (!declaration)
    return false;
  auto kind = declaration.getPrimitiveKindAttr();
  auto found = kind ? builtins_.find(kind.getValue()) : builtins_.end();
  return found != builtins_.end() &&
         detail::sameDeclaration(found->second, declaration);
}

LogicalResult SourceHeaderRegistry::verifyModuleImport(
    ac::ModuleImportOp declaration, ac::detail::EmitError emitError) const {
  if (!declaration)
    return emitError() << "module import is missing";
  auto symbol = FlatSymbolRefAttr::get(declaration.getContext(),
                                       declaration.getSymName());
  if (declaration.getPrimitiveKindAttr()) {
    ModuleInterfaceView builtin =
        lookupBuiltin(declaration.getPrimitiveKindAttr().getValue());
    if (!builtin || !detail::sameDeclaration(builtin.declaration, declaration))
      return emitError() << "primitive import is not the canonical catalog "
                            "declaration: "
                         << symbol;
    return success();
  }
  ModuleInterfaceView authority = lookupModule(symbol);
  if (!authority ||
      !detail::sameDeclaration(authority.declaration, declaration))
    return emitError() << "module import differs from its published authority: "
                       << symbol;
  return success();
}

DictionaryAttr SourceHeaderRegistry::ownerForModule(StringRef module) const {
  auto found = moduleOwners_.find(module);
  return found == moduleOwners_.end() ? DictionaryAttr() : found->second;
}

FlatSymbolRefAttr
SourceHeaderRegistry::lookupExport(StringRef module,
                                   StringRef sourceName) const {
  auto found = exports_.find(exportKey(module, sourceName));
  return found == exports_.end() ? FlatSymbolRefAttr() : found->second;
}

ArrayAttr SourceHeaderRegistry::interfacesForModule(StringRef module) const {
  auto found = interfaceClosures_.find(module);
  return found == interfaceClosures_.end() ? ArrayAttr() : found->second;
}

} // namespace acir::compiler
