#include "SourceHeaderHelpers.h"

#include "mlir/Dialect/Arith/IR/Arith.h"
#include "mlir/IR/OperationSupport.h"
#include "mlir/IR/SymbolTable.h"
#include "mlir/Interfaces/SideEffectInterfaces.h"
#include "llvm/ADT/STLExtras.h"
#include "llvm/ADT/StringSet.h"

using namespace mlir;

namespace acir::compiler::detail {
namespace {

Type physicalType(DictionaryAttr logical, MLIRContext *context,
                  ac::detail::EmitError emitError) {
  auto kind = logical ? logical.getAs<StringAttr>("kind") : StringAttr();
  if (!kind)
    return (emitError() << "logical type requires a StringAttr kind"), Type();
  if (kind.getValue() == "bool")
    return IntegerType::get(context, 1);
  if (kind.getValue() == "integer") {
    auto storage = logical.getAs<TypeAttr>("storage");
    if (storage)
      return storage.getValue();
  } else if (kind.getValue() == "record") {
    auto symbol = logical.getAs<FlatSymbolRefAttr>("symbol");
    if (symbol)
      return ac::StructType::get(context,
                                 StringAttr::get(context, symbol.getValue()));
  }
  emitError() << "unsupported U01 logical type representation";
  return {};
}

} // namespace

LogicalResult verifyOriginDefinition(DictionaryAttr origin,
                                     FlatSymbolRefAttr expected,
                                     StringRef description,
                                     ac::detail::EmitError emitError) {
  auto site = origin ? origin.getAs<DictionaryAttr>("site") : DictionaryAttr();
  auto definition =
      site ? site.getAs<FlatSymbolRefAttr>("definition") : FlatSymbolRefAttr();
  if (!expected || !definition || definition != expected)
    return emitError()
           << description
           << " occurrence definition does not match its canonical symbol";
  return success();
}

LogicalResult verifySourcePath(DictionaryAttr sourceSpan, DictionaryAttr owner,
                               StringRef description,
                               ac::detail::EmitError emitError) {
  auto sourcePath =
      sourceSpan ? sourceSpan.getAs<StringAttr>("path") : StringAttr();
  auto ownerPath = owner ? owner.getAs<StringAttr>("path") : StringAttr();
  if (!sourcePath || !ownerPath || sourcePath != ownerPath)
    return emitError()
           << description
           << " SourceSpan.path does not match its original SourceOwner.path";
  return success();
}

LogicalResult
verifySnapshotRecordFieldLocations(ac::StructOp record, DictionaryAttr owner,
                                   ac::detail::EmitError emitError) {
  auto fields = record->getAttrOfType<ArrayAttr>("fields");
  if (!fields)
    return emitError() << "record snapshot requires an ArrayAttr fields";
  for (auto [index, rawField] : llvm::enumerate(fields)) {
    auto field = dyn_cast<DictionaryAttr>(rawField);
    auto location =
        field ? field.getAs<DictionaryAttr>("location") : DictionaryAttr();
    if (!field || field.size() != 4 || !location ||
        failed(ac::detail::verifySourceSpan(location, emitError)) ||
        failed(verifySourcePath(location, owner, "record field snapshot",
                                emitError)))
      return emitError() << "invalid source location in record snapshot field["
                         << index << "]";
  }
  return success();
}

LogicalResult verifyHeaderHelper(func::FuncOp helper,
                                 const SourceHeaderRegistry &registry,
                                 ac::detail::EmitError emitError) {
  if (helper.isExternal() || helper.getBody().getBlocks().size() != 1)
    return emitError() << "header helper must have one executable body block";
  auto helperKind = helper->getAttrOfType<StringAttr>("ac.helper_kind");
  auto parameters = helper->getAttrOfType<ArrayAttr>("ac.parameters");
  auto returnForm = helper->getAttrOfType<StringAttr>("ac.return_form");
  auto constraints = helper->getAttrOfType<ArrayAttr>("ac.result_constraints");
  auto checks = helper->getAttrOfType<ArrayAttr>("ac.check_templates");
  if (!helperKind ||
      (helperKind.getValue() != "record_constructor" &&
       helperKind.getValue() != "value") ||
      !parameters || !returnForm || !constraints || !checks)
    return emitError() << "header helper contract metadata is incomplete";
  if (!checks.empty())
    return emitError() << "U01 helper check templates are not supported yet";
  if (returnForm.getValue() != "single" || constraints.size() != 1)
    return emitError() << "U01 header helper supports one declared data result";

  auto resolver = [&](FlatSymbolRefAttr symbol) {
    return registry.resolveRecord(symbol);
  };
  SmallVector<Type> expectedInputs;
  llvm::StringSet<> parameterNames;
  unsigned previousBindingRank = 0;
  bool hasPreviousBinding = false;
  bool positionalDefaultSeen = false;
  for (auto [index, raw] : llvm::enumerate(parameters)) {
    auto parameter = dyn_cast<DictionaryAttr>(raw);
    if (!parameter || parameter.size() != 6)
      return emitError() << "helper parameter[" << index
                         << "] must contain exactly six fields";
    auto name = parameter.getAs<StringAttr>("name");
    auto binding = parameter.getAs<StringAttr>("binding");
    auto constraint = parameter.getAs<DictionaryAttr>("constraint");
    auto defaultValue = parameter.getAs<DictionaryAttr>("default");
    auto origin = parameter.getAs<DictionaryAttr>("origin");
    auto location = parameter.getAs<DictionaryAttr>("location");
    auto constraintKind =
        constraint ? constraint.getAs<StringAttr>("kind") : StringAttr();
    unsigned bindingRank = !binding                                        ? 3
                           : binding.getValue() == "positional_only"       ? 0
                           : binding.getValue() == "positional_or_keyword" ? 1
                           : binding.getValue() == "keyword_only"          ? 2
                                                                           : 3;
    if (!name || !parameterNames.insert(name.getValue()).second ||
        bindingRank == 3 ||
        (hasPreviousBinding && bindingRank < previousBindingRank) ||
        !constraint || constraint.size() != 2 || !constraintKind ||
        constraintKind.getValue() != "logical")
      return emitError() << "helper parameter[" << index
                         << "] name, binding order, or constraint is invalid";
    previousBindingRank = bindingRank;
    hasPreviousBinding = true;
    auto type = constraint.getAs<DictionaryAttr>("type");
    if (!type ||
        failed(ac::detail::verifyDefaultMatchesType(
            defaultValue, type, ac::detail::ExpectedTypeKind::Logical, resolver,
            emitError)) ||
        failed(ac::detail::verifyOccurrence(origin, emitError)) ||
        failed(ac::detail::verifySourceSpan(location, emitError)) ||
        failed(verifySourcePath(
            location, helper->getAttrOfType<DictionaryAttr>("ac.source_owner"),
            "helper parameter", emitError)))
      return emitError() << "helper parameter[" << index
                         << "] type, default, or source metadata is invalid";
    auto defaultPresent = defaultValue.getAs<BoolAttr>("present");
    if (!defaultPresent || (bindingRank < 2 && !defaultPresent.getValue() &&
                            positionalDefaultSeen))
      return emitError() << "helper positional defaults must form a suffix";
    if (bindingRank < 2 && defaultPresent.getValue())
      positionalDefaultSeen = true;
    auto helperName =
        helper->getAttrOfType<StringAttr>(SymbolTable::getSymbolAttrName());
    if (failed(verifyOriginDefinition(
            origin,
            helperName ? FlatSymbolRefAttr::get(helper.getContext(),
                                                helperName.getValue())
                       : FlatSymbolRefAttr(),
            "helper parameter", emitError)))
      return failure();
    Type input = physicalType(type, helper.getContext(), emitError);
    if (!input)
      return failure();
    expectedInputs.push_back(input);
  }

  auto resultConstraint = dyn_cast<DictionaryAttr>(constraints[0]);
  auto resultKind = resultConstraint
                        ? resultConstraint.getAs<StringAttr>("kind")
                        : StringAttr();
  auto resultType = resultConstraint
                        ? resultConstraint.getAs<DictionaryAttr>("type")
                        : DictionaryAttr();
  if (!resultConstraint || resultConstraint.size() != 2 || !resultKind ||
      resultKind.getValue() != "logical" || !resultType ||
      failed(ac::detail::verifyTypeResolved(
          resultType, ac::detail::ExpectedTypeKind::Logical, resolver,
          emitError)))
    return emitError() << "header helper result constraint is invalid";
  Type result = physicalType(resultType, helper.getContext(), emitError);
  if (!result)
    return failure();

  expectedInputs.push_back(IntegerType::get(helper.getContext(), 1));
  FunctionType functionType = helper.getFunctionType();
  if (functionType.getNumInputs() != expectedInputs.size() ||
      !llvm::equal(functionType.getInputs(), expectedInputs) ||
      functionType.getNumResults() != 2 ||
      functionType.getResult(0) != result ||
      !functionType.getResult(1).isInteger(1))
    return emitError() << "helper physical signature does not match "
                          "parameters, evaluation_path, result, and valid";

  if (helperKind.getValue() == "record_constructor") {
    auto record = helper->getAttrOfType<FlatSymbolRefAttr>("ac.record");
    auto resultRecord = resultType.getAs<FlatSymbolRefAttr>("symbol");
    if (!record || !resultRecord || record != resultRecord)
      return emitError() << "record constructor nominal result mismatch";
  } else if (helper->hasAttr("ac.record")) {
    return emitError() << "value helper must not carry ac.record";
  }

  Block &entry = helper.getBody().front();
  for (Operation &operation : entry) {
    if (operation.getNumRegions() != 0)
      return emitError()
             << "nested regions are not supported in source helper bodies: "
             << operation.getName();
    if (isa<func::ReturnOp, arith::ConstantOp, ac::StructCreateOp,
            ac::StructGetOp, ac::MathConstantOp, ac::MathFromBitsOp,
            ac::MathBinaryOp, ac::MathToBitsOp>(operation))
      continue;
    if (auto call = dyn_cast<func::CallOp>(operation)) {
      auto target = registry.lookupHelper(call.getCalleeAttr());
      if (!target || target.isExternal() ||
          target.getFunctionType() != call.getCalleeType() ||
          call.getNumOperands() == 0 ||
          !call.getOperand(call.getNumOperands() - 1).getType().isInteger(1) ||
          call.getNumResults() != 2 ||
          !call.getResult(1).getType().isInteger(1))
        return emitError()
               << "helper calls an unresolved or incompatible helper";
      continue;
    }
    if (operation.getDialect() &&
        operation.getDialect()->getNamespace() == "arith" &&
        isMemoryEffectFree(&operation))
      continue;
    return emitError() << "unsupported operation in U01 helper body: "
                       << operation.getName();
  }
  auto returned = dyn_cast<func::ReturnOp>(helper.getBody().front().back());
  if (!returned || returned.getNumOperands() != 2 ||
      returned.getOperand(0).getType() != result ||
      !returned.getOperand(1).getType().isInteger(1))
    return emitError() << "helper must return one declared value and valid";
  if (helperKind.getValue() == "record_constructor" &&
      !isa<ac::StructType>(result))
    return emitError() << "record constructor result must be a nominal record";
  if (helperKind.getValue() == "record_constructor") {
    auto created = returned.getOperand(0).getDefiningOp<ac::StructCreateOp>();
    if (!created || created.getResult().getType() != result)
      return emitError()
             << "record constructor must return its constructed record";
  }
  return success();
}

Attribute normalizedAttribute(Attribute attribute, StringRef key = {}) {
  if (key == "location" || key == "loc") {
    auto span = dyn_cast<DictionaryAttr>(attribute);
    auto path = span ? span.getAs<StringAttr>("path") : StringAttr();
    if (!span || !path)
      return attribute;
    SmallVector<NamedAttribute> fields{
        NamedAttribute(StringAttr::get(attribute.getContext(), "path"), path)};
    return DictionaryAttr::get(attribute.getContext(), fields);
  }
  if (auto dictionary = dyn_cast<DictionaryAttr>(attribute)) {
    SmallVector<NamedAttribute> fields;
    for (NamedAttribute field : dictionary) {
      StringRef name = field.getName().getValue();
      Attribute value = normalizedAttribute(field.getValue(), name);
      if (name == "ac.declaration_role")
        value = StringAttr::get(attribute.getContext(), "definition");
      fields.push_back(
          NamedAttribute(field.getName(), value ? value : field.getValue()));
    }
    return DictionaryAttr::get(attribute.getContext(), fields);
  }
  if (auto array = dyn_cast<ArrayAttr>(attribute)) {
    SmallVector<Attribute> values;
    for (Attribute value : array)
      values.push_back(normalizedAttribute(value));
    return ArrayAttr::get(attribute.getContext(), values);
  }
  return attribute;
}

bool sameDeclaration(Operation *definition, Operation *snapshot) {
  if (definition->getName() != snapshot->getName() ||
      definition->getAttrs().size() != snapshot->getAttrs().size() ||
      normalizedAttribute(definition->getPropertiesAsAttribute()) !=
          normalizedAttribute(snapshot->getPropertiesAsAttribute()))
    return false;
  auto left = cast<DictionaryAttr>(normalizedAttribute(
      DictionaryAttr::get(definition->getContext(), definition->getAttrs())));
  auto right = cast<DictionaryAttr>(normalizedAttribute(
      DictionaryAttr::get(snapshot->getContext(), snapshot->getAttrs())));
  if (left != right || definition->getNumRegions() != snapshot->getNumRegions())
    return false;
  for (auto [leftRegion, rightRegion] :
       llvm::zip(definition->getRegions(), snapshot->getRegions()))
    if (!OperationEquivalence::isRegionEquivalentTo(
            &leftRegion, &rightRegion, OperationEquivalence::IgnoreLocations))
      return false;
  return true;
}

} // namespace acir::compiler::detail
