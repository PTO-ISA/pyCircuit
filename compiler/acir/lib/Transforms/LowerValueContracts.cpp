#include "acir/Transforms/Passes.h"

#include "acir/Dialect/ACIR/ACIROps.h"

#include "mlir/IR/IRMapping.h"
#include "mlir/IR/SymbolTable.h"
#include "llvm/ADT/SmallPtrSet.h"
#include "llvm/ADT/STLExtras.h"

using namespace mlir;

namespace acir {
namespace {

ac::VarType varType(MLIRContext *context, Type element) {
  return ac::VarType::get(context, element);
}

Value createVarGet(OpBuilder &builder, Location location, Value input,
                   StringRef field, Type type) {
  OperationState state(location, ac::VarGetOp::getOperationName());
  state.addOperands(input);
  state.addTypes(varType(builder.getContext(), type));
  state.addAttribute("field", builder.getStringAttr(field));
  return builder.create(state)->getResult(0);
}

Value createVarElement(OpBuilder &builder, Location location, Value input,
                       int64_t index, Type type) {
  OperationState state(location, ac::VarElementOp::getOperationName());
  state.addOperands(input);
  state.addTypes(varType(builder.getContext(), type));
  state.addAttribute("index", builder.getI64IntegerAttr(index));
  return builder.create(state)->getResult(0);
}

Value createCmp(OpBuilder &builder, Location location, Value lhs, Value rhs) {
  OperationState state(location, ac::VarCmpOp::getOperationName());
  state.addOperands({lhs, rhs});
  state.addTypes(varType(builder.getContext(), builder.getI1Type()));
  state.addAttribute("predicate", builder.getStringAttr("eq"));
  return builder.create(state)->getResult(0);
}

Value createBoolBinary(OpBuilder &builder, Location location, StringRef name,
                       Value lhs, Value rhs) {
  OperationState state(location, name);
  state.addOperands({lhs, rhs});
  state.addTypes(lhs.getType());
  return builder.create(state)->getResult(0);
}

Value createNot(OpBuilder &builder, Location location, Value input) {
  OperationState state(location, ac::VarNotOp::getOperationName());
  state.addOperands(input);
  state.addTypes(input.getType());
  return builder.create(state)->getResult(0);
}

FailureOr<Operation *> resolveStruct(Operation *from, ac::StructType type) {
  Operation *declaration = SymbolTable::lookupNearestSymbolFrom(
      from, cast<SymbolRefAttr>(type.getName()));
  if (!isa_and_nonnull<ac::StructOp>(declaration))
    return failure();
  return declaration;
}

LogicalResult collectEqualLeaves(OpBuilder &builder, Location location,
                                 Operation *anchor, Value lhs, Value rhs,
                                 Type type, SmallVectorImpl<Value> &leaves,
                                 llvm::SmallPtrSetImpl<Operation *> &seen) {
  if (isa<IntegerType, ac::EnumType>(type)) {
    leaves.push_back(createCmp(builder, location, lhs, rhs));
    return success();
  }
  if (auto structure = dyn_cast<ac::StructType>(type)) {
    FailureOr<Operation *> declaration = resolveStruct(anchor, structure);
    if (failed(declaration))
      return anchor->emitOpError()
             << "cannot lower equality for unresolved " << structure;
    if (!seen.insert(*declaration).second)
      return anchor->emitOpError()
             << "cannot lower equality for recursive " << structure;
    ArrayAttr fields = (*declaration)->getAttrOfType<ArrayAttr>("fields");
    for (Attribute rawField : fields) {
      auto field = cast<DictionaryAttr>(rawField);
      StringRef name = field.getAs<StringAttr>("name").getValue();
      Type fieldType = field.getAs<TypeAttr>("type").getValue();
      Value lhsField = createVarGet(builder, location, lhs, name, fieldType);
      Value rhsField = createVarGet(builder, location, rhs, name, fieldType);
      if (failed(collectEqualLeaves(builder, location, anchor, lhsField,
                                    rhsField, fieldType, leaves, seen)))
        return failure();
    }
    seen.erase(*declaration);
    return success();
  }
  if (auto tuple = dyn_cast<TupleType>(type)) {
    for (auto [index, element] : llvm::enumerate(tuple.getTypes())) {
      Value lhsElement =
          createVarElement(builder, location, lhs, index, element);
      Value rhsElement =
          createVarElement(builder, location, rhs, index, element);
      if (failed(collectEqualLeaves(builder, location, anchor, lhsElement,
                                    rhsElement, element, leaves, seen)))
        return failure();
    }
    return success();
  }
  if (auto array = dyn_cast<ac::ValueArrayType>(type)) {
    for (int64_t index = 0; index < array.getLength(); ++index) {
      Value lhsElement = createVarElement(builder, location, lhs, index,
                                          array.getElementType());
      Value rhsElement = createVarElement(builder, location, rhs, index,
                                          array.getElementType());
      if (failed(collectEqualLeaves(builder, location, anchor, lhsElement,
                                    rhsElement, array.getElementType(),
                                    leaves, seen)))
        return failure();
    }
    return success();
  }
  return anchor->emitOpError() << "cannot lower equality leaf type " << type;
}

LogicalResult lowerAggregateCmp(ac::VarCmpOp comparison) {
  Type payload = cast<ac::VarType>(comparison.getLhs().getType())
                     .getElementType();
  if (!isa<ac::StructType, TupleType, ac::ValueArrayType>(payload))
    return success();
  OpBuilder builder(comparison);
  SmallVector<Value> leaves;
  llvm::SmallPtrSet<Operation *, 8> seen;
  if (failed(collectEqualLeaves(builder, comparison.getLoc(), comparison,
                                comparison.getLhs(), comparison.getRhs(),
                                payload, leaves, seen)))
    return failure();
  if (leaves.empty())
    return comparison.emitOpError("aggregate equality has no scalar leaves");
  while (leaves.size() > 1) {
    SmallVector<Value> next;
    for (size_t index = 0; index < leaves.size(); index += 2) {
      if (index + 1 == leaves.size()) {
        next.push_back(leaves[index]);
        continue;
      }
      next.push_back(createBoolBinary(builder, comparison.getLoc(),
                                      ac::VarAndOp::getOperationName(),
                                      leaves[index], leaves[index + 1]));
    }
    leaves = std::move(next);
  }
  Value result = leaves.front();
  if (comparison.getPredicate() == "ne")
    result = createNot(builder, comparison.getLoc(), result);
  comparison.getResult().replaceAllUsesWith(result);
  comparison.erase();
  return success();
}

LogicalResult inlineInvariant(ac::VarInvariantOp invariant) {
  Block &block = invariant.getPredicate().front();
  auto yielded = cast<ac::VarInvariantYieldOp>(block.getTerminator());
  OpBuilder builder(invariant);
  IRMapping mapping;
  mapping.map(block.getArgument(0), invariant.getInput());
  for (Operation &nested : block.without_terminator())
    builder.clone(nested, mapping);
  Value result = mapping.lookupOrNull(yielded.getValue());
  if (!result)
    return invariant.emitOpError(
        "predicate yield value is outside the invariant region");
  invariant.getResult().replaceAllUsesWith(result);
  invariant.erase();
  return success();
}

#define GEN_PASS_DEF_LOWERVALUECONTRACTSPASS
#include "acir/Transforms/Passes.h.inc"

struct LowerValueContractsPass
    : impl::LowerValueContractsPassBase<LowerValueContractsPass> {
  void runOnOperation() override {
    if (failed(lowerValueContracts(getOperation())))
      signalPassFailure();
  }
};

} // namespace

LogicalResult lowerValueContracts(ModuleOp model) {
  SmallVector<ac::VarCmpOp> comparisons;
  model.walk([&](ac::VarCmpOp comparison) {
    Type payload = cast<ac::VarType>(comparison.getLhs().getType())
                       .getElementType();
    if (isa<ac::StructType, TupleType, ac::ValueArrayType>(payload))
      comparisons.push_back(comparison);
  });
  for (ac::VarCmpOp comparison : comparisons)
    if (failed(lowerAggregateCmp(comparison)))
      return failure();

  while (true) {
    SmallVector<ac::VarInvariantOp> invariants;
    model.walk([&](ac::VarInvariantOp invariant) {
      invariants.push_back(invariant);
    });
    if (invariants.empty())
      break;

    SmallVector<ac::VarInvariantOp> leaves;
    for (ac::VarInvariantOp invariant : invariants) {
      bool hasCallee = false;
      invariant.getPredicate().walk([&](ac::VarInvariantOp) {
        hasCallee = true;
        return WalkResult::interrupt();
      });
      if (!hasCallee)
        leaves.push_back(invariant);
    }
    if (leaves.empty())
      return model.emitError(
          "invariant call graph has no leaf; recursive composition is illegal");
    for (ac::VarInvariantOp leaf : leaves)
      if (failed(inlineInvariant(leaf)))
        return failure();
  }
  return success();
}

std::unique_ptr<Pass> createLowerValueContractsPass() {
  return std::make_unique<LowerValueContractsPass>();
}

} // namespace acir
