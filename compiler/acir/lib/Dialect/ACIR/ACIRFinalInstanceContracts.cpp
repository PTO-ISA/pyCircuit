#include "ACIRFinalContracts.h"

#include "ACIRSourceContracts.h"
#include "mlir/IR/BuiltinOps.h"

using namespace mlir;

namespace acir::ac {
namespace {

mlir::ModuleOp packageOf(InstanceOp op) {
  auto unit = op->getParentOfType<mlir::ModuleOp>();
  auto package = unit ? dyn_cast_or_null<mlir::ModuleOp>(unit->getParentOp())
                      : mlir::ModuleOp();
  auto stage =
      package ? package->getAttrOfType<StringAttr>("ac.stage") : StringAttr();
  return stage && stage.getValue() == "final" &&
                 !package->hasAttr("ac.unit_kind") &&
                 package->hasAttr("ac.entry") &&
                 package->hasAttr("ac.instance_bindings")
             ? package
             : mlir::ModuleOp();
}

bool control(Value value, ModuleOp module, unsigned index) {
  auto argument = dyn_cast<BlockArgument>(value);
  return argument && argument.getOwner() == &module.getBody().front() &&
         argument.getArgNumber() == index && value.getType().isInteger(1);
}

} // namespace

LogicalResult verifyFinalInstance(InstanceOp op) {
  auto emit = [&] { return op.emitOpError(); };
  auto unit = op->getParentOfType<mlir::ModuleOp>();
  auto unitKind =
      unit ? unit->getAttrOfType<StringAttr>("ac.unit_kind") : StringAttr();
  auto unitOwner = unit ? unit->getAttrOfType<DictionaryAttr>("ac.source_owner")
                        : DictionaryAttr();
  auto module = op->getParentOfType<ModuleOp>();
  auto key = dyn_cast<DictionaryAttr>(op.getCalleeAttr());
  auto bindings = op->getAttrOfType<ArrayAttr>("ac.port_bindings");
  if (!packageOf(op) || !unitKind || unitKind.getValue() != "implementation" ||
      failed(detail::verifySourceOwner(unitOwner, emit)) ||
      op.getName().empty() ||
      failed(detail::verifyOccurrence(
          op->getAttrOfType<DictionaryAttr>("ac.origin"), emit)) ||
      !module || !control(op.getClock(), module, 0) ||
      !control(op.getReset(), module, 1) ||
      failed(detail::verifySpecKey(key, emit)) || !bindings ||
      op->hasAttr("ac.static_args") ||
      op.getNextValues().size() != 2 * op.getTargets().size())
    return emit() << "global final instance metadata/control is incomplete";
  for (size_t index = 0; index < op.getTargets().size(); ++index) {
    Type payload =
        cast<RegType>(op.getTargets()[index].getType()).getElementType();
    if (op.getNextValues()[2 * index].getType() != payload ||
        !op.getNextValues()[2 * index + 1].getType().isInteger(1))
      return emit() << "global final instance result type is invalid";
  }
  return success();
}

} // namespace acir::ac
