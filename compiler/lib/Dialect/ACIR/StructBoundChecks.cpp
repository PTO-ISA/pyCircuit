// Staged struct declaration and access checks (B2 step-2 design r3
// ACIRPacketOps row, r9 §5): the native verifiers keep the context-free
// grammar; the packed-width, nominal-field schema and declaration-agreement
// checks compile on the analysis side and are reached from the source-unit
// walk and the resolved operation dispatch. The moved bodies are verbatim:
// they already admit generic symbolic units through the same code path.
#include "pycircuit/Dialect/ACIR/HardwareAnalysis.h"
#include "pycircuit/Dialect/ACIR/ACIROps.h"
#include "ACIRSourceContracts.h"

#include "llvm/ADT/StringSet.h"

using namespace mlir;

namespace acir::ac {
namespace detail {
LogicalResult verifyNominalStructField(
    Attribute raw, const HardwareAnalysis &analysis, Operation *lexicalModule,
    Operation *diagnosticOperation) {
  auto field = dyn_cast<DictionaryAttr>(raw);
  auto name = field ? field.getAs<StringAttr>("name") : StringAttr();
  auto type = field ? field.getAs<TypeAttr>("type") : TypeAttr();
  Attribute constraint = field ? field.get("source_constraint") : Attribute();
  if (!field || field.size() != (constraint ? 3 : 2) || !name ||
      name.getValue().empty() || !type ||
      !isa<BitsType, StructType, EnumType, TableType>(type.getValue()))
    return diagnosticOperation->emitOpError()
           << "fields require unique names and finite bits/struct/enum/table "
              "types, with optional Boolean source_constraint";
  if (!constraint)
    return success();
  auto descriptor = dyn_cast<DictionaryAttr>(constraint);
  auto kind = descriptor ? descriptor.getAs<StringAttr>("source_kind")
                         : StringAttr();
  if (!kind || kind.getValue() != "boolean")
    return diagnosticOperation->emitOpError()
           << "Struct field source_constraint requires Boolean source kind";
  return verifySourceCallHardwareConstraint(
      constraint, type.getValue(), analysis, lexicalModule, diagnosticOperation,
      "Struct field");
}
} // namespace detail

LogicalResult HardwareAnalysis::verifyStructOperation(
    Operation *operation, const HardwareBindings &bindings,
    bool requireResolved) const {
  if (auto structDecl = dyn_cast<StructOp>(operation)) {
    for (Attribute raw : structDecl.getFields())
      if (failed(detail::verifyNominalStructField(raw, *this, structDecl,
                                                  structDecl)))
        return failure();
    // Same empty-binding call shape as the native verifier this moved from.
    (void)requireResolved;
    return success(succeeded(getPackedWidth(
        StructType::get(operation->getContext(), structDecl.getSymNameAttr()),
        {}, structDecl)));
  }
  if (auto create = dyn_cast<StructCreateOp>(operation)) {
    auto declaration = lookupStruct(create.getResult().getType());
    if (!declaration || create.getValues().size() != declaration.getFields().size())
      return create.emitOpError() << "struct.create requires every declared field";
    for (auto [value, raw] : llvm::zip(create.getValues(), declaration.getFields()))
      if (!areEquivalentHardwareTypes(
              value.getType(),
              cast<DictionaryAttr>(raw).getAs<TypeAttr>("type").getValue()))
        return create.emitOpError() << "struct.create field type mismatch";
    return success();
  }
  if (auto get = dyn_cast<StructGetOp>(operation)) {
    auto declaration = lookupStruct(get.getValue().getType());
    if (!declaration)
      return get.emitOpError() << "cannot resolve nominal struct declaration";
    for (Attribute raw : declaration.getFields()) {
      auto field = cast<DictionaryAttr>(raw);
      if (field.getAs<StringAttr>("name") != get.getFieldAttr())
        continue;
      if (!areEquivalentHardwareTypes(field.getAs<TypeAttr>("type").getValue(),
                                      get.getResult().getType()))
        return get.emitOpError() << "struct.get result type mismatch";
      return success();
    }
    return get.emitOpError() << "unknown struct field '" << get.getField()
                             << "'";
  }
  return operation->emitOpError() << "operation is not a struct-family check";
}
} // namespace acir::ac
