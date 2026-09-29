#include "SourceHeaderHelpers.h"
#include "SourceNamespace.h"
#include "SourceUnit.h"

#include "mlir/IR/OperationSupport.h"
#include "mlir/IR/SymbolTable.h"
#include "mlir/IR/Verifier.h"
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

bool isInOwnerModule(StringRef symbol, StringRef module) {
  return module.empty() ? !symbol.empty()
                        : symbol.starts_with((Twine(module) + ".").str());
}

bool hasQualifiedDeclarationIdentity(Operation *operation, StringRef symbol,
                                     StringRef module) {
  if (!isInOwnerModule(symbol, module))
    return false;
  StringRef local =
      module.empty() ? symbol : symbol.drop_front(module.size() + 1);
  if (isa<ac::TypeAliasOp, ac::ConstantOp, ac::StructOp, ac::ModuleImportOp>(
          operation))
    return !local.empty() && !local.contains('.');
  if (isa<func::FuncOp>(operation)) {
    if (!local.contains('.'))
      return !local.empty();
    StringRef parent, leaf;
    std::tie(parent, leaf) = local.rsplit('.');
    return !parent.empty() && !parent.contains('.') && leaf == "__init__";
  }
  return false;
}

std::string exportKey(StringRef module, StringRef name) {
  return (Twine(module) + "::" + name).str();
}

bool ownerLess(DictionaryAttr left, DictionaryAttr right) {
  StringRef leftPackage = left.getAs<StringAttr>("package").getValue();
  StringRef rightPackage = right.getAs<StringAttr>("package").getValue();
  if (leftPackage != rightPackage)
    return leftPackage < rightPackage;
  return left.getAs<StringAttr>("path").getValue() <
         right.getAs<StringAttr>("path").getValue();
}

LogicalResult verifyUnitEnvelope(ModuleOp header, DictionaryAttr &owner,
                                 ac::detail::EmitError emitError) {
  owner = header->getAttrOfType<DictionaryAttr>("ac.source_owner");
  auto unitKind = header->getAttrOfType<StringAttr>("ac.unit_kind");
  auto stage = header->getAttrOfType<StringAttr>("ac.stage");
  auto interfaces = header->getAttrOfType<ArrayAttr>("ac.interfaces");
  auto exports = header->getAttrOfType<ArrayAttr>("ac.exports");
  auto importBindings = header->getAttrOfType<ArrayAttr>("ac.import_bindings");
  if (failed(ac::detail::verifySourceOwner(owner, emitError)) || !unitKind ||
      unitKind.getValue() != "interface" || !stage ||
      stage.getValue() != "source" || !interfaces || interfaces.empty())
    return emitError() << "interface unit envelope is incomplete";
  if (!exports)
    return emitError() << "interface requires ArrayAttr ac.exports";
  if (!importBindings)
    return emitError() << "interface requires ArrayAttr ac.import_bindings";
  llvm::DenseSet<Attribute> unique;
  DictionaryAttr previous;
  for (auto [index, raw] : llvm::enumerate(interfaces)) {
    auto dependency = dyn_cast<DictionaryAttr>(raw);
    if (!dependency ||
        failed(ac::detail::verifySourceOwner(dependency, emitError)))
      return emitError() << "ac.interfaces[" << index
                         << "] is not a valid SourceOwner";
    if (!unique.insert(dependency).second)
      return emitError() << "ac.interfaces contains a duplicate SourceOwner";
    if (index == 0) {
      if (dependency != owner)
        return emitError()
               << "ac.interfaces must begin with the interface unit owner";
      continue;
    }
    if (dependency == owner || (previous && !ownerLess(previous, dependency)))
      return emitError()
             << "ac.interfaces dependencies must be structurally ordered";
    previous = dependency;
  }
  return success();
}

LogicalResult verifyCommonDeclaration(Operation *operation,
                                      DictionaryAttr enclosingOwner,
                                      ac::detail::EmitError emitError) {
  auto declarationOwner =
      operation->getAttrOfType<DictionaryAttr>("ac.source_owner");
  auto origin = operation->getAttrOfType<DictionaryAttr>("ac.origin");
  auto role = operation->getAttrOfType<StringAttr>("ac.declaration_role");
  if (failed(ac::detail::verifySourceOwner(declarationOwner, emitError)) ||
      failed(ac::detail::verifyOccurrence(origin, emitError)))
    return failure();
  StringRef name = symbolName(operation);
  if (name.empty() ||
      failed(detail::verifyOriginDefinition(
          origin, FlatSymbolRefAttr::get(operation->getContext(), name),
          "declaration", emitError)))
    return failure();
  if (!role ||
      (role.getValue() != "definition" && role.getValue() != "import_snapshot"))
    return emitError() << "exported declaration role is invalid";
  if (role.getValue() == "definition" && declarationOwner != enclosingOwner)
    return emitError() << "definition SourceOwner does not match its interface";
  if (role.getValue() == "import_snapshot") {
    auto file = operation->getParentOfType<ModuleOp>();
    auto interfaces =
        file ? file->getAttrOfType<ArrayAttr>("ac.interfaces") : ArrayAttr();
    if (!interfaces || !llvm::is_contained(interfaces, declarationOwner))
      return emitError()
             << "import snapshot SourceOwner is absent from ac.interfaces";
  }
  return success();
}

} // namespace

FailureOr<SourceHeaderRegistry>
SourceHeaderRegistry::create(ArrayRef<ModuleOp> headers,
                             ac::detail::EmitError emitError) {
  SourceHeaderRegistry registry;
  llvm::DenseMap<Attribute, ModuleOp> ownerHeaders;
  SmallVector<Operation *> snapshots;
  for (ModuleOp header : headers) {
    DictionaryAttr owner;
    if (failed(verifyUnitEnvelope(header, owner, emitError)) ||
        failed(verify(header)))
      return failure();
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
    DictionaryAttr enclosingOwner =
        header->getAttrOfType<DictionaryAttr>("ac.source_owner");
    for (Operation &operation : header.getBody()->getOperations()) {
      auto symbol = dyn_cast<SymbolOpInterface>(&operation);
      if (!symbol)
        return emitError() << "interface contains a non-symbol operation";
      StringRef name = symbolName(&operation);
      auto role = operation.getAttrOfType<StringAttr>("ac.declaration_role");
      auto declarationOwner =
          operation.getAttrOfType<DictionaryAttr>("ac.source_owner");
      DictionaryAttr identityOwner =
          role && role.getValue() == "import_snapshot" ? declarationOwner
                                                       : enclosingOwner;
      std::string declarationModule = moduleName(identityOwner);
      if (name.empty() ||
          !hasQualifiedDeclarationIdentity(&operation, name, declarationModule))
        return emitError() << "declaration qualified identity does not match "
                              "its SourceOwner: "
                           << name;
      if (failed(
              verifyCommonDeclaration(&operation, enclosingOwner, emitError)))
        return failure();
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
      else if (isa<ac::ConstantOp>(operation))
        continue;
      else if (auto record = dyn_cast<ac::StructOp>(operation))
        registry.records_.try_emplace(flat, record);
      else if (auto helper = dyn_cast<func::FuncOp>(operation))
        registry.helpers_.try_emplace(flat, helper);
      else if (isa<ac::ModuleImportOp>(operation))
        continue;
      else
        return emitError() << "interface contains an unsupported declaration";
    }
  }

  for (Operation *snapshot : snapshots) {
    auto owner = snapshot->getAttrOfType<DictionaryAttr>("ac.source_owner");
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
    } else if (auto record = dyn_cast<ac::StructOp>(snapshot)) {
      if (failed(detail::verifySnapshotRecordFieldLocations(record, owner,
                                                            emitError)))
        return failure();
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
  for (auto &[symbol, record] : registry.records_) {
    auto recordRef = dyn_cast<FlatSymbolRefAttr>(symbol);
    auto recordOwner = record->getAttrOfType<DictionaryAttr>("ac.source_owner");
    auto fields = record->getAttrOfType<ArrayAttr>("fields");
    auto constructorAttr =
        record->getAttrOfType<FlatSymbolRefAttr>("constructor");
    if (!recordRef || !fields || !constructorAttr)
      return emitError()
             << "record declaration requires fields and constructor";
    llvm::StringSet<> fieldNames;
    for (auto [index, raw] : llvm::enumerate(fields)) {
      auto field = dyn_cast<DictionaryAttr>(raw);
      if (!field || field.size() != 4 || !field.getAs<StringAttr>("name") ||
          !fieldNames.insert(field.getAs<StringAttr>("name").getValue())
               .second ||
          failed(ac::detail::verifyTypeResolved(
              field.getAs<DictionaryAttr>("type"),
              ac::detail::ExpectedTypeKind::Logical, resolver, emitError)) ||
          failed(ac::detail::verifyOccurrence(
              field.getAs<DictionaryAttr>("origin"), emitError)) ||
          failed(ac::detail::verifySourceSpan(
              field.getAs<DictionaryAttr>("location"), emitError)))
        return emitError() << "invalid record field[" << index << "] in "
                           << symbol;
      if (failed(detail::verifyOriginDefinition(
              field.getAs<DictionaryAttr>("origin"), recordRef, "record field",
              emitError)))
        return failure();
      if (failed(
              detail::verifySourcePath(field.getAs<DictionaryAttr>("location"),
                                       recordOwner, "record field", emitError)))
        return failure();
    }
    auto constructor = registry.lookupHelper(constructorAttr);
    if (!constructor)
      return emitError()
             << "record constructor is absent from explicit headers: "
             << constructorAttr;
    auto declaredRecord =
        constructor->getAttrOfType<FlatSymbolRefAttr>("ac.record");
    if (!recordRef || !declaredRecord || declaredRecord != recordRef)
      return emitError() << "record constructor nominal result mismatch for "
                         << symbol;
    Type expected = ac::StructType::get(
        record.getContext(),
        StringAttr::get(record.getContext(), recordRef.getValue()));
    FunctionType functionType = constructor.getFunctionType();
    if (functionType.getNumResults() != 2 ||
        functionType.getResult(0) != expected ||
        !functionType.getResult(1).isInteger(1))
      return emitError() << "record constructor physical return is invalid for "
                         << symbol;
  }

  for (auto &[symbol, declaration] : registry.authorities_) {
    auto module = dyn_cast<ac::ModuleImportOp>(declaration);
    if (!module)
      continue;
    auto contract = module->getAttrOfType<DictionaryAttr>("ac.contract");
    auto parameters =
        contract ? contract.getAs<ArrayAttr>("parameters") : ArrayAttr();
    if (!parameters)
      return emitError() << "module import has no verified parameters "
                         << symbol;
    for (auto [index, raw] : llvm::enumerate(parameters)) {
      auto parameter = dyn_cast<DictionaryAttr>(raw);
      auto category =
          parameter ? parameter.getAs<StringAttr>("category") : StringAttr();
      auto type = parameter ? parameter.getAs<DictionaryAttr>("type")
                            : DictionaryAttr();
      auto defaultValue = parameter ? parameter.getAs<DictionaryAttr>("default")
                                    : DictionaryAttr();
      if (!category || !type || !defaultValue)
        return emitError() << "invalid module parameter[" << index << "] in "
                           << symbol;
      auto kind = category.getValue() == "static"
                      ? ac::detail::ExpectedTypeKind::Static
                      : ac::detail::ExpectedTypeKind::Logical;
      if (failed(ac::detail::verifyTypeResolved(type, kind, resolver,
                                                emitError)) ||
          failed(ac::detail::verifyDefaultMatchesType(defaultValue, type, kind,
                                                      resolver, emitError)))
        return emitError() << "unresolved module parameter[" << index
                           << "] type/default in " << symbol;
    }
  }

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
      emitError() << "recursive U01 helper call graph is not supported";
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
