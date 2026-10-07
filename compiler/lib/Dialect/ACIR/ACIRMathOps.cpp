#include "ACIRSourceContracts.h"
#include "SourceValueSemantics.h"
#include "pycircuit/Dialect/ACIR/ACIROps.h"
using namespace mlir;
namespace acir::ac {
namespace {
LogicalResult domain(DictionaryAttr value, Type storage, Operation *op) {
  if (failed(detail::verifyLogicalTypeStructure(
          value, [&] { return op->emitOpError(); })))
    return failure();
  auto declared = value.getAs<TypeAttr>("storage");
  if (!declared || declared.getValue() != storage || !isa<IntegerType>(storage))
    return op->emitOpError()
           << "mathematical conversion storage must match finite domain";
  return success();
}
} // namespace
LogicalResult MathConstantOp::verify() { return success(); }
LogicalResult MathFromBitsOp::verify() {
  return domain(getDomain(), getValue().getType(), *this);
}
LogicalResult MathToBitsOp::verify() {
  return domain(getDomain(), getResult().getType(), *this);
}
LogicalResult MathBinaryOp::verify() {
  auto operation = (*this)->getAttrOfType<StringAttr>("operator");
  auto opcode =
      operation ? detail::parseValueOpcode(operation.getValue()) : std::nullopt;
  if (!opcode || detail::getValueOpcodeInfo(*opcode).arity != 2 ||
      detail::getValueOpcodeInfo(*opcode).result != detail::ValueKind::Integer)
    return emitOpError() << "mathematical binary requires implemented integer "
                            "binary operator";
  return success();
}
LogicalResult MathCompareOp::verify() {
  if (!llvm::is_contained(
          ArrayRef<StringRef>{"eq", "ne", "lt", "le", "gt", "ge"},
          getPredicate()) ||
      !getResult().getType().isInteger(1))
    return emitOpError()
           << "mathematical comparison requires known predicate and i1 result";
  return success();
}
} // namespace acir::ac
