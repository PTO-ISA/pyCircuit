#include "ACIRSourceContracts.h"
#include "pycircuit/Dialect/ACIR/HardwareAnalysis.h"
#include "llvm/ADT/StringSet.h"
using namespace mlir;
namespace acir::ac {
LogicalResult TypeAliasOp::verify() {
  return detail::verifyLogicalTypeStructure(getTarget(),
                                            [&] { return emitOpError(); });
}
LogicalResult StructOp::verify() {
  if ((*this)->hasAttr("constructor"))
    return emitOpError() << "retired struct constructor binding is unsupported "
                            "on packed hardware structs";
  if (!isa<mlir::ModuleOp>((*this)->getParentOp()))
    return emitOpError() << "struct requires package placement";
  llvm::StringSet<> names;
  if (getFields().empty())
    return emitOpError() << "packed struct requires at least one field";
  for (Attribute raw : getFields()) {
    auto field = dyn_cast<DictionaryAttr>(raw);
    auto name = field ? field.getAs<StringAttr>("name") : StringAttr();
    auto type = field ? field.getAs<TypeAttr>("type") : TypeAttr();
    if (!field || field.size() != 2 || !name || name.getValue().empty() ||
        !names.insert(name.getValue()).second || !type ||
        !isa<BitsType, StructType, EnumType>(type.getValue()))
      return emitOpError()
             << "fields require unique names and finite bits/struct/enum types";
  }
  HardwareAnalysis analysis((*this)->getParentOfType<mlir::ModuleOp>());
  return success(succeeded(analysis.getPackedWidth(
      StructType::get(getContext(), getSymNameAttr()), {}, *this)));
}
LogicalResult StructCreateOp::verify() {
  HardwareAnalysis analysis((*this)->getParentOfType<mlir::ModuleOp>());
  auto declaration = analysis.lookupStruct(getResult().getType());
  if (!declaration || getValues().size() != declaration.getFields().size())
    return emitOpError() << "struct.create requires every declared field";
  for (auto [value, raw] : llvm::zip(getValues(), declaration.getFields()))
    if (!areEquivalentHardwareTypes(
            value.getType(),
            cast<DictionaryAttr>(raw).getAs<TypeAttr>("type").getValue()))
      return emitOpError() << "struct.create field type mismatch";
  return success();
}
LogicalResult StructGetOp::verify() {
  HardwareAnalysis analysis((*this)->getParentOfType<mlir::ModuleOp>());
  auto declaration = analysis.lookupStruct(getValue().getType());
  if (!declaration)
    return emitOpError() << "cannot resolve nominal struct declaration";
  for (Attribute raw : declaration.getFields()) {
    auto field = cast<DictionaryAttr>(raw);
    if (field.getAs<StringAttr>("name") != getFieldAttr())
      continue;
    if (!areEquivalentHardwareTypes(field.getAs<TypeAttr>("type").getValue(),
                                    getResult().getType()))
      return emitOpError() << "struct.get result type mismatch";
    return success();
  }
  return emitOpError() << "unknown struct field '" << getField() << "'";
}
} // namespace acir::ac
