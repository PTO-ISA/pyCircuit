#include "acir/Dialect/ACIR/ACIROps.h"

#include "ACIRSourceContracts.h"
#include "mlir/IR/BuiltinOps.h"
#include "llvm/ADT/StringSwitch.h"

using namespace mlir;

namespace acir::ac {
namespace {

LogicalResult requireSourceMathOp(Operation *operation) {
  auto file = operation->getParentOfType<mlir::ModuleOp>();
  auto stage =
      file ? file->getAttrOfType<StringAttr>("ac.stage") : StringAttr();
  auto kind =
      file ? file->getAttrOfType<StringAttr>("ac.unit_kind") : StringAttr();
  if (!stage || stage.getValue() != "source" || !kind ||
      kind.getValue() != "implementation")
    return operation->emitOpError()
           << "source math op requires a source-stage implementation unit";
  if (isa<UnknownLoc>(operation->getLoc()))
    return operation->emitOpError()
           << "source math op requires a source location";
  auto origin = operation->getAttrOfType<DictionaryAttr>("ac.origin");
  if (failed(detail::verifyOccurrence(
          origin, [&] { return operation->emitOpError(); })))
    return failure();
  return success();
}

LogicalResult verifyIntegerDomain(DictionaryAttr domain, Type physical,
                                  Operation *operation) {
  auto error = [&] { return operation->emitOpError(); };
  if (!domain || failed(detail::verifyLogicalTypeStructure(domain, error)))
    return error() << "domain must be a closed LogicalType.Integer";
  auto kind = domain.getAs<StringAttr>("kind");
  auto storage = domain.getAs<TypeAttr>("storage");
  if (!kind || kind.getValue() != "integer" || !storage)
    return error() << "domain must be LogicalType.Integer";
  if (storage.getValue() != physical)
    return error() << "integer domain storage must exactly match bounded bits";
  auto integer = dyn_cast<IntegerType>(physical);
  if (!integer || !integer.isSignless() || integer.getWidth() == 0 ||
      integer.getWidth() > 64)
    return error() << "bounded source integer storage must be signless i1..i64";
  return success();
}

LogicalResult verifyCheckTemplate(DictionaryAttr check, StringRef expectedKind,
                                  Operation *operation) {
  auto error = [&] { return operation->emitOpError(); };
  if (!check || check.size() != 4)
    return error() << "check_template must contain exactly four fields";
  auto leaf = check.getAs<DictionaryAttr>("leaf");
  auto kind = check.getAs<StringAttr>("kind");
  auto obligation = check.getAs<IntegerAttr>("obligation");
  auto location = check.getAs<DictionaryAttr>("location");
  if (!leaf || failed(detail::verifySite(leaf, error)) || !kind ||
      kind.getValue() != expectedKind ||
      failed(
          detail::decodeU64(obligation, "check_template obligation", error)) ||
      !location || failed(detail::verifySourceSpan(location, error)))
    return error() << "check_template does not match the source obligation";
  return success();
}

} // namespace

LogicalResult MathConstantOp::verify() { return requireSourceMathOp(*this); }

LogicalResult MathFromBitsOp::verify() {
  if (failed(requireSourceMathOp(*this)))
    return failure();
  return verifyIntegerDomain(getDomain(), getValue().getType(), *this);
}

LogicalResult MathBinaryOp::verify() {
  if (failed(requireSourceMathOp(*this)))
    return failure();
  auto operation = (*this)->getAttrOfType<StringAttr>("operator");
  if (!operation)
    return emitOpError() << "requires StringAttr 'operator'";
  if (!llvm::StringSwitch<bool>(operation.getValue())
           .Cases({"add", "and_bits"}, true)
           .Default(false))
    return emitOpError()
           << "operator '" << operation.getValue()
           << "' is not implemented by the source-math foundation";
  if ((*this)->getAttr("ac.check_template"))
    return emitOpError() << "add and and_bits must not carry a check_template";
  return success();
}

LogicalResult MathToBitsOp::verify() {
  if (failed(requireSourceMathOp(*this)))
    return failure();
  if (failed(verifyIntegerDomain(getDomain(), getResult().getType(), *this)))
    return failure();
  return verifyCheckTemplate(
      (*this)->getAttrOfType<DictionaryAttr>("ac.check_template"), "range",
      *this);
}

} // namespace acir::ac
