#include "ACIRSourceContracts.h"
#include "mlir/IR/SymbolTable.h"
#include "mlir/IR/Verifier.h"
#include "pycircuit/Dialect/ACIR/HardwareAnalysis.h"
#include "pycircuit/Dialect/ACIR/SourceIdentifier.h"
#include "pycircuit/Dialect/ACIR/SourceMethodAnalysis.h"
#include "llvm/ADT/DenseSet.h"
#include "llvm/ADT/StringSet.h"

using namespace mlir;

namespace acir::ac {
namespace {

HardwareBindings scope(Operation *operation) {
  HardwareBindings bindings;
  bindings.owner = operation->getParentOfType<ModuleOp>();
  return bindings;
}

LogicalResult moduleOccurrence(Operation *operation, DictionaryAttr identity,
                               StringRef description) {
  auto error = [&] { return operation->emitOpError(); };
  if (failed(detail::verifyOccurrence(identity, error)))
    return failure();
  auto owner = operation->getParentOfType<ModuleOp>();
  auto symbol =
      owner ? owner->getAttrOfType<StringAttr>("sym_name") : StringAttr();
  if (!symbol)
    return error() << description << " requires a named owning module";
  return detail::verifyOriginDefinition(
      identity,
      FlatSymbolRefAttr::get(operation->getContext(), symbol.getValue()),
      description, error);
}

LogicalResult occurrence(Operation *operation) {
  return moduleOccurrence(
      operation, operation->getAttrOfType<DictionaryAttr>("occurrence"),
      "typed source operation");
}

LogicalResult namedDeclaration(Operation *operation, StringAttr name) {
  auto module = dyn_cast_or_null<ModuleOp>(operation->getParentOp());
  if (!module || !name)
    return operation->emitOpError() << "requires named direct module placement";
  if (failed(detail::verifyPythonAstIdentifier(
          name.getValue(), true, [&] { return operation->emitOpError(); })))
    return failure();
  return occurrence(operation);
}

LogicalResult predicate(const HardwareAnalysis &analysis, Value value,
                        const HardwareBindings &bindings, Operation *site,
                        StringRef description) {
  if (!isa<BitsType>(value.getType()))
    return site->emitOpError() << description << " requires width-one Bits";
  auto width = analysis.getPackedWidth(value.getType(), bindings, site);
  if (failed(width))
    return failure();
  if (*width != 1)
    return site->emitOpError() << description << " requires width-one Bits";
  return success();
}

bool equivalentTypes(TypeRange left, TypeRange right) {
  if (left.size() != right.size())
    return false;
  for (auto [lhs, rhs] : llvm::zip(left, right))
    if (!areEquivalentHardwareTypes(lhs, rhs))
      return false;
  return true;
}

LogicalResult transactionPlacement(Operation *operation, bool allowGuard) {
  auto *parent = operation->getParentOp();
  if (!isa_and_nonnull<SourceMethodOp, SourceTransactionOp>(parent))
    return operation->emitOpError()
           << "requires direct method or transaction region placement";
  if (!allowGuard && operation->getParentRegion() != &parent->getRegion(1))
    return operation->emitOpError()
           << "state writes and method calls are forbidden in guards";
  return occurrence(operation);
}

LogicalResult region(Operation *operation, unsigned ordinal, TypeRange inputs,
                     TypeRange outputs) {
  auto &region = operation->getRegion(ordinal);
  bool guard = ordinal == 0;
  if (!region.hasOneBlock() || region.front().empty() ||
      !equivalentTypes(region.front().getArgumentTypes(), inputs) ||
      !isa<YieldOp>(region.front().back()))
    return operation->emitOpError()
           << "guard/body requires one block, matching arguments and final "
              "ac.yield";
  auto &block = region.front();
  auto &yield = block.back();
  HardwareAnalysis analysis(operation->getParentOfType<mlir::ModuleOp>());
  if (guard) {
    if (yield.getNumOperands() != 1)
      return yield.emitOpError() << "guard must yield exactly one predicate";
    if (failed(predicate(analysis, yield.getOperand(0), scope(operation),
                         &yield, "guard yield")))
      return failure();
  } else if (!equivalentTypes(yield.getOperandTypes(), outputs)) {
    return yield.emitOpError() << "body yield types do not match results";
  }
  for (Operation &nested : block.without_terminator()) {
    if (isa<SourceStateReadOp>(nested))
      continue;
    if (!guard && isa<SourceStateWriteOp, SourceMethodCallOp>(nested))
      continue;
    if (!detail::isCombinationalHardwareOperation(&nested))
      return nested.emitOpError()
             << (guard ? "guard accepts only closed pure payload operations "
                         "and state reads"
                       : "body accepts only closed payload operations, state "
                         "accesses and method calls");
    // Pure nested collection regions do not open another effect placement.
    auto walk =
        nested.walk([&](Operation *child) {
          if (child == &nested ||
              detail::isCombinationalHardwareOperation(child) ||
              isa<YieldOp>(child))
            return WalkResult::advance();
          child->emitOpError()
              << "nested payload region cannot contain transaction "
                 "effects or declarations";
          return WalkResult::interrupt();
        });
    if (walk.wasInterrupted())
      return failure();
  }
  return success();
}

FailureOr<Type> selectedStateType(Operation *operation, ArrayAttr selectors,
                                  ValueRange indices) {
  auto owner = operation->getParentOfType<ModuleOp>();
  auto name = operation->getAttrOfType<StringAttr>("state");
  SourceStateOp state;
  if (owner && owner.getBody().hasOneBlock() && name)
    for (auto candidate : owner.getBody().front().getOps<SourceStateOp>())
      if (candidate->getAttrOfType<StringAttr>("instance_name") == name) {
        if (state)
          return operation->emitOpError() << "state reference is ambiguous";
        state = candidate;
      }
  if (!state)
    return operation->emitOpError()
           << "state reference must resolve to a state in the nearest module";
  if (state->getNumOperands() != 3 || state->getNumResults() != 1 ||
      state->getNumRegions() != 0)
    return state.emitOpError()
           << "state requires clock, reset, initial value and one old-Q result";
  if (failed(state.verifyInvariantsImpl()))
    return failure();
  HardwareAnalysis analysis(operation->getParentOfType<mlir::ModuleOp>());
  auto bindings = scope(operation);
  auto resolved =
      analysis.resolveType(state.getQ().getType(), bindings, operation);
  if (failed(resolved))
    return failure();
  Type current = *resolved;
  unsigned nextIndex = 0;
  for (Attribute raw : selectors) {
    auto selector = dyn_cast<DictionaryAttr>(raw);
    if (!selector || selector.size() != 1)
      return operation->emitOpError()
             << "state selector requires exactly field or index";
    if (auto field = selector.getAs<StringAttr>("field")) {
      auto record = dyn_cast<StructType>(current);
      auto declaration = record ? analysis.lookupStruct(record) : StructOp();
      if (!declaration)
        return operation->emitOpError()
               << "state field selector requires a resolved nominal struct";
      Type selected;
      for (Attribute rawField : declaration.getFields()) {
        auto item = cast<DictionaryAttr>(rawField);
        if (item.getAs<StringAttr>("name") == field) {
          selected = item.getAs<TypeAttr>("type").getValue();
          break;
        }
      }
      if (!selected)
        return operation->emitOpError()
               << "unknown state field '" << field.getValue() << "'";
      auto resolvedField = analysis.resolveType(selected, bindings, operation);
      if (failed(resolvedField))
        return failure();
      current = *resolvedField;
      continue;
    }
    auto ordinal = selector.getAs<IntegerAttr>("index");
    auto table = dyn_cast<TableType>(current);
    if (!ordinal || !ordinal.getType().isSignlessInteger(64) ||
        ordinal.getValue().isNegative() ||
        ordinal.getValue().getZExtValue() != nextIndex ||
        nextIndex >= indices.size() || !table || table.getShape().empty())
      return operation->emitOpError()
             << "state index selector requires a Table and sequential i64 "
                "SSA index ordinal";
    auto indexWidth = analysis.getPackedWidth(indices[nextIndex].getType(),
                                              bindings, operation);
    if (!isa<BitsType>(indices[nextIndex].getType()) || failed(indexWidth))
      return operation->emitOpError() << "state index must have resolved Bits "
                                         "type";
    auto shape = analysis.getTableShape(table, bindings, operation);
    if (failed(shape))
      return failure();
    ++nextIndex;
    if (table.getShape().size() == 1)
      current = table.getElementType();
    else
      current = TableType::get(
          operation->getContext(),
          ArrayAttr::get(operation->getContext(),
                         table.getShape().getValue().drop_front()),
          table.getElementType());
  }
  if (nextIndex != indices.size())
    return operation->emitOpError()
           << "state selector index count must match SSA indices exactly";
  return current;
}

LogicalResult staticInitializer(const HardwareAnalysis &analysis, Value initial,
                                const HardwareBindings &bindings,
                                Operation *state) {
  // Prove closure of the existing literal/constructor graph. This performs
  // no SSA arithmetic evaluation and never obtains values from runtime wires.
  struct Frame {
    Operation *operation;
    unsigned nextOperand = 0;
  };
  SmallVector<Frame> stack;
  llvm::DenseSet<Operation *> active, complete;
  auto push = [&](Value value) -> LogicalResult {
    auto *operation = value.getDefiningOp();
    if (!operation || operation->getParentOp() != bindings.owner ||
        !isa<BitsConstantOp, EnumCreateOp, StructCreateOp, TableCreateOp,
             TableSplatOp, StructGetOp, EnumToBitsOp, BitsExtractOp,
             BitsResizeOp>(operation))
      return state->emitOpError()
             << "state initializer requires a closed static "
                "literal/constructor "
                "DAG; runtime values and unsupported computations are rejected";
    if (!active.insert(operation).second)
      return state->emitOpError()
             << "state initializer constructor DAG is cyclic";
    if (failed(mlir::verify(operation)) ||
        failed(analysis.verifyResolvedOperation(operation, bindings)))
      return failure();
    stack.push_back({operation});
    return success();
  };
  if (failed(push(initial)))
    return failure();
  while (!stack.empty()) {
    auto &frame = stack.back();
    if (frame.nextOperand == frame.operation->getNumOperands()) {
      complete.insert(frame.operation);
      active.erase(frame.operation);
      stack.pop_back();
      continue;
    }
    Value input = frame.operation->getOperand(frame.nextOperand++);
    if (complete.contains(input.getDefiningOp()))
      continue;
    if (failed(push(input)))
      return failure();
  }
  return success();
}

} // namespace

LogicalResult SourceStateOp::verify() {
  if ((*this)->hasAttr("sym_name"))
    return emitOpError() << "state uses instance_name; sym_name is unsupported";
  if (failed(namedDeclaration(*this, getInstanceNameAttr())))
    return failure();
  auto owner = cast<ModuleOp>((*this)->getParentOp());
  for (Operation &other : owner.getBody().front())
    if (&other != getOperation() &&
        isa<SourceStateOp, InstanceOp, CollectionOp, QueueOp>(other) &&
        other.getAttrOfType<StringAttr>("instance_name") ==
            getInstanceNameAttr())
      return emitOpError() << "state instance_name must be unique across state "
                              "and child instance declarations";
  for (auto other : owner.getBody().front().getOps<SourceStateOp>()) {
    if (other == *this)
      continue;
    if (failed(occurrence(other)))
      return failure();
    if (detail::compareClosedSourceStructure(getOccurrence(),
                                             other.getOccurrence()) == 0)
      return emitOpError() << "state declarations require distinct occurrence "
                              "identities within their module";
  }
  HardwareAnalysis analysis((*this)->getParentOfType<mlir::ModuleOp>());
  auto bindings = scope(*this);
  if (failed(predicate(analysis, getClk(), bindings, *this, "state clock")) ||
      failed(predicate(analysis, getRst(), bindings, *this, "state reset")))
    return failure();
  if (!areEquivalentHardwareTypes(getInitial().getType(), getQ().getType()))
    return emitOpError() << "initial value and old-Q payload types must match";
  return staticInitializer(analysis, getInitial(), bindings, *this);
}

LogicalResult SourceStateReadOp::verify() {
  if (failed(transactionPlacement(*this, true)))
    return failure();
  HardwareAnalysis analysis((*this)->getParentOfType<mlir::ModuleOp>());
  if (failed(predicate(analysis, getPath(), scope(*this), *this,
                       "state read path")))
    return failure();
  auto selected = selectedStateType(*this, getSelectors(), getIndices());
  if (failed(selected))
    return failure();
  if (!areEquivalentHardwareTypes(*selected, getValue().getType()))
    return emitOpError()
           << "state read result must match selected payload type";
  return success();
}

LogicalResult SourceStateWriteOp::verify() {
  if (failed(transactionPlacement(*this, false)))
    return failure();
  HardwareAnalysis analysis((*this)->getParentOfType<mlir::ModuleOp>());
  if (failed(predicate(analysis, getPath(), scope(*this), *this,
                       "state write path")))
    return failure();
  auto selected = selectedStateType(*this, getSelectors(), getIndices());
  if (failed(selected))
    return failure();
  if (!areEquivalentHardwareTypes(*selected, getValue().getType()))
    return emitOpError()
           << "state write value must match selected payload type";
  return success();
}

LogicalResult SourceMethodOp::verify() {
  if (failed(namedDeclaration(*this, getSymNameAttr())))
    return failure();
  auto signature = getFunctionType();
  if (getArgumentNames().size() != signature.getNumInputs())
    return emitOpError() << "argument name count must match method signature";
  llvm::StringSet<> names;
  for (Attribute raw : getArgumentNames()) {
    auto name = cast<StringAttr>(raw);
    if (failed(detail::verifyPythonAstIdentifier(
            name.getValue(), true, [&] { return emitOpError(); })))
      return failure();
    if (!names.insert(name.getValue()).second)
      return emitOpError() << "method argument names must be unique";
  }
  auto form = (*this)->getAttrOfType<StringAttr>("ac.return_form");
  auto parameters = (*this)->getAttrOfType<ArrayAttr>("ac.parameters");
  auto results = (*this)->getAttrOfType<ArrayAttr>("ac.result_constraints");
  if (!form || !parameters || !results ||
      parameters.size() != signature.getNumInputs() ||
      results.size() != signature.getNumResults() ||
      signature.getNumResults() > 1 ||
      form.getValue() != (signature.getNumResults() == 0 ? "none" : "single"))
    return emitOpError()
           << "method source contract requires ac.parameters, "
              "ac.result_constraints and matching none/single ac.return_form";
  auto lexicalModule = (*this)->getParentOp();
  auto error = [&] { return emitOpError(); };
  if (failed(detail::verifySourceOwner(
          lexicalModule->getAttrOfType<DictionaryAttr>("source_owner"), error)))
    return failure();
  for (TypeRange types : {signature.getInputs(), signature.getResults()})
    for (Type type : types) {
      if (!isa<BitsType, StructType, EnumType, TypeParamType, TableType>(type))
        return emitOpError()
               << "method signature requires hardware payload types";
      if (failed(detail::verifyHardwareTypeScope(type, lexicalModule)))
        return failure();
    }
  HardwareAnalysis analysis((*this)->getParentOfType<mlir::ModuleOp>());
  for (auto [index, raw] : llvm::enumerate(parameters)) {
    if (failed(detail::verifyRequiredSourceParameter(
            raw, cast<StringAttr>(getArgumentNames()[index]), lexicalModule,
            *this, "source method")))
      return failure();
    auto parameter = cast<DictionaryAttr>(raw);
    if (failed(detail::verifySourceCallHardwareConstraint(
            parameter.get("constraint"), signature.getInput(index), analysis,
            lexicalModule, *this, "source method")))
      return failure();
  }
  for (auto [index, raw] : llvm::enumerate(results))
    if (failed(detail::verifySourceCallHardwareConstraint(
            raw, signature.getResult(index), analysis, lexicalModule, *this,
            "source method")))
      return failure();
  return success();
}

LogicalResult SourceMethodOp::verifyRegions() {
  SmallVector<Type> arguments(getFunctionType().getInputs());
  llvm::append_range(arguments, getCaptures().getTypes());
  if (failed(region(*this, 0, arguments, {})))
    return failure();
  return region(*this, 1, arguments, getFunctionType().getResults());
}

LogicalResult SourceMethodCallOp::verify() {
  if (failed(transactionPlacement(*this, false)))
    return failure();
  HardwareAnalysis analysis((*this)->getParentOfType<mlir::ModuleOp>());
  if (failed(predicate(analysis, getPath(), scope(*this), *this,
                       "method call path")))
    return failure();
  auto callee = getCalleeAttr();
  if (callee.getNestedReferences().size() != 1)
    return emitOpError() << "method callee must be @Module::@method";
  for (Attribute raw : getInstancePath()) {
    auto name = cast<StringAttr>(raw);
    if (name.getValue().empty() || name.getValue().contains('\0'))
      return emitOpError() << "method instance path requires nonempty names";
  }
  return success();
}

LogicalResult SourceMethodCallOp::verifySymbolUses(SymbolTableCollection &) {
  if ((*this)->getNumOperands() == 0 || (*this)->getNumRegions() != 0)
    return emitOpError() << "method call requires path operand and no regions";
  if (failed(verifyInvariantsImpl()))
    return failure();
  HardwareAnalysis analysis((*this)->getParentOfType<mlir::ModuleOp>());
  auto resolved = resolveSourceMethodCall(*this);
  if (failed(resolved))
    return failure();
  auto bindings = resolved->bindings;
  auto signature = resolved->method.getFunctionType();
  if (getArguments().size() != signature.getNumInputs() ||
      getResults().size() != signature.getNumResults())
    return emitOpError() << "method call arity must match signature";
  auto check = [&](Type actual, Type formal) -> LogicalResult {
    auto expected = analysis.resolveType(formal, bindings, *this);
    auto supplied = analysis.resolveType(actual, scope(*this), *this);
    if (failed(expected) || failed(supplied))
      return failure();
    if (!areEquivalentHardwareTypes(*expected, *supplied))
      return emitOpError() << "method call payload type must match bound "
                              "receiver signature";
    return success();
  };
  for (auto [actual, formal] :
       llvm::zip(getArguments().getTypes(), signature.getInputs()))
    if (failed(check(actual, formal)))
      return failure();
  for (auto [actual, formal] :
       llvm::zip(getResultTypes(), signature.getResults()))
    if (failed(check(actual, formal)))
      return failure();
  return success();
}

LogicalResult SourceTransactionOp::verify() {
  if ((*this)->hasAttr("sym_name"))
    return emitOpError() << "transaction uses name; sym_name is unsupported";
  if (failed(namedDeclaration(*this, getNameAttr())))
    return failure();
  if (failed(
          moduleOccurrence(*this, getDeclaration(), "transaction declaration")))
    return failure();
  auto owner = cast<ModuleOp>((*this)->getParentOp());
  for (auto other : owner.getBody().front().getOps<SourceTransactionOp>()) {
    if (other == *this)
      continue;
    if (failed(moduleOccurrence(other, other.getDeclaration(),
                                "transaction declaration")))
      return failure();
    if (detail::compareClosedSourceStructure(getDeclaration(),
                                             other.getDeclaration()) == 0)
      return emitOpError() << "transaction declaration must be registered "
                              "exactly once within its module";
  }
  HardwareAnalysis analysis((*this)->getParentOfType<mlir::ModuleOp>());
  auto bindings = scope(*this);
  if (failed(predicate(analysis, getClk(), bindings, *this,
                       "transaction clock")) ||
      failed(
          predicate(analysis, getRst(), bindings, *this, "transaction reset")))
    return failure();
  return success();
}

LogicalResult SourceTransactionOp::verifyRegions() {
  if (failed(region(*this, 0, getCaptures().getTypes(), {})))
    return failure();
  return region(*this, 1, getCaptures().getTypes(), getResultTypes());
}

LogicalResult detail::verifyNoUnloweredTransactions(Operation *root) {
  auto walk = root->walk([&](Operation *operation) {
    if (!isa<SourceStateOp, SourceStateReadOp, SourceStateWriteOp,
             SourceMethodOp, SourceMethodCallOp, SourceTransactionOp>(
            operation))
      return WalkResult::advance();
    operation->emitOpError()
        << "unlowered source transaction operation cannot cross the verified "
           "hardware boundary";
    return WalkResult::interrupt();
  });
  return failure(walk.wasInterrupted());
}

} // namespace acir::ac
