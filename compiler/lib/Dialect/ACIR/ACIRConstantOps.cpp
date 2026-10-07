#include "pycircuit/Dialect/ACIR/ACIROps.h"

#include "ACIRSourceContracts.h"
#include "ACIRStaticEvaluation.h"

using namespace mlir;

namespace acir::ac {

LogicalResult ConstantOp::verify() {
  if (failed(detail::verifyDeclarationMetadata(
          *this, (*this)->getAttrOfType<DictionaryAttr>("ac.source_owner"),
          (*this)->getAttrOfType<DictionaryAttr>("ac.origin"),
          (*this)->getAttrOfType<StringAttr>("ac.declaration_role"))))
    return failure();

  auto error = [&] { return emitOpError(); };
  auto type = (*this)->getAttrOfType<DictionaryAttr>("type");
  auto value = (*this)->getAttrOfType<DictionaryAttr>("value");
  if (!type || !value ||
      failed(detail::verifyStaticTypeStructure(type, error)) ||
      failed(detail::verifyStaticValueStructure(value, error)))
    return error() << "constant requires StaticType and StaticValue";

  auto resolver = [&](FlatSymbolRefAttr symbol) {
    return detail::resolveSourceRecord(symbol, *this, error);
  };
  if (failed(detail::verifyStaticValueMatchesType(
          value, type, detail::ExpectedTypeKind::Static, resolver, error)))
    return error() << "constant value does not match its StaticType";
  return success();
}

} // namespace acir::ac
