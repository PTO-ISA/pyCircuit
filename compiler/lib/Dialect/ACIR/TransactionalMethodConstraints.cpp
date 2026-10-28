#include "mlir/IR/Builders.h"
#include "pycircuit/Dialect/ACIR/TransactionalContracts.h"
#include "llvm/ADT/DenseSet.h"
#include "llvm/ADT/SmallPtrSet.h"
#include <functional>

using namespace mlir;
namespace acir::ac {
namespace {
Operation *provider(mlir::ModuleOp package, Value receiver) {
  auto ref = dyn_cast<ModuleRefType>(receiver.getType());
  if (!ref)
    return nullptr;
  for (Operation &op : *package.getBody())
    if (auto name = op.getAttrOfType<StringAttr>("sym_name"))
      if (name.getValue() == ref.getModule().getValue())
        return &op;
  return nullptr;
}
ArrayAttr types(Builder &b, TypeRange values) {
  SmallVector<Attribute> result;
  for (Type value : values)
    result.push_back(TypeAttr::get(value));
  return b.getArrayAttr(result);
}
bool compatible(Type a, Type b) {
  return isa<InferType>(a) || isa<InferType>(b) ||
         transactionalTypesEqual(a, b);
}
} // namespace
LogicalResult resolveTransactionalMethodConstraints(mlir::ModuleOp package,
                                                    bool requireClosed) {
  Builder b(package.getContext());
  SmallVector<TransactionMethodPendingOp> pending;
  package.walk(
      [&](TransactionMethodPendingOp call) { pending.push_back(call); });
  // Requirements are derived from current SSA each time, so newly solved
  // payload types are published without preserving stale capture-time guesses.
  llvm::SmallPtrSet<Operation *, 8> localRequirements;
  for (auto call : pending) {
    auto *target = provider(package, call.getReceiver());
    if (isTransactionalReferenceRequirement(target))
      localRequirements.insert(target);
  }
  for (Operation *target : localRequirements)
    target->setAttr("ac.methods", b.getArrayAttr({}));
  for (auto call : pending) {
    auto *target = provider(package, call.getReceiver());
    if (!target)
      return call.emitOpError("Method receiver has no provider declaration");
    if (isTransactionalReferenceRequirement(target)) {
      if (requireClosed)
        return call.emitOpError("Method provider remains unresolved");
      SmallVector<Attribute> entries;
      bool found = false;
      SmallVector<Attribute> resultContexts;
      for (Value result : call.getResults()) {
        SmallVector<Attribute> contexts;
        llvm::DenseSet<Value> visited;
        std::function<void(Value)> collect = [&](Value value) {
          if (!visited.insert(value).second)
            return;
          for (OpOperand &use : value.getUses()) {
            Operation *user = use.getOwner();
            if (auto conversion = dyn_cast<PendingExprOp>(user)) {
              if (conversion.getKind() == "convert")
                contexts.push_back(
                    TypeAttr::get(conversion.getResult().getType()));
              continue;
            }
            if (!isa<TransactionRuleOp, TransactionMethodOp>(user))
              continue;
            auto sig = cast<FunctionType>(
                user->getAttrOfType<TypeAttr>("function_type").getValue());
            unsigned offset =
                sig.getNumInputs() +
                (isa<TransactionRuleOp>(user) ? sig.getNumResults() : 0);
            for (Region &region : user->getRegions())
              if (!region.empty() && offset + use.getOperandNumber() <
                                         region.front().getNumArguments())
                collect(region.front().getArgument(offset +
                                                   use.getOperandNumber()));
          }
        };
        collect(result);
        resultContexts.push_back(b.getArrayAttr(contexts));
      }
      auto constraint = b.getDictionaryAttr(
          {b.getNamedAttr("inputs", types(b, call.getArguments().getTypes())),
           b.getNamedAttr("results", types(b, call.getResultTypes())),
           b.getNamedAttr("result_requirement",
                          call.getResultRequirementAttr()),
           b.getNamedAttr("result_contexts", b.getArrayAttr(resultContexts))});
      for (Attribute raw : getTransactionalMethodInterfaces(target)) {
        auto entry = cast<DictionaryAttr>(raw);
        if (entry.getAs<StringAttr>("name").getValue() ==
            call.getCalleeAttr().getRootReference().getValue()) {
          NamedAttrList fields(entry);
          SmallVector<Attribute> calls(
              entry.getAs<ArrayAttr>("calls").getValue());
          for (Attribute previous : calls) {
            auto prior = cast<DictionaryAttr>(previous);
            auto priorInputs = prior.getAs<ArrayAttr>("inputs");
            auto currentInputs = constraint.getAs<ArrayAttr>("inputs");
            auto priorDemand =
                prior.getAs<StringAttr>("result_requirement").getValue();
            if (priorInputs.size() != currentInputs.size() ||
                (priorDemand != "ignored" &&
                 call.getResultRequirement() != "ignored" &&
                 prior.getAs<ArrayAttr>("results").size() !=
                     call.getNumResults()))
              return call.emitOpError("inconsistent Method call arities");
            for (auto [left, right] : llvm::zip(priorInputs, currentInputs)) {
              auto a = cast<TypeAttr>(left).getValue();
              auto z = cast<TypeAttr>(right).getValue();
              if (!compatible(a, z) && !(isa<BitsType>(a) && isa<BitsType>(z)))
                return call.emitOpError(
                    "incompatible nominal Method argument constraints");
            }
          }
          calls.push_back(constraint);
          fields.set("calls", b.getArrayAttr(calls));
          fields.set(
              "read_only",
              b.getBoolAttr(entry.getAs<BoolAttr>("read_only").getValue() ||
                            call.getReadOnly()));
          raw = fields.getDictionary(package.getContext());
          found = true;
        }
        entries.push_back(raw);
      }
      if (!found)
        entries.push_back(b.getDictionaryAttr(
            {b.getNamedAttr(
                 "name",
                 b.getStringAttr(
                     call.getCalleeAttr().getRootReference().getValue())),
             b.getNamedAttr("read_only", call.getReadOnlyAttr()),
             b.getNamedAttr("calls", b.getArrayAttr({constraint}))}));
      target->setAttr("ac.methods", b.getArrayAttr(entries));
      continue;
    }
    DictionaryAttr method;
    for (Attribute raw : getTransactionalMethodInterfaces(target)) {
      auto entry = cast<DictionaryAttr>(raw);
      if (entry.getAs<StringAttr>("name").getValue() ==
          call.getCalleeAttr().getRootReference().getValue())
        method = entry;
    }
    if (!method)
      return call.emitOpError("provider lacks required Method");
    auto sig =
        cast<FunctionType>(method.getAs<TypeAttr>("function_type").getValue());
    auto demand = call.getResultRequirement();
    if (call.getArguments().size() != sig.getNumInputs() ||
        (demand != "ignored" && call.getNumResults() != sig.getNumResults()) ||
        (call.getReadOnly() && !method.getAs<BoolAttr>("read_only").getValue()))
      return call.emitOpError(
          "provider Method does not satisfy call requirements");
    for (auto [value, type] : llvm::zip(call.getResults(), sig.getResults()))
      if (!compatible(value.getType(), type))
        return call.emitOpError(
            "Method result payload conflicts with provider");
    OpBuilder builder(call);
    OperationState state(call.getLoc(), "ac.method.call");
    state.addOperands(call.getReceiver());
    for (auto [value, type] : llvm::zip(call.getArguments(), sig.getInputs())) {
      if (!transactionalTypesEqual(value.getType(), type)) {
        OperationState conversion(call.getLoc(), "ac.payload.pending");
        conversion.addOperands(value);
        conversion.addTypes(type);
        conversion.addAttribute("kind", b.getStringAttr("convert"));
        value = builder.create(conversion)->getResult(0);
      }
      state.addOperands(value);
    }
    state.addTypes(sig.getResults());
    state.addAttribute("callee", call.getCalleeAttr());
    if (auto occurrence = call->getAttr("occurrence"))
      state.addAttribute("occurrence", occurrence);
    auto *resolved = builder.create(state);
    for (auto [oldValue, newValue] :
         llvm::zip(call.getResults(), resolved->getResults()))
      oldValue.replaceAllUsesWith(newValue);
    call.erase();
  }
  return success();
}
LogicalResult TransactionMethodPendingOp::verify() {
  auto demand = getResultRequirement();
  if (!isTransactionalPackage(*this) || isLinkedTransactionalPackage(*this) ||
      !isa<ModuleRefType>(getReceiver().getType()) ||
      (demand != "ignored" && demand != "scalar" && demand != "tuple") ||
      (demand == "ignored" && getNumResults()) ||
      (demand == "scalar" && getNumResults() != 1))
    return emitOpError(
        "requires an unlinked Module Method use with explicit result demand");
  return success();
}
} // namespace acir::ac
