#include "pycircuit/Dialect/ACIR/ACIROps.h"
#include "pycircuit/Dialect/ACIR/TransactionalContracts.h"
#include "mlir/Dialect/SCF/IR/SCF.h"
#include "llvm/ADT/APSInt.h"

using namespace mlir;
namespace acir::ac {
namespace {
std::optional<uint64_t> capacity(TableType type) {
  auto tree = type.getCapacity().getTree();
  auto kind = tree.getAs<StringAttr>("kind");
  auto value = tree.getAs<DictionaryAttr>("value");
  auto number = value ? value.getAs<MathIntAttr>("value") : MathIntAttr();
  if (!kind || kind.getValue() != "literal" || !number)
    return std::nullopt;
  llvm::APSInt integer(number.getCanonicalValue());
  if (integer.isNegative() || integer.isZero() || integer.getActiveBits() > 63)
    return std::nullopt;
  return integer.getZExtValue();
}
Operation *provider(Operation *site, ModuleRefType type) {
  auto package = site->getParentOfType<mlir::ModuleOp>();
  return package ? SymbolTable::lookupSymbolIn(package, type.getModule()) : nullptr;
}
bool memberType(Operation *site, Type expected, Type actual) {
  if (transactionalTypesEqual(expected, actual)) return true;
  auto left = dyn_cast<ModuleRefType>(expected);
  auto right = dyn_cast<ModuleRefType>(actual);
  if (!left || !right || !isLinkedTransactionalPackage(site)) return false;
  auto *a = provider(site, left), *b = provider(site, right);
  return a && b && getTransactionalMethodInterfaces(a) ==
                       getTransactionalMethodInterfaces(b);
}
} // namespace
LogicalResult TableType::verify(function_ref<InFlightDiagnostic()> error,
                               StaticExprAttr capacity, Type elementType) {
  if (!capacity || !isa<StateType, ModuleRefType>(elementType))
    return error() << "component Table requires static capacity and State or Module references";
  if (failed(StaticExprAttr::verify(error, capacity.getTree()))) return failure();
  auto tree = capacity.getTree();
  auto kind = tree.getAs<StringAttr>("kind");
  if (kind && kind.getValue() == "literal") {
    auto value = tree.getAs<DictionaryAttr>("value");
    auto number = value ? value.getAs<MathIntAttr>("value") : MathIntAttr();
    if (!number) return error() << "Table capacity requires an Integer";
    llvm::APSInt integer(number.getCanonicalValue());
    if (integer.isNegative() || integer.isZero() || integer.getActiveBits() > 63)
      return error() << "Table capacity requires a positive i64";
  }
  return success();
}
LogicalResult TableAllocOp::verify() {
  if (!isTransactionalPackage(*this) || isLinkedTransactionalPackage(*this))
    return emitOpError("Table allocation requires unlinked transactional ACIR");
  Operation *owner = (*this)->getParentOp();
  while (isa_and_nonnull<scf::ForOp>(owner)) owner = owner->getParentOp();
  if (!isa_and_nonnull<ModuleOp>(owner) || getInstanceName().empty())
    return emitOpError("Table allocation requires a named Module-owned allocation");
  auto type = getResult().getType();
  if (auto state = dyn_cast<StateType>(type.getElementType())) {
    for (Value value : getInitial())
      if (!transactionalTypesEqual(value.getType(), state.getPayload()))
        return emitOpError("Table initializers must match the member payload");
    if (!getInitialImage() && getInitial().size() > 1)
      return emitOpError("Table broadcast accepts at most one initializer");
    if (auto count = capacity(type); count && getInitialImage() && getInitial().size() != *count)
      return emitOpError("Table initializer image must cover its capacity");
  } else if (!getInitial().empty())
    return emitOpError("Module Table uses provider defaults, not payload initializers");
  return success();
}
LogicalResult TableCreateOp::verify() {
  if (!isTransactionalPackage(*this)) return emitOpError("requires transactional ACIR");
  if (getMembers().empty()) return emitOpError("Table requires members");
  if (auto count = capacity(getResult().getType()); count && *count != getMembers().size())
    return emitOpError("Table capacity differs from member count");
  for (Value member : getMembers())
    if (!memberType(*this, getResult().getType().getElementType(), member.getType()))
      return emitOpError("Table members require the declared component interface");
  return success();
}
LogicalResult TableSelectOp::verify() {
  if (!isTransactionalPackage(*this)) return emitOpError("requires transactional ACIR");
  if (isLinkedTransactionalPackage(*this) && !isa<BitsType>(getIndex().getType()))
    return emitOpError("linked Table index requires concrete Bits");
  if (!memberType(*this, getTable().getType().getElementType(), getResult().getType()))
    return emitOpError("Table selection must retain its element reference type");
  if (auto index = getIndex().getDefiningOp<BitsConstantOp>()) {
    auto tree = index.getValue().getTree();
    auto value = tree.getAs<DictionaryAttr>("value");
    auto number = value ? value.getAs<MathIntAttr>("value") : MathIntAttr();
    if (number) {
      llvm::APSInt integer(number.getCanonicalValue());
      if (auto count = capacity(getTable().getType()); count &&
          (integer.isNegative() || integer.getActiveBits() > 63 || integer.getZExtValue() >= *count))
        return emitOpError("Table index is outside its capacity");
    }
  }
  return success();
}
} // namespace acir::ac
