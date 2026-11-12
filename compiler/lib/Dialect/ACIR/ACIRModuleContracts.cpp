#include "ACIRSourceContracts.h"
#include "mlir/IR/SymbolTable.h"
#include "pycircuit/Dialect/ACIR/ACIROps.h"
#include "llvm/ADT/APSInt.h"
#include "llvm/ADT/DenseSet.h"
#include "llvm/ADT/STLExtras.h"
#include "llvm/ADT/ScopeExit.h"
#include <functional>
using namespace mlir;
namespace acir::ac {
DictionaryAttr getStaticParameterAttr(Builder &builder, StringRef name,
                                      StaticExprAttr defaultValue) {
  NamedAttrList fields;
  fields.append("name", builder.getStringAttr(name));
  fields.append("type", TypeAttr::get(MathIntType::get(builder.getContext())));
  if (defaultValue)
    fields.append("default", defaultValue);
  return fields.getDictionary(builder.getContext());
}
DictionaryAttr getOutputDependencyAttr(Builder &builder, unsigned output,
                                       ArrayRef<unsigned> inputs) {
  auto endpoint = [&](unsigned port) {
    return builder.getDictionaryAttr(
        {builder.getNamedAttr("port", builder.getI64IntegerAttr(port)),
         builder.getNamedAttr("path", builder.getArrayAttr({}))});
  };
  SmallVector<Attribute> deps;
  for (unsigned port : inputs)
    deps.push_back(endpoint(port));
  return builder.getDictionaryAttr(
      {builder.getNamedAttr("output", endpoint(output)),
       builder.getNamedAttr("inputs", builder.getArrayAttr(deps))});
}
} // namespace acir::ac

namespace acir::ac::detail {
LogicalResult verifyModuleImportSummarySelection(ModuleImportOp imported) {
  auto kind = imported->getAttrOfType<StringAttr>("semantic_kind");
  if (!kind || (kind.getValue() != "hardware" &&
                kind.getValue() != "transactions"))
    return imported.emitOpError()
           << "semantic_kind must be hardware or transactions";
  if (Attribute raw = imported->getAttr("ac.external_contract")) {
    auto contract = dyn_cast<DictionaryAttr>(raw);
    auto matches = [&](StringRef key, StringRef expected) {
      auto value = contract ? contract.getAs<StringAttr>(key) : StringAttr();
      return value && value.getValue() == expected;
    };
    auto effects = contract ? contract.getAs<ArrayAttr>("effects") : ArrayAttr();
    auto latency = contract ? contract.getAs<IntegerAttr>("latency")
                            : IntegerAttr();
    if (kind.getValue() != "hardware" ||
        imported->hasAttr("primitive_kind") ||
        imported->hasAttr("transaction_summary") ||
        imported.getSymName().starts_with("pycircuit.__builtins__."))
      return imported.emitOpError()
             << "ac.external_contract requires an ordinary hardware import "
                "without primitive_kind or transaction_summary";
    if (!contract || contract.size() != 6 || !matches("state", "none") ||
        !effects || !effects.empty() || !latency ||
        !latency.getType().isSignlessInteger(64) ||
        !latency.getValue().isZero() ||
        !matches("availability", "all_inputs") ||
        !matches("value_domain", "four_state") || !matches("failure", "none"))
      return imported.emitOpError()
             << "ac.external_contract requires exactly state = none, effects = "
                "[], latency = 0 : i64, availability = all_inputs, value_domain "
                "= four_state and failure = none";
  }
  if (kind.getValue() == "hardware") {
    if (!imported->getAttrOfType<ArrayAttr>("dependency_summary") ||
        imported->hasAttr("transaction_summary"))
      return imported.emitOpError()
             << "hardware import requires dependency_summary and forbids "
                "transaction_summary";
  } else if (!imported->getAttrOfType<DictionaryAttr>("transaction_summary") ||
             imported->hasAttr("dependency_summary") ||
             imported->hasAttr("primitive_kind")) {
    return imported.emitOpError()
           << "transactions import requires transaction_summary and forbids "
              "dependency_summary and primitive_kind";
  }
  return success();
}

LogicalResult verifyRequiredSourceParameter(Attribute raw,
                                            StringAttr expectedName,
                                            DictionaryAttr sourceOwner,
                                            FlatSymbolRefAttr definition,
                                            Operation *diagnosticOperation,
                                            StringRef description) {
  auto error = [&] { return diagnosticOperation->emitOpError(); };
  auto parameter = dyn_cast_or_null<DictionaryAttr>(raw);
  auto name = parameter ? parameter.getAs<StringAttr>("name") : StringAttr();
  auto binding =
      parameter ? parameter.getAs<StringAttr>("binding") : StringAttr();
  auto defaultValue =
      parameter ? parameter.getAs<DictionaryAttr>("default") : DictionaryAttr();
  auto origin =
      parameter ? parameter.getAs<DictionaryAttr>("origin") : DictionaryAttr();
  auto location = parameter ? parameter.getAs<DictionaryAttr>("location")
                            : DictionaryAttr();
  if (!parameter || parameter.size() != 6 || !name || !expectedName ||
      name != expectedName || !binding ||
      binding.getValue() != "positional_or_keyword" ||
      failed(verifyDefaultStructure(defaultValue, error)) ||
      defaultValue.getAs<BoolAttr>("present").getValue())
    return error() << description
                   << " parameters require matching names, "
                      "positional-or-keyword binding and absent defaults";
  if (!definition || failed(verifySourceOwner(sourceOwner, error)))
    return error() << description
                   << " requires a canonical definition and valid SourceOwner";
  if (failed(verifyOccurrence(origin, error)) ||
      failed(verifyOriginDefinition(origin, definition, description,
                                    error)) ||
      failed(verifySourceSpan(location, error)))
    return failure();
  if (location.getAs<StringAttr>("path") !=
      sourceOwner.getAs<StringAttr>("path"))
    return error() << description
                   << " parameter path differs from "
                      "its original SourceOwner";
  return success();
}

LogicalResult verifyRequiredSourceParameter(Attribute raw,
                                            StringAttr expectedName,
                                            Operation *lexicalModule,
                                            Operation *diagnosticOperation,
                                            StringRef description) {
  auto symbol = lexicalModule->getAttrOfType<StringAttr>(
      SymbolTable::getSymbolAttrName());
  return verifyRequiredSourceParameter(
      raw, expectedName,
      lexicalModule->getAttrOfType<DictionaryAttr>("source_owner"),
      symbol ? FlatSymbolRefAttr::get(lexicalModule->getContext(),
                                     symbol.getValue())
             : FlatSymbolRefAttr(),
      diagnosticOperation, description);
}

} // namespace acir::ac::detail
