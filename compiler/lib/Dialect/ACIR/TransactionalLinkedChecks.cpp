// The transactional closed-payload width verification (r8 review): the
// enum/struct/table and symbolic-Bits payload width checks over one package
// under an empty binding context. The shared packed-width core
// (HardwarePackedWidth.cpp, this target) runs through the closed resolver
// below, so the native transactional verifier tail calls this entry directly
// and no analysis-layer symbol participates.
#include "pycircuit/Dialect/ACIR/ACIRDialect.h"
#include "pycircuit/Dialect/ACIR/HardwarePackedWidth.h"
#include "pycircuit/Dialect/ACIR/TransactionalContracts.h"
#include "Dialect/ACIR/ACIRSourceContracts.h"
#include "llvm/ADT/StringMap.h"
#include <functional>

using namespace mlir;
namespace acir::ac {
namespace {
// The closed resolver: the same package walked under an empty binding
// context, so every width must close from its own declarations and literals.
// Package symbols are the top-level declarations, collected exactly like the
// analysis symbol map.
class ClosedPayloadResolver : public detail::PackedWidthResolver {
public:
  explicit ClosedPayloadResolver(mlir::ModuleOp package) : package(package) {
    if (!package || package->getNumRegions() != 1 ||
        !package->getRegion(0).hasOneBlock())
      return;
    for (Operation &op : *package.getBody())
      if (auto name = op.getAttrOfType<StringAttr>("sym_name"))
        symbols.try_emplace(name.getValue(), &op);
  }
  mlir::ModuleOp getPackage() const override { return package; }
  Operation *lookupDefinition(StringRef name) const override {
    auto found = symbols.find(name);
    return found == symbols.end() ? nullptr : found->second;
  }
  StructOp lookupStruct(StructType type) const override {
    return dyn_cast_or_null<StructOp>(
        lookupDefinition(type.getName().getValue()));
  }
  FailureOr<EnumDefinitionView> resolveEnum(EnumType type,
                                            Operation *site) const override {
    return detail::resolveNominalEnum(
        type, site, [this](StringRef name) { return lookupDefinition(name); });
  }
  FailureOr<Attribute> evaluateStatic(StaticExprAttr expression,
                                      const HardwareBindings &bindings,
                                      Operation *site) const override {
    return detail::evaluateHardwareStatic(*this, expression, bindings, site);
  }
  FailureOr<Type> resolveType(Type type, const HardwareBindings &bindings,
                              Operation *site) const override {
    return detail::resolveHardwareType(*this, type, bindings, site);
  }
  FailureOr<uint64_t> getPackedWidth(Type type,
                                     const HardwareBindings &bindings,
                                     Operation *site) const override {
    return detail::packedHardwareWidth(*this, type, bindings, site);
  }
  llvm::SmallVector<std::pair<Attribute, Operation *>> &
  activeStatic() const override {
    return active;
  }

private:
  mlir::ModuleOp package;
  llvm::StringMap<Operation *> symbols;
  mutable llvm::SmallVector<std::pair<Attribute, Operation *>> active;
};
Type payload(Type type) {
  if (auto t = dyn_cast<PrimitiveType>(type)) return t.getPayload();
  if (auto t = dyn_cast<StateType>(type)) return t.getPayload();
  if (auto t = dyn_cast<QueueType>(type)) return t.getPayload();
  return type;
}
FunctionType signature(Operation *op) {
  auto attr = op ? op->getAttrOfType<TypeAttr>("function_type") : TypeAttr();
  return attr ? dyn_cast<FunctionType>(attr.getValue()) : FunctionType();
}
} // namespace

LogicalResult verifyTransactionalLinkedPackage(mlir::ModuleOp package) {
  ClosedPayloadResolver resolver(package);
  // The moved width-closure half of the native checkType lambda: scope and
  // packed-width rejections keep their exact sites and texts. The
  // literal-Bits carrier width subset stays with the native hook, so a
  // literal width that passed there re-resolves here without a duplicate
  // diagnostic; the concrete-payload and module-reference guards re-run so a
  // direct staged call stays safe on mutated IR.
  std::function<LogicalResult(Type, Operation *)> checkType = [&](Type type,
                                                                  Operation *site) -> LogicalResult {
    if (isa<ModuleRefType>(type)) return success();
    type = payload(type);
    if (!isa<BitsType, EnumType, StructType, TableType>(type))
      return site->emitOpError("transactional resource contract requires concrete hardware payloads");
    HardwareBindings bindings;
    bindings.owner = isa<ModuleOp, ModuleImportOp>(site) ? site : site->getParentOfType<ModuleOp>().getOperation();
    if (bindings.owner && failed(detail::verifyHardwareTypeScope(type, bindings.owner))) return failure();
    return success(succeeded(detail::packedHardwareWidth(resolver, type, bindings, site)));
  };
  auto fail = package.walk([&](Operation *op) {
    if (op == package.getOperation()) return WalkResult::advance();
    for (Type type : op->getOperandTypes()) if (failed(checkType(type, op))) return WalkResult::interrupt();
    for (Type type : op->getResultTypes()) if (failed(checkType(type, op))) return WalkResult::interrupt();
    if (auto sig = signature(op)) {
      for (Type type : sig.getInputs()) if (failed(checkType(type, op))) return WalkResult::interrupt();
      for (Type type : sig.getResults()) if (failed(checkType(type, op))) return WalkResult::interrupt();
    }
    return WalkResult::advance();
  });
  return failure(fail.wasInterrupted());
}
} // namespace acir::ac
