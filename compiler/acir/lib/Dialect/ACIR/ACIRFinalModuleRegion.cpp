#include "ACIRFinalContracts.h"

#include "ACIRSourceContracts.h"
#include "mlir/IR/Builders.h"
#include "llvm/ADT/STLExtras.h"
#include "llvm/ADT/StringSet.h"

using namespace mlir;

namespace acir::ac {

LogicalResult verifyFinalModuleRegion(ModuleOp op) {
  auto emit = [&] { return op.emitOpError(); };
  if (op.getBody().getBlocks().size() != 1)
    return emit() << "final module requires one block";
  Block &body = op.getBody().front();
  auto ports = op->getAttrOfType<ArrayAttr>("ac.ports");
  auto targets = op->getAttrOfType<ArrayAttr>("ac.commit_targets");
  auto terminator = dyn_cast_or_null<YieldOp>(body.getTerminator());
  if (!ports || body.getNumArguments() != ports.size() + 2 ||
      !body.getArgument(0).getType().isInteger(1) ||
      !body.getArgument(1).getType().isInteger(1) || !terminator || !targets ||
      terminator.getValues().size() != 2 * targets.size())
    return emit() << "global final module argument/commit arity is invalid";
  Builder builder(op.getContext());
  SmallVector<std::pair<DictionaryAttr, Type>> expected;
  llvm::StringSet<> names;
  for (RegOp reg : body.getOps<RegOp>()) {
    if (!names.insert(reg.getName()).second)
      return emit() << "final module repeats a register name";
    expected.push_back(
        {builder.getDictionaryAttr({
             builder.getNamedAttr("kind", builder.getStringAttr("owned")),
             builder.getNamedAttr("declaration",
                                  reg->getAttr("ac.declaration")),
             builder.getNamedAttr("element", reg->getAttr("ac.element")),
         }),
         cast<RegType>(reg.getState().getType()).getElementType()});
  }
  for (auto [index, raw] : llvm::enumerate(ports)) {
    auto port = cast<DictionaryAttr>(raw);
    auto handle = dyn_cast<RegType>(body.getArgument(index + 2).getType());
    auto logical = port.getAs<DictionaryAttr>("type");
    auto storage = logical ? logical.getAs<TypeAttr>("storage") : TypeAttr();
    auto kind = logical ? logical.getAs<StringAttr>("kind") : StringAttr();
    bool physical =
        handle && kind &&
        ((kind.getValue() == "bool" && handle.getElementType().isInteger(1)) ||
         (kind.getValue() == "integer" && storage &&
          storage.getValue() == handle.getElementType()) ||
         kind.getValue() == "record");
    if (!physical)
      return emit() << "final module port payload type is invalid";
    if (port.getAs<StringAttr>("role").getValue() != "next")
      continue;
    expected.push_back(
        {builder.getDictionaryAttr({
             builder.getNamedAttr("kind", builder.getStringAttr("formal")),
             builder.getNamedAttr("parameter", port.get("parameter")),
             builder.getNamedAttr("ordinal", port.get("ordinal")),
         }),
         handle.getElementType()});
  }
  if (expected.size() != targets.size())
    return emit() << "global final module commit target inventory is stale";
  for (auto [index, item] : llvm::enumerate(expected)) {
    auto target = dyn_cast<DictionaryAttr>(targets[index]);
    if (!target || failed(detail::verifyStateRef(target, emit)) ||
        target != item.first ||
        terminator.getValues()[2 * index].getType() != item.second ||
        !terminator.getValues()[2 * index + 1].getType().isInteger(1))
      return emit() << "global final module commit target/type is invalid";
  }
  return success();
}

} // namespace acir::ac
