// Collection operation local verifiers (B2 step-2 design r9): these stay
// with the operations layer. The bound checks that required HardwareAnalysis
// left the native verifiers at IX (r7 row): the staged collection rows — the
// source-unit semantic walk in its symbolic-tolerant mode and
// HardwareAnalysis::verifyResolvedOperation at the binding tiers — run the
// shared member implementations from CollectionAnalysis.cpp. The native
// verifiers keep placement, naming and region grammar.
#include "ACIRSourceContracts.h"
#include "IR/StateDeclarationContracts.h"
#include "pycircuit/Dialect/ACIR/ACIROps.h"

using namespace mlir;

namespace acir::ac {
namespace {

bool equivalentTypes(TypeRange lhs, TypeRange rhs) {
  if (lhs.size() != rhs.size())
    return false;
  for (auto [left, right] : llvm::zip(lhs, rhs))
    if (!areEquivalentHardwareTypes(left, right))
      return false;
  return true;
}

LogicalResult scalarRegion(Operation *op, TypeRange args, TypeRange results) {
  auto &region = op->getRegion(0);
  if (!region.hasOneBlock() || region.front().empty() ||
      !equivalentTypes(region.front().getArgumentTypes(), args) ||
      !isa<YieldOp>(region.front().back()))
    return op->emitOpError() << "requires isolated single block with matching "
                                "scalar arguments and ac.yield";
  if (!equivalentTypes(region.front().back().getOperandTypes(), results))
    return op->emitOpError()
           << "scalar region yield types do not match results";
  for (auto &nested : region.front().without_terminator())
    if (!isa<BitsConstantOp, BitsConstant4Op, BitsUnaryOp, BitsBinaryOp,
             BitsCompareOp, BitsSelectOp, BitsConcatOp, BitsExtractOp,
             BitsResizeOp, StructCreateOp, StructGetOp, EnumCreateOp,
             EnumToBitsOp, EnumFromBitsOp, ValueMergeOp>(nested) &&
        !(isa<TableMapOp>(op) &&
          isa<TableCreateOp, TableSplatOp, TableReadOp, TableGetOp, TableMapOp>(nested)))
      return nested.emitOpError()
             << "table region accepts only closed pure scalar operations";
  return success();
}

} // namespace


LogicalResult CollectionOp::verify() {
  auto parent = dyn_cast_or_null<ModuleOp>((*this)->getParentOp());
  if (!parent || getInstanceName().empty())
    return emitOpError() << "collection requires named direct module placement";
  for (Operation &other : parent.getBody().front())
    if (&other != getOperation() && isa<InstanceOp, CollectionOp, QueueOp>(other) &&
        other.getAttrOfType<StringAttr>("instance_name") ==
            getInstanceNameAttr())
      return emitOpError() << "instance_name must be unique within parent";
  return detail::verifyOccurrence(getOccurrence(),
                                  [&] { return emitOpError(); });
}
// Symbol-use verification keeps the default (no additional symbol facts);
// the bound checks run from the staged collection rows after the IX
// severing.
LogicalResult CollectionOp::verifySymbolUses(SymbolTableCollection &) {
  return success();
}
LogicalResult TableMapOp::verifyRegions() {
  if (getBody().empty() || getBody().front().getArguments().empty())
    return emitOpError() << "table.map requires ordinal argument";
  SmallVector<Type> args{getBody().front().getArgument(0).getType()}, results;
  for (Value v : getTables())
    args.push_back(cast<TableType>(v.getType()).getElementType());
  llvm::append_range(args, getCaptures().getTypes());
  for (Value v : getResults())
    results.push_back(cast<TableType>(v.getType()).getElementType());
  return scalarRegion(*this, args, results);
}
LogicalResult TableMatchOp::verifyRegions() {
  SmallVector<Type> args{getInput().getType().getElementType()};
  llvm::append_range(args, getCaptures().getTypes());
  if (getBody().empty() || getBody().front().empty() ||
      !isa<YieldOp>(getBody().front().back()) ||
      getBody().front().back().getNumOperands() != 1)
    return emitOpError() << "table.match predicate must yield one bit";
  auto result =
      dyn_cast<BitsType>(getBody().front().back().getOperand(0).getType());
  if (!result)
    return emitOpError() << "table.match predicate must yield one bit";
  // The yield width resolution runs from the staged collection entry
  // (r3 CollectionLocalVerifies residual).
  return scalarRegion(*this, args, TypeRange{result});
}
} // namespace acir::ac
