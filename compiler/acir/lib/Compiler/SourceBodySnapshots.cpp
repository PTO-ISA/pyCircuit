#include "SourceHeaderHelpers.h"
#include "SourceUnit.h"

#include "mlir/IR/SymbolTable.h"
#include "llvm/ADT/STLExtras.h"

using namespace mlir;

namespace acir::compiler {
namespace {

LogicalResult verifyModulePortsAgainstHeader(ac::ModuleOp module,
                                             ac::ModuleImportOp declaration,
                                             ac::detail::EmitError emitError) {
  auto contract = declaration->getAttrOfType<DictionaryAttr>("ac.contract");
  auto parameters =
      contract ? contract.getAs<ArrayAttr>("parameters") : ArrayAttr();
  auto connections =
      contract ? contract.getAs<ArrayAttr>("connections") : ArrayAttr();
  auto actual = module->getAttrOfType<ArrayAttr>("ac.ports");
  if (!parameters || !connections || !actual)
    return emitError() << "source module/header port authority is incomplete";

  llvm::StringMap<DictionaryAttr> parameterByName;
  for (Attribute raw : parameters) {
    auto parameter = dyn_cast<DictionaryAttr>(raw);
    auto name = parameter ? parameter.getAs<StringAttr>("name") : StringAttr();
    auto category =
        parameter ? parameter.getAs<StringAttr>("category") : StringAttr();
    if (name && category && category.getValue() == "connection")
      parameterByName.try_emplace(name.getValue(), parameter);
  }

  SmallVector<Attribute> currentPorts;
  SmallVector<Attribute> nextPorts;
  Builder builder(module.getContext());
  for (Attribute raw : connections) {
    auto connection = dyn_cast<DictionaryAttr>(raw);
    auto name =
        connection ? connection.getAs<StringAttr>("parameter") : StringAttr();
    auto elements =
        connection ? connection.getAs<ArrayAttr>("elements") : ArrayAttr();
    auto found =
        name ? parameterByName.find(name.getValue()) : parameterByName.end();
    if (!name || !elements || found == parameterByName.end())
      return emitError()
             << "source module header has an unresolved connection contract";
    DictionaryAttr parameter = found->second;
    for (Attribute rawElement : elements) {
      auto element = dyn_cast<DictionaryAttr>(rawElement);
      auto read = element ? element.getAs<BoolAttr>("read") : BoolAttr();
      auto write = element ? element.getAs<BoolAttr>("write") : BoolAttr();
      if (!element || !read || !write)
        return emitError()
               << "source module header has a malformed connection effect";
      auto makePort = [&](StringRef role) -> Attribute {
        return builder.getDictionaryAttr({
            builder.getNamedAttr("parameter", name),
            builder.getNamedAttr("ordinal", element.get("ordinal")),
            builder.getNamedAttr("role", builder.getStringAttr(role)),
            builder.getNamedAttr("type", parameter.get("type")),
            builder.getNamedAttr("origin", parameter.get("origin")),
            builder.getNamedAttr("location", parameter.get("location")),
        });
      };
      if (read.getValue())
        currentPorts.push_back(makePort("current"));
      if (write.getValue())
        nextPorts.push_back(makePort("next"));
    }
  }
  SmallVector<Attribute> expected;
  llvm::append_range(expected, currentPorts);
  llvm::append_range(expected, nextPorts);
  if (actual != builder.getArrayAttr(expected))
    return emitError()
           << "source module ports differ from the owning header contract";
  return success();
}

} // namespace

LogicalResult SourceHeaderRegistry::verifyBodySnapshots(
    ModuleOp body, ModuleOp owningHeader,
    ac::detail::EmitError emitError) const {
  if (!body || !owningHeader)
    return emitError() << "source body and owning header are both required";
  SmallVector<ModuleOp> completeHeaders(headers_.begin(), headers_.end());
  completeHeaders.push_back(owningHeader);
  auto authority = SourceHeaderRegistry::create(completeHeaders, emitError);
  if (failed(authority))
    return failure();

  auto owner = owningHeader->getAttrOfType<DictionaryAttr>("ac.source_owner");
  auto bodyOwner = body->getAttrOfType<DictionaryAttr>("ac.source_owner");
  auto bodyKind = body->getAttrOfType<StringAttr>("ac.unit_kind");
  auto stage = body->getAttrOfType<StringAttr>("ac.stage");
  if (!owner || bodyOwner != owner || !stage || stage.getValue() != "source" ||
      !bodyKind ||
      (bodyKind.getValue() != "implementation" &&
       bodyKind.getValue() != "declarations"))
    return emitError()
           << "source body envelope does not match its owning header";

  for (StringRef name : {"ac.interfaces", "ac.exports", "ac.import_bindings"})
    if (!body->getAttr(name) ||
        body->getAttr(name) != owningHeader->getAttr(name))
      return emitError() << "source body/header metadata differs for " << name;

  bool hasExecutableModule = false;
  for (Operation &operation : body.getBody()->getOperations()) {
    if (auto module = dyn_cast<ac::ModuleOp>(operation)) {
      if (hasExecutableModule || bodyKind.getValue() != "implementation")
        return emitError() << "implementation body must own exactly one module";
      hasExecutableModule = true;
      auto symbol =
          module->getAttrOfType<StringAttr>(SymbolTable::getSymbolAttrName());
      auto canonical =
          symbol ? FlatSymbolRefAttr::get(body.getContext(), symbol.getValue())
                 : FlatSymbolRefAttr();
      Operation *declaration =
          canonical ? authority->lookupDeclaration(canonical) : nullptr;
      auto moduleDeclaration =
          dyn_cast_or_null<ac::ModuleImportOp>(declaration);
      if (!moduleDeclaration || declaration->getAttrOfType<DictionaryAttr>(
                                    "ac.source_owner") != owner)
        return emitError()
               << "source module has no matching owning header declaration";
      if (failed(verifyModulePortsAgainstHeader(module, moduleDeclaration,
                                                emitError)))
        return failure();
      continue;
    }

    auto role = operation.getAttrOfType<StringAttr>("ac.declaration_role");
    auto symbol =
        operation.getAttrOfType<StringAttr>(SymbolTable::getSymbolAttrName());
    auto snapshotOwner =
        operation.getAttrOfType<DictionaryAttr>("ac.source_owner");
    if (!role || role.getValue() != "import_snapshot" || !symbol ||
        !snapshotOwner)
      return emitError() << "source body contains an unverified declaration";
    auto canonical =
        FlatSymbolRefAttr::get(body.getContext(), symbol.getValue());
    Operation *definition = authority->lookupDeclaration(canonical);
    if (!definition ||
        definition->getAttrOfType<DictionaryAttr>("ac.source_owner") !=
            snapshotOwner ||
        !detail::sameDeclaration(definition, &operation))
      return emitError()
             << "source body import snapshot differs from its owning header: "
             << canonical;
  }
  if (hasExecutableModule != (bodyKind.getValue() == "implementation"))
    return emitError() << "source body kind disagrees with executable module";
  return success();
}

} // namespace acir::compiler
