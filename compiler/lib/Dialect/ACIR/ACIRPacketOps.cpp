// Grammar-only native verifies (B2 step-2 design r3 ACIRPacketOps row,
// r9 §5): the nominal-field schema, packed-width and declaration-agreement
// checks moved to the staged StructBoundChecks TU and run from the
// source-unit walk and the resolved operation dispatch.
#include "pycircuit/Dialect/ACIR/ACIROps.h"
#include "ACIRSourceContracts.h"
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
    if (!field || !field.getAs<StringAttr>("name"))
      return emitOpError() << "fields require unique names and finite "
                              "bits/struct/enum/table types, with optional "
                              "Boolean source_constraint";
    if (!names.insert(field.getAs<StringAttr>("name").getValue()).second)
      return emitOpError() << "Struct fields require unique names";
  }
  return success();
}
} // namespace acir::ac
