#include "acir/Dialect/ACIR/ACIROps.h"

#include "Dialect/ACIR/ACIRSourceContracts.h"
#include "mlir/IR/BuiltinOps.h"
#include "mlir/IR/SymbolTable.h"

using namespace mlir;

namespace acir::ac {
namespace {

LogicalResult verifyDeclarationMetadata(Operation *operation,
                                        DictionaryAttr owner,
                                        DictionaryAttr origin,
                                        StringAttr role) {
  auto emitError = [&] { return operation->emitOpError(); };
  if (failed(detail::verifySourceOwner(owner, emitError)) ||
      failed(detail::verifyOccurrence(origin, emitError)))
    return failure();
  if (!role ||
      (role.getValue() != "definition" && role.getValue() != "import_snapshot"))
    return operation->emitOpError()
           << "declaration_role must be 'definition' or 'import_snapshot'";
  return success();
}

FailureOr<Type> physicalType(DictionaryAttr logical, Operation *owner) {
  auto kind = logical.getAs<StringAttr>("kind");
  if (!kind)
    return failure();
  if (kind.getValue() == "bool")
    return IntegerType::get(owner->getContext(), 1);
  if (kind.getValue() == "integer") {
    auto storage = logical.getAs<TypeAttr>("storage");
    if (!storage)
      return failure();
    return storage.getValue();
  }
  if (kind.getValue() == "record") {
    auto symbol = logical.getAs<FlatSymbolRefAttr>("symbol");
    if (!symbol)
      return failure();
    return Type(StructType::get(
        owner->getContext(),
        StringAttr::get(owner->getContext(), symbol.getValue())));
  }
  return failure();
}

StructOp lookupStruct(Operation *operation, StructType type) {
  auto file = operation->getParentOfType<ModuleOp>();
  if (!file)
    return {};
  auto symbol = FlatSymbolRefAttr::get(operation->getContext(), type.getName());
  return dyn_cast_or_null<StructOp>(SymbolTable::lookupSymbolIn(file, symbol));
}

} // namespace

LogicalResult TypeAliasOp::verify() {
  if (failed(verifyDeclarationMetadata(
          *this, (*this)->getAttrOfType<DictionaryAttr>("ac.source_owner"),
          (*this)->getAttrOfType<DictionaryAttr>("ac.origin"),
          (*this)->getAttrOfType<StringAttr>("ac.declaration_role"))))
    return failure();
  auto emitError = [&] { return emitOpError(); };
  return detail::verifyLogicalTypeStructure(getTarget(), emitError);
}

LogicalResult StructOp::verify() {
  if (failed(verifyDeclarationMetadata(
          *this, (*this)->getAttrOfType<DictionaryAttr>("ac.source_owner"),
          (*this)->getAttrOfType<DictionaryAttr>("ac.origin"),
          (*this)->getAttrOfType<StringAttr>("ac.declaration_role"))))
    return failure();
  auto emitError = [&] { return emitOpError(); };
  for (auto [index, rawField] : llvm::enumerate(getFields())) {
    auto field = dyn_cast<DictionaryAttr>(rawField);
    if (!field || field.size() != 4)
      return emitOpError() << "field[" << index
                           << "] must be a four-field DictionaryAttr";
    if (!field.getAs<StringAttr>("name"))
      return emitOpError() << "field[" << index << "] name must be StringAttr";
    auto type = field.getAs<DictionaryAttr>("type");
    auto origin = field.getAs<DictionaryAttr>("origin");
    auto location = field.getAs<DictionaryAttr>("location");
    if (!type || failed(detail::verifyLogicalTypeStructure(type, emitError)) ||
        !origin || failed(detail::verifyOccurrence(origin, emitError)) ||
        !location || failed(detail::verifySourceSpan(location, emitError)))
      return emitOpError() << "field[" << index
                           << "] has invalid type/origin/location";
  }
  return success();
}

LogicalResult StructCreateOp::verify() {
  StructOp declaration = lookupStruct(*this, getResult().getType());
  if (!declaration)
    return emitOpError() << "cannot resolve nominal struct declaration";
  if (getValues().size() != declaration.getFields().size())
    return emitOpError() << "struct.create field arity mismatch";
  for (auto [index, pair] :
       llvm::enumerate(llvm::zip(getValues(), declaration.getFields()))) {
    auto field = dyn_cast<DictionaryAttr>(std::get<1>(pair));
    auto logical =
        field ? field.getAs<DictionaryAttr>("type") : DictionaryAttr();
    if (!field || !logical)
      return emitOpError() << "referenced struct field[" << index
                           << "] metadata is malformed";
    auto expected = physicalType(logical, *this);
    if (failed(expected) || std::get<0>(pair).getType() != *expected)
      return emitOpError() << "struct.create field[" << index
                           << "] physical type mismatch";
  }
  return success();
}

LogicalResult StructGetOp::verify() {
  StructOp declaration = lookupStruct(*this, getValue().getType());
  if (!declaration)
    return emitOpError() << "cannot resolve nominal struct declaration";
  for (auto [index, rawField] : llvm::enumerate(declaration.getFields())) {
    auto field = dyn_cast<DictionaryAttr>(rawField);
    auto name = field ? field.getAs<StringAttr>("name") : StringAttr();
    auto logical =
        field ? field.getAs<DictionaryAttr>("type") : DictionaryAttr();
    if (!field || !name || !logical)
      return emitOpError() << "referenced struct field[" << index
                           << "] metadata is malformed";
    if (name != getFieldAttr())
      continue;
    auto expected = physicalType(logical, *this);
    if (failed(expected) || getResult().getType() != *expected)
      return emitOpError() << "struct.get result type mismatch";
    return success();
  }
  return emitOpError() << "unknown struct field '" << getField() << "'";
}

} // namespace acir::ac
