#include "ACIRSourceContracts.h"
#include "mlir/IR/Builders.h"
#include "mlir/IR/SymbolTable.h"
#include "pycircuit/Dialect/ACIR/SourceIdentifier.h"
#include "pycircuit/Dialect/ACIR/SourceMethodAnalysis.h"
#include "llvm/ADT/DenseSet.h"
#include "llvm/ADT/StringSet.h"

using namespace mlir;
namespace acir::ac {
namespace {
// Symbol-use checks run before child custom verifiers. Validate only the
// receiver header here; never recurse into body verification or Method calls.
LogicalResult receiverHeader(Operation *module, Operation *site) {
  if (!isa_and_nonnull<ModuleOp, ModuleImportOp>(module))
    return site->emitOpError() << "Method receiver must resolve to a module";
  if (auto body = dyn_cast<ModuleOp>(module)) {
    if (body->getNumRegions() != 1 || !body->getRegion(0).hasOneBlock())
      return site->emitOpError()
             << "Method receiver requires one module body block";
    if (failed(body.verifyInvariantsImpl()))
      return failure();
    return body.verify();
  }
  auto imported = cast<ModuleImportOp>(module);
  if (imported->getNumRegions() != 0)
    return site->emitOpError()
           << "imported Method receiver cannot have regions";
  if (failed(imported.verifyInvariantsImpl()))
    return failure();
  return imported.verify();
}
} // namespace
FailureOr<const SourceReceiverProofContext::Inventory *>
SourceReceiverProofContext::prepare(Operation *module, Operation *site,
                                    const SourceReceiverProofFees &fees) {
  uint64_t lookupWork = 1;
  for (size_t n = complete.size(); n; n >>= 1)
    lookupWork += 2;
  if (failed(fees.workStorage(site, lookupWork, 0)))
    return failure();
  auto found = complete.find(module);
  if (found != complete.end())
    return &found->second;
  if (!isa_and_nonnull<ModuleOp, ModuleImportOp>(module))
    return site->emitOpError() << "Method receiver must resolve to a module";
  if (failed(fees.moduleMiss(module, site)) ||
      failed(receiverHeader(module, site)))
    return failure();
  Inventory inventory;
  auto body = dyn_cast<ModuleOp>(module);
  DictionaryAttr summary;
  if (body) {
    for (Operation &op : body.getBody().front()) {
      if (failed(fees.workStorage(site, 1, 0)))
        return failure();
      inventory.instanceCount += isa<InstanceOp>(op);
      inventory.methodCount += isa<SourceMethodOp>(op);
    }
  } else {
    summary = module->getAttrOfType<DictionaryAttr>("transaction_summary");
    if (summary) {
      if (failed(verifySourceTransactionSummary(module, summary)))
        return failure();
      inventory.hasSummary = true;
      for (Attribute raw : summary.getAs<ArrayAttr>("owners")) {
        if (failed(fees.workStorage(site, 1, 0)))
          return failure();
        inventory.instanceCount +=
            cast<DictionaryAttr>(raw).getAs<StringAttr>("kind").getValue() ==
            "instance";
      }
      inventory.methodCount = summary.getAs<ArrayAttr>("methods").size();
    }
  }
  uint64_t bytes = inventory.instanceCount * sizeof(Instance) +
                   inventory.methodCount * sizeof(SourceMethodView);
  if (failed(fees.workStorage(
          site, inventory.instanceCount + inventory.methodCount, bytes)))
    return failure();
  if (inventory.instanceCount)
    inventory.instances = std::make_unique<Instance[]>(inventory.instanceCount);
  if (inventory.methodCount)
    inventory.methods =
        std::make_unique<SourceMethodView[]>(inventory.methodCount);
  size_t instanceIndex = 0, methodIndex = 0;
  auto instance = [&](StringAttr name, DictionaryAttr occurrence,
                      Operation *allocation,
                      DictionaryAttr row) -> LogicalResult {
    if (!name || !occurrence ||
        failed(detail::verifyOccurrence(occurrence,
                                        [&] { return site->emitOpError(); })) ||
        failed(detail::verifyOriginDefinition(
            occurrence,
            FlatSymbolRefAttr::get(SymbolTable::getSymbolName(module)),
            "Method receiver instance", [&] { return site->emitOpError(); })))
      return failure();
    for (size_t i = 0; i < instanceIndex; ++i) {
      if (failed(fees.workStorage(site, 3, 0)))
        return failure();
      if (inventory.instances[i].name == name ||
          inventory.instances[i].occurrence == occurrence)
        return site->emitOpError()
               << "Method receiver instances require distinct names and "
                  "occurrence identities";
    }
    inventory.instances[instanceIndex++] = {name, occurrence, allocation, row};
    return success();
  };
  if (body) {
    for (Operation &op : body.getBody().front()) {
      if (failed(fees.workStorage(site, 1, 0)))
        return failure();
      if (auto allocation = dyn_cast<InstanceOp>(op)) {
        if (failed(allocation.verifyInvariantsImpl()) ||
            failed(allocation.verify()) ||
            failed(instance(allocation.getInstanceNameAttr(),
                            allocation.getOccurrence(), allocation, {})))
          return failure();
      } else if (auto method = dyn_cast<SourceMethodOp>(op)) {
        if (method->getNumRegions() != 2)
          return method.emitOpError()
                 << "method requires guard and body regions";
        for (Region &region : method->getRegions())
          if (!region.hasOneBlock() || region.front().empty())
            return method.emitOpError()
                   << "method guard and body require one nonempty block";
        if (failed(method.verifyInvariantsImpl()))
          return failure();
        inventory.methods[methodIndex++] = {module, method, {}};
      }
    }
  } else if (summary) {
    for (Attribute raw : summary.getAs<ArrayAttr>("owners")) {
      auto row = cast<DictionaryAttr>(raw);
      if (row.getAs<StringAttr>("kind").getValue() == "instance" &&
          failed(instance(row.getAs<StringAttr>("name"),
                          row.getAs<DictionaryAttr>("occurrence"), nullptr,
                          row)))
        return failure();
    }
    for (Attribute raw : summary.getAs<ArrayAttr>("methods"))
      inventory.methods[methodIndex++] = {
          module, {}, cast<DictionaryAttr>(raw)};
  }
  // A failed proof or payment publishes neither an entry nor any IR changes.
  if (failed(fees.workStorage(site, lookupWork + 1,
                              sizeof(decltype(complete)::value_type) +
                                  4 * sizeof(void *))))
    return failure();
  return &complete.emplace(module, std::move(inventory)).first->second;
}

FailureOr<SourceMethodView>
SourceReceiverProofContext::method(Operation *module, StringRef name,
                                   Operation *site,
                                   const SourceReceiverProofFees &fees) {
  auto inventory = prepare(module, site, fees);
  if (failed(inventory))
    return failure();
  if (isa<ModuleImportOp>(module) && !(**inventory).hasSummary)
    return site->emitOpError() << "imported Method receiver requires a "
                                  "verified transaction summary";
  SourceMethodView selected{module, {}, {}};
  for (size_t i = 0; i < (**inventory).methodCount; ++i) {
    if (failed(fees.workStorage(site, 1, 0)))
      return failure();
    auto candidate = (**inventory).methods[i];
    auto candidateName =
        candidate.definition
            ? candidate.definition.getSymName()
            : candidate.row.getAs<StringAttr>("name").getValue();
    if (candidateName != name)
      continue;
    if (selected.definition || selected.row)
      return site->emitOpError() << "callee receiver method is ambiguous";
    if (candidate.definition &&
        (failed(fees.selectedMethod(candidate.definition, site)) ||
         failed(candidate.definition.verify())))
      return failure();
    selected = candidate;
  }
  if (!selected.definition && !selected.row)
    return site->emitOpError()
           << "callee must resolve to a direct receiver method";
  return selected;
}

Type SourceQueueView::getPayloadType() const {
  if (allocation)
    return allocation->getResult(2).getType();
  return cast<TypeAttr>(row.getAs<ArrayAttr>("result_types")[2]).getValue();
}
Attribute SourceQueueView::getPayloadConstraint() const {
  return allocation ? allocation->getAttr("ac.payload_constraint")
                    : row.get("payload_constraint");
}
DictionaryAttr SourceQueueView::getOccurrence() const {
  return row.getAs<DictionaryAttr>("occurrence");
}
DictionaryAttr SourceQueueView::getDomain() const {
  return row.getAs<DictionaryAttr>("domain");
}
DictionaryAttr SourceQueueView::getConfig() const {
  if (!allocation)
    return {};
  auto config = getQueueConfiguration(allocation);
  return succeeded(config) ? *config : DictionaryAttr();
}
bool SourceQueueView::isPushUnbound() const {
  auto value =
      allocation ? allocation->getAttrOfType<BoolAttr>("ac.source_unbound_push")
                 : row.getAs<BoolAttr>("unbound_push");
  return value && value.getValue();
}
bool SourceQueueView::isPopUnbound() const {
  auto value =
      allocation ? allocation->getAttrOfType<BoolAttr>("ac.source_unbound_pop")
                 : row.getAs<BoolAttr>("unbound_pop");
  return value && value.getValue();
}
FailureOr<DictionaryAttr> getSourceQueueOwnerRow(InstanceOp queue) {
  if (!isQueueInstance(queue))
    return queue.emitOpError() << "source queue owner requires a canonical FIFO instance";
  if (failed(queue.verifyInvariantsImpl()) || failed(queue.verify()))
    return failure();
  auto owner = dyn_cast_or_null<ModuleOp>(queue->getParentOp());
  if (!owner)
    return queue.emitOpError() << "queue owner requires direct module placement";
  // Source publication and effects require a unique actual allocation identity.
  for (Operation &other : owner.getBody().front())
    if (&other != queue.getOperation() &&
        isa<SourceStateOp, InstanceOp, CollectionOp>(other) &&
        other.getAttrOfType<DictionaryAttr>("occurrence") == queue.getOccurrence())
      return queue.emitOpError() << "source queue allocations require distinct occurrence identities";
  Builder builder(queue->getContext());
  HardwareAnalysis hardware(queue->getParentOfType<mlir::ModuleOp>());
  HardwareBindings bindings;
  bindings.owner = owner;
  auto endpoint = [&](Value value) -> FailureOr<DictionaryAttr> {
    if (auto argument = dyn_cast<BlockArgument>(value)) {
      if (argument.getOwner()->getParentOp() == owner)
        return builder.getDictionaryAttr({
            builder.getNamedAttr("kind", builder.getStringAttr("input")),
            builder.getNamedAttr("ordinal", builder.getI64IntegerAttr(argument.getArgNumber()))});
    }
    if (auto constant = value.getDefiningOp<BitsConstantOp>()) {
      auto literal = hardware.evaluateStatic(constant.getValue(), bindings, queue);
      auto number = succeeded(literal) ? dyn_cast<MathIntAttr>(*literal) : MathIntAttr();
      if (number) {
        llvm::APSInt integer(number.getCanonicalValue());
        if (integer.isZero() || integer == 1)
          return builder.getDictionaryAttr({
              builder.getNamedAttr("kind", builder.getStringAttr("constant")),
              builder.getNamedAttr("value", builder.getBoolAttr(!integer.isZero()))});
      }
    }
    return queue.emitOpError() << "queue domain requires a direct one-bit module input or literal";
  };
  auto clock = endpoint(queue->getOperand(0)),
       reset = endpoint(queue->getOperand(1));
  if (failed(clock) || failed(reset))
    return failure();
  NamedAttrList fields({
      builder.getNamedAttr("kind", builder.getStringAttr("instance")),
      builder.getNamedAttr("name", queue.getInstanceNameAttr()),
      builder.getNamedAttr("occurrence", queue.getOccurrence()),
      builder.getNamedAttr("callee", queue.getCalleeAttr()),
      builder.getNamedAttr("parameters", queue.getParameters()),
      builder.getNamedAttr("type_arguments", queue.getTypeArguments()),
      builder.getNamedAttr("input_types", builder.getArrayAttr(llvm::to_vector(
          llvm::map_range(queue.getOperandTypes(), [](Type type) {
            return Attribute(TypeAttr::get(type));
          })))),
      builder.getNamedAttr("result_types", builder.getArrayAttr(llvm::to_vector(
          llvm::map_range(queue.getResultTypes(), [](Type type) {
            return Attribute(TypeAttr::get(type));
          }))))});
  if (Attribute constraint = queue->getAttr("ac.payload_constraint"))
    fields.append("payload_constraint", constraint);
  if (queue->hasAttr("ac.source_unbound_push"))
    fields.append("unbound_push", builder.getBoolAttr(true));
  if (queue->hasAttr("ac.source_unbound_pop"))
    fields.append("unbound_pop", builder.getBoolAttr(true));
  return fields.getDictionary(queue->getContext());
}
FailureOr<SourceQueueView> lookupSourceQueue(Operation *module, StringRef name,
                                            Operation *site) {
  if (failed(receiverHeader(module, site)))
    return failure();
  SourceQueueView result{module, {}, {}};
  if (auto body = dyn_cast<ModuleOp>(module)) {
    for (auto queue : body.getBody().front().getOps<InstanceOp>()) {
      if (!isQueueInstance(queue))
        continue;
      if (queue.getInstanceName() != name)
        continue;
      if (result.row)
        return site->emitOpError() << "queue allocation name is ambiguous";
      auto row = getSourceQueueOwnerRow(queue);
      if (failed(row))
        return failure();
      result.allocation = queue;
      result.row = *row;
    }
  } else {
    auto summary = module->getAttrOfType<DictionaryAttr>("transaction_summary");
    if (!summary || failed(verifySourceTransactionSummary(module, summary)))
      return site->emitOpError() << "imported queue requires a verified transaction summary";
    for (Attribute raw : summary.getAs<ArrayAttr>("owners")) {
      auto row = cast<DictionaryAttr>(raw);
      if (row.getAs<StringAttr>("kind").getValue() == "instance" &&
          (row.getAs<FlatSymbolRefAttr>("callee").getValue() ==
               "pycircuit.__builtins__.fifo_local" ||
           row.getAs<FlatSymbolRefAttr>("callee").getValue() ==
               "pycircuit.__builtins__.fifo_downstream_pop") &&
          row.getAs<StringAttr>("name").getValue() == name) {
        if (result.row)
          return site->emitOpError() << "queue allocation name is ambiguous";
        result.row = row;
      }
    }
  }
  if (!result.row)
    return site->emitOpError() << "queue effect must identify an actual lexical allocation";
  return result;
}
FunctionType SourceMethodView::getFunctionType() const {
  if (isNativeQueueTerminal()) {
    auto moduleType = cast<FunctionType>(
        module->getAttrOfType<TypeAttr>("function_type").getValue());
    Type formalPayload = moduleType.getInput(3);
    SmallVector<Type> inputs, results;
    (nativeQueueAction == NativeQueueAction::Push ? inputs : results)
        .push_back(formalPayload);
    return FunctionType::get(module->getContext(), inputs, results);
  }
  return definition ? cast<FunctionType>(
                          definition->getAttrOfType<TypeAttr>("function_type")
                              .getValue())
                    : cast<FunctionType>(
                          row.getAs<TypeAttr>("function_type").getValue());
}
ArrayAttr SourceMethodView::getArgumentNames() const {
  if (isNativeQueueTerminal()) {
    Builder builder(module->getContext());
    return nativeQueueAction == NativeQueueAction::Push
               ? builder.getArrayAttr({builder.getStringAttr("value")})
               : builder.getArrayAttr({});
  }
  return definition ? definition->getAttrOfType<ArrayAttr>("argument_names")
                    : row.getAs<ArrayAttr>("argument_names");
}
ArrayAttr SourceMethodView::getParameters() const {
  if (isNativeQueueTerminal()) {
    Builder builder(module->getContext());
    if (nativeQueueAction != NativeQueueAction::Push)
      return builder.getArrayAttr({});
    return builder.getArrayAttr({builder.getDictionaryAttr({
        builder.getNamedAttr("name", builder.getStringAttr("value")),
        builder.getNamedAttr("binding",
                             builder.getStringAttr("positional_or_keyword")),
        builder.getNamedAttr("constraint", nativeQueuePayloadConstraint),
        builder.getNamedAttr(
            "default",
            builder.getDictionaryAttr({builder.getNamedAttr(
                "present", builder.getBoolAttr(false))}))})});
  }
  return definition ? definition->getAttrOfType<ArrayAttr>("ac.parameters")
                    : row.getAs<ArrayAttr>("ac.parameters");
}
ArrayAttr SourceMethodView::getResultConstraints() const {
  if (isNativeQueueTerminal()) {
    Builder builder(module->getContext());
    return nativeQueueAction == NativeQueueAction::Push
               ? builder.getArrayAttr({})
               : builder.getArrayAttr({nativeQueuePayloadConstraint});
  }
  return definition
             ? definition->getAttrOfType<ArrayAttr>("ac.result_constraints")
             : row.getAs<ArrayAttr>("ac.result_constraints");
}
ArrayAttr SourceMethodView::getFormalProposals() const {
  if (isNativeQueueTerminal())
    return {};
  return definition
             ? definition->getAttrOfType<ArrayAttr>("ac.formal_proposals")
             : row.getAs<ArrayAttr>("ac.formal_proposals");
}
unsigned SourceMethodView::getNumPublicResults() const {
  auto proposals = getFormalProposals();
  return getFunctionType().getNumResults() -
         2 * (proposals ? proposals.size() : 0);
}
LogicalResult
verifySourceMethodFormalProposals(const SourceMethodView &method,
                                  Operation *site) {
  Attribute raw = method.definition
                      ? method.definition->getAttr("ac.formal_proposals")
                      : method.row.get("ac.formal_proposals");
  if (!raw)
    return success();
  auto proposals = dyn_cast<ArrayAttr>(raw);
  auto signature = method.getFunctionType();
  auto parameters = method.getParameters();
  auto results = method.getResultConstraints();
  if (!proposals || !parameters || !results ||
      parameters.size() != signature.getNumInputs() ||
      results.size() != signature.getNumResults() ||
      proposals.size() > signature.getNumResults() / 2)
    return site->emitOpError()
           << "method formal proposals require input indices and trailing "
              "[data, intent] result pairs";
  unsigned publicCount = signature.getNumResults() - 2 * proposals.size();
  HardwareAnalysis hardware(method.module->getParentOfType<mlir::ModuleOp>());
  HardwareBindings bindings;
  bindings.owner = method.module;
  int64_t previous = -1;
  for (auto [ordinal, entry] : llvm::enumerate(proposals)) {
    auto index = dyn_cast<IntegerAttr>(entry);
    if (!index || !index.getType().isSignlessInteger(64) ||
        index.getInt() <= previous || index.getInt() < 0 ||
        static_cast<uint64_t>(index.getInt()) >= signature.getNumInputs())
      return site->emitOpError()
             << "method formal proposal indices must be sorted unique "
                "nonnegative i64 input indices";
    previous = index.getInt();
    unsigned dataIndex = publicCount + 2 * ordinal;
    unsigned intentIndex = dataIndex + 1;
    auto parameter = dyn_cast<DictionaryAttr>(parameters[previous]);
    auto input = parameter
                     ? parameter.getAs<DictionaryAttr>("constraint")
                     : DictionaryAttr();
    auto data = dyn_cast<DictionaryAttr>(results[dataIndex]);
    auto intent = dyn_cast<DictionaryAttr>(results[intentIndex]);
    if (!input || !data || !intent ||
        !areEquivalentHardwareTypes(signature.getInput(previous),
                                    signature.getResult(dataIndex)))
      return site->emitOpError()
             << "method formal proposal data must match its input contract";
    // Static expression origins may differ; source kind and logical domain
    // remain exact rather than being inferred from physical bit width.
    NamedAttrList inputFields(input), dataFields(data);
    inputFields.erase("type");
    dataFields.erase("type");
    if (inputFields.getDictionary(site->getContext()) !=
        dataFields.getDictionary(site->getContext()))
      return site->emitOpError()
             << "method formal proposal data must match its input source "
                "kind and logical domain";
    auto intentKind = intent.getAs<StringAttr>("source_kind");
    auto intentType = dyn_cast<BitsType>(signature.getResult(intentIndex));
    if (!intentKind || intentKind.getValue() != "fixed_bits" || !intentType)
      return site->emitOpError()
             << "method formal proposal intent requires fixed one-bit Bits";
    auto width = hardware.getPackedWidth(intentType, bindings, site);
    if (failed(width))
      return failure();
    if (*width != 1)
      return site->emitOpError()
             << "method formal proposal intent requires fixed one-bit Bits";
  }
  return success();
}
LogicalResult verifySourceFormalOwnerBindings(
    Attribute raw, unsigned argumentCount,
    std::optional<ArrayAttr> calleeProposals, FunctionType callerSignature,
    ArrayAttr callerProposals,
    llvm::function_ref<bool(StringRef)> hasCallerState, Operation *site) {
  if (!raw)
    return success();
  auto rows = dyn_cast<ArrayAttr>(raw);
  if (!rows)
    return site->emitOpError() << "formal owner bindings require an array";
  int64_t previous = -1;
  llvm::StringSet<> stateTargets;
  llvm::DenseSet<int64_t> formalTargets;
  for (Attribute entry : rows) {
    auto row = dyn_cast<DictionaryAttr>(entry);
    auto index = row ? row.getAs<IntegerAttr>("formal") : IntegerAttr();
    if (!row || row.size() != 2 || !index ||
        !index.getType().isSignlessInteger(64) || index.getInt() < 0 ||
        index.getInt() <= previous ||
        static_cast<uint64_t>(index.getInt()) >= argumentCount)
      return site->emitOpError()
             << "formal owner bindings require sorted unique i64 input indices";
    previous = index.getInt();
    if (calleeProposals &&
        (!*calleeProposals || !llvm::is_contained(*calleeProposals, index)))
      return site->emitOpError()
             << "owner binding must identify a callee writable formal";
    if (auto state = row.getAs<StringAttr>("state")) {
      if (!stateTargets.insert(state.getValue()).second)
        return site->emitOpError() << "writable owner bindings overlap the same caller state";
      if (failed(detail::verifyPythonAstIdentifier(
              state.getValue(), true, [&] { return site->emitOpError(); })) ||
          !hasCallerState(state.getValue()))
        return site->emitOpError()
               << "formal owner binding requires a caller lexical state";
    } else if (auto formal = row.getAs<IntegerAttr>("caller_formal")) {
      if (!formal.getType().isSignlessInteger(64) || formal.getInt() < 0 ||
          !callerSignature || static_cast<uint64_t>(formal.getInt()) >=
                                  callerSignature.getNumInputs() ||
          !callerProposals || !llvm::is_contained(callerProposals, formal))
        return site->emitOpError()
               << "forwarded owner binding requires a caller writable formal";
      if (!formalTargets.insert(formal.getInt()).second)
        return site->emitOpError() << "writable owner bindings overlap the same caller formal";
    } else
      return site->emitOpError()
             << "formal owner binding requires state or caller_formal";
  }
  return success();
}
StringAttr SourceMethodView::getReturnForm() const {
  if (isNativeQueueTerminal())
    return StringAttr::get(module->getContext(),
                           nativeQueueAction == NativeQueueAction::Push
                               ? "none"
                               : "single");
  return definition ? definition->getAttrOfType<StringAttr>("ac.return_form")
                    : row.getAs<StringAttr>("ac.return_form");
}
StringAttr SourceMethodView::getVisibility() const {
  if (isNativeQueueTerminal())
    return StringAttr::get(module->getContext(), "public");
  return definition ? definition->getAttrOfType<StringAttr>("sym_visibility")
                    : row.getAs<StringAttr>("visibility");
}
DictionaryAttr SourceMethodView::getDeclaration() const {
  if (isNativeQueueTerminal())
    return nativeQueueInstance
               ? nativeQueueInstance->getAttrOfType<DictionaryAttr>(
                     "occurrence")
                               : nativeQueueOwnerRow.getAs<DictionaryAttr>(
                                     "occurrence");
  return definition ? definition->getAttrOfType<DictionaryAttr>("occurrence")
                    : row.getAs<DictionaryAttr>("declaration");
}
DictionaryAttr SourceMethodView::getGuardLocation() const {
  if (isNativeQueueTerminal())
    return {};
  return definition
             ? definition->getAttrOfType<DictionaryAttr>("ac.guard_location")
             : row.getAs<DictionaryAttr>("guard_location");
}
SymbolRefAttr SourceMethodView::getSymbol() const {
  if (isNativeQueueTerminal()) {
    StringRef name = nativeQueueAction == NativeQueueAction::Push
                         ? "push"
                         : nativeQueueAction == NativeQueueAction::Pop ? "pop"
                                                                       : "peek";
    return SymbolRefAttr::get(module->getContext(),
                              SymbolTable::getSymbolName(module).getValue(),
                              {FlatSymbolRefAttr::get(module->getContext(),
                                                     name)});
  }
  auto name = definition ? definition->getAttrOfType<StringAttr>("sym_name")
                         : row.getAs<StringAttr>("name");
  return SymbolRefAttr::get(module->getContext(),
                            SymbolTable::getSymbolName(module).getValue(),
                            {FlatSymbolRefAttr::get(name)});
}
FailureOr<SourceMethodView>
lookupSourceMethod(Operation *module, StringRef name, Operation *site) {
  if (failed(receiverHeader(module, site)))
    return failure();
  SourceMethodView result{module, {}, {}};
  if (auto body = dyn_cast<ModuleOp>(module)) {
    if (!body.getBody().hasOneBlock())
      return site->emitOpError()
             << "Method receiver requires one module body block";
    for (auto candidate : body.getBody().front().getOps<SourceMethodOp>()) {
      if (candidate->getNumRegions() != 2)
        return candidate.emitOpError()
               << "method requires guard and body regions";
      for (Region &region : candidate->getRegions())
        if (!region.hasOneBlock() || region.front().empty())
          return candidate.emitOpError()
                 << "method guard and body require one nonempty block";
      if (failed(candidate.verifyInvariantsImpl()))
        return failure();
      if (candidate.getSymName() == name) {
        if (result.definition)
          return site->emitOpError() << "callee receiver method is ambiguous";
        if (failed(candidate.verify()))
          return failure();
        result.definition = candidate;
      }
    }
  } else {
    auto summary = module->getAttrOfType<DictionaryAttr>("transaction_summary");
    if (!summary || failed(verifySourceTransactionSummary(module, summary)))
      return site->emitOpError() << "imported Method receiver requires a "
                                    "verified transaction summary";
    for (auto raw : summary.getAs<ArrayAttr>("methods")) {
      auto candidate = cast<DictionaryAttr>(raw);
      if (candidate.getAs<StringAttr>("name").getValue() == name) {
        if (result.row)
          return site->emitOpError() << "callee receiver method is ambiguous";
        result.row = candidate;
      }
    }
  }
  if (!result.definition && !result.row)
    return site->emitOpError()
           << "callee must resolve to a direct receiver method";
  return result;
}
namespace {
LogicalResult admitDependency(Operation *caller, Operation *target,
                              Operation *site) {
  auto provider = target->getAttrOfType<DictionaryAttr>("source_owner");
  // Native generic packages have no source envelope and retain native
  // admission.
  auto package = caller->getParentOfType<mlir::ModuleOp>();
  auto interfaces = package ? package->getAttrOfType<ArrayAttr>("ac.interfaces")
                            : ArrayAttr();
  // Source-header callers supply canonical authorities after validating every
  // primitive snapshot against the builtin catalog. Native callers establish
  // only structural legality; publication still requires that registry check.
  bool primitive =
      isa<ModuleImportOp>(target) && target->hasAttr("primitive_kind");
  if (interfaces && provider && !primitive &&
      !llvm::is_contained(interfaces, provider))
    return site->emitOpError() << "Method receiver provider is absent from "
                                  "caller source ac.interfaces closure";
  return success();
}
FailureOr<ResolvedSourceMethod> resolve(Operation *caller, SymbolRefAttr symbol,
                                        ArrayAttr path, bool names,
                                        const HardwareBindings &parent,
                                        SourceModuleLookup lookup,
                                        Operation *site,
                                        SourceReceiverProofContext *context = nullptr,
                                        const SourceReceiverProofFees *fees = nullptr) {
  if (context && !fees) return failure();
  auto header = [&](Operation *module) -> LogicalResult {
    return context ? success(succeeded(context->prepare(module, site, *fees)))
                   : receiverHeader(module, site);
  };
  if (!caller || !symbol || symbol.getNestedReferences().size() != 1 || !path)
    return site->emitOpError()
           << "Method call requires a qualified callee and receiver path";
  if (path.size() > 128)
    return site->emitOpError() << "Method receiver depth budget exceeded";
  if (failed(header(caller)))
    return failure();
  auto package = caller->getParentOfType<mlir::ModuleOp>();
  if (!package)
    return site->emitOpError()
           << "Method receiver requires an enclosing package";
  HardwareAnalysis hardware(package);
  Operation *owner = caller;
  HardwareBindings bindings = parent;
  if (!bindings.owner)
    bindings.owner = caller;
  SmallVector<Attribute> occurrences;
  InstanceOp terminalInstance;
  DictionaryAttr terminalOwnerRow;
  uint64_t work = 0;
  for (Attribute endpoint : path) {
    if (failed(header(owner)))
      return failure();
    Operation *allocation = nullptr;
    DictionaryAttr selected, identity;
    llvm::DenseSet<Attribute> seenNames, seenOccurrences;
    auto consider = [&](StringAttr name, DictionaryAttr occurrence,
                        Operation *op, DictionaryAttr row) -> LogicalResult {
      if (++work > 1024 * 1024)
        return site->emitOpError() << "Method receiver work budget exceeded";
      if (!name || !occurrence ||
          failed(detail::verifyOccurrence(
              occurrence, [&] { return site->emitOpError(); })) ||
          failed(detail::verifyOriginDefinition(
              occurrence,
              FlatSymbolRefAttr::get(SymbolTable::getSymbolName(owner)),
              "Method receiver instance", [&] { return site->emitOpError(); })))
        return failure();
      if (!seenNames.insert(name).second ||
          !seenOccurrences.insert(occurrence).second)
        return site->emitOpError()
               << "Method receiver instances require distinct names and "
                  "occurrence identities";
      bool matches = names ? endpoint == name : endpoint == occurrence;
      if (!matches)
        return success();
      if (identity)
        return site->emitOpError() << "Method receiver instance is ambiguous";
      allocation = op;
      selected = row;
      identity = occurrence;
      return success();
    };
    if (context) {
      auto inventory = context->prepare(owner, site, *fees);
      if (failed(inventory)) return failure();
      if (isa<ModuleImportOp>(owner) && !(**inventory).hasSummary)
        return site->emitOpError() << "Method receiver suffix requires published instance owners";
      for (size_t i = 0; i < (**inventory).instanceCount; ++i) {
        if (++work > 1024 * 1024)
          return site->emitOpError() << "Method receiver work budget exceeded";
        if (failed(fees->workStorage(site, 1, 0))) return failure();
        const auto &candidate = (**inventory).instances[i];
        if (endpoint != (names ? Attribute(candidate.name) : Attribute(candidate.occurrence)))
          continue;
        allocation = candidate.allocation;
        selected = candidate.row;
        identity = candidate.occurrence;
        break;
      }
    } else if (auto body = dyn_cast<ModuleOp>(owner)) {
      if (!body.getBody().hasOneBlock())
        return site->emitOpError()
               << "Method receiver requires one module body block";
      for (auto instance : body.getBody().front().getOps<InstanceOp>()) {
        if (failed(instance.verifyInvariantsImpl()) ||
            failed(instance.verify()) ||
            failed(consider(instance.getInstanceNameAttr(),
                            instance.getOccurrence(), instance, {})))
          return failure();
      }
    } else {
      auto summary =
          owner->getAttrOfType<DictionaryAttr>("transaction_summary");
      if (!summary || failed(verifySourceTransactionSummary(owner, summary)))
        return site->emitOpError()
               << "Method receiver suffix requires published instance owners";
      for (Attribute raw : summary.getAs<ArrayAttr>("owners")) {
        auto row = cast<DictionaryAttr>(raw);
        if (row.getAs<StringAttr>("kind").getValue() == "instance" &&
            failed(consider(row.getAs<StringAttr>("name"),
                            row.getAs<DictionaryAttr>("occurrence"), nullptr,
                            row)))
          return failure();
      }
    }
    if (!identity)
      return site->emitOpError()
             << "Method instance path does not name a direct child instance";
    auto callee = allocation
                      ? allocation->getAttrOfType<FlatSymbolRefAttr>("callee")
                      : selected.getAs<FlatSymbolRefAttr>("callee");
    Operation *target = callee ? lookup(callee) : nullptr;
    if (failed(header(target)))
      return failure();
    if (failed(admitDependency(caller, target, site)))
      return failure();
    auto actualParameters =
        allocation ? allocation->getAttrOfType<ArrayAttr>("parameters")
                   : selected.getAs<ArrayAttr>("parameters");
    auto actualTypes =
        allocation ? allocation->getAttrOfType<ArrayAttr>("type_arguments")
                   : selected.getAs<ArrayAttr>("type_arguments");
    // The registry authority can be a prototype owned by another header.
    // Bind through this package's authenticated snapshot so TypeParam owner
    // identity agrees with HardwareAnalysis, while retaining the registry
    // target for receiver closure and Method lookup.
    Operation *bindingTarget = hardware.lookupDefinition(callee.getValue());
    if (!bindingTarget)
      bindingTarget = target;
    if (bindingTarget != target) {
      if (!target || !isa<ModuleImportOp>(bindingTarget))
        return site->emitOpError()
               << "Method receiver provider snapshot is invalid";
      for (StringRef field : {"source_owner", "function_type", "parameters",
                              "type_parameters", "primitive_kind"})
        if (bindingTarget->getAttr(field) != target->getAttr(field))
          return site->emitOpError()
                 << "Method receiver provider snapshot differs from authority: "
                 << field;
    }
    if (context && failed(fees->instanceBinding(owner, allocation, bindingTarget, site)))
      return failure();
    auto bound = hardware.bindCallee(bindingTarget, actualParameters,
                                     actualTypes, bindings, site);
    if (failed(bound))
      return failure();
    auto signature = cast<FunctionType>(
        target->getAttrOfType<TypeAttr>("function_type").getValue());
    auto checkTypes = [&](TypeRange actual, TypeRange formal) -> LogicalResult {
      if (actual.size() != formal.size())
        return site->emitOpError()
               << "Method receiver instance signature arity mismatch";
      for (auto [a, f] : llvm::zip(actual, formal)) {
        auto actualType = hardware.resolveType(a, bindings, site);
        auto formalType = hardware.resolveType(f, *bound, site);
        if (failed(actualType) || failed(formalType))
          return failure();
        if (!areEquivalentHardwareTypes(*actualType, *formalType))
          return site->emitOpError()
                 << "Method receiver instance payload type mismatch: actual "
                 << *actualType << " does not match bound formal "
                 << *formalType;
      }
      return success();
    };
    SmallVector<Type> inputs, results;
    if (allocation) {
      inputs.assign(allocation->getOperandTypes().begin(),
                    allocation->getOperandTypes().end());
      results.assign(allocation->getResultTypes().begin(),
                     allocation->getResultTypes().end());
    } else {
      for (Attribute raw : selected.getAs<ArrayAttr>("input_types"))
        inputs.push_back(cast<TypeAttr>(raw).getValue());
      for (Attribute raw : selected.getAs<ArrayAttr>("result_types"))
        results.push_back(cast<TypeAttr>(raw).getValue());
    }
    if (failed(checkTypes(inputs, signature.getInputs())) ||
        failed(checkTypes(results, signature.getResults())))
      return failure();
    occurrences.push_back(identity);
    terminalInstance = dyn_cast_or_null<InstanceOp>(allocation);
    terminalOwnerRow = selected;
    bindings = *bound;
    owner = target;
  }
  if (symbol.getRootReference() != SymbolTable::getSymbolName(owner))
    return site->emitOpError()
           << "callee module must match resolved receiver module";
  StringRef methodName = symbol.getNestedReferences().front().getValue();
  SourceMethodView method;
  bool nativeQueue =
      (terminalInstance && isQueueInstance(terminalInstance)) ||
      (!terminalInstance && terminalOwnerRow &&
       (terminalOwnerRow.getAs<FlatSymbolRefAttr>("callee").getValue() ==
            "pycircuit.__builtins__.fifo_local" ||
        terminalOwnerRow.getAs<FlatSymbolRefAttr>("callee").getValue() ==
            "pycircuit.__builtins__.fifo_downstream_pop"));
  if (nativeQueue) {
    NativeQueueAction action = methodName == "push"
                                   ? NativeQueueAction::Push
                                   : methodName == "pop"
                                         ? NativeQueueAction::Pop
                                         : methodName == "peek"
                                               ? NativeQueueAction::Peek
                                               : NativeQueueAction::None;
    if (action == NativeQueueAction::None)
      return site->emitOpError() << "unknown native Queue action";
    bool pushUnbound = terminalInstance
                           ? terminalInstance->hasAttr(
                                 "ac.source_unbound_push")
                           : (terminalOwnerRow.getAs<BoolAttr>("unbound_push") &&
                              terminalOwnerRow.getAs<BoolAttr>("unbound_push")
                                  .getValue());
    bool popUnbound = terminalInstance
                          ? terminalInstance->hasAttr(
                                "ac.source_unbound_pop")
                          : (terminalOwnerRow.getAs<BoolAttr>("unbound_pop") &&
                             terminalOwnerRow.getAs<BoolAttr>("unbound_pop")
                                 .getValue());
    if (action == NativeQueueAction::Push && !pushUnbound)
      return site->emitOpError()
             << "Queue push conflicts with an externally bound raw input";
    if (action == NativeQueueAction::Pop && !popUnbound)
      return site->emitOpError()
             << "Queue pop conflicts with an externally bound raw output";
    Type payload = terminalInstance
                       ? terminalInstance.getResult(2).getType()
                       : cast<TypeAttr>(terminalOwnerRow
                                            .getAs<ArrayAttr>("result_types")[2])
                             .getValue();
    Attribute constraint = terminalInstance
                               ? terminalInstance->getAttr(
                                     "ac.payload_constraint")
                               : terminalOwnerRow.get("payload_constraint");
    if (!constraint ||
        failed(detail::verifySourceCallHardwareConstraint(
            constraint, payload, hardware, caller, site,
            "native Queue payload")))
      return site->emitOpError()
             << "native Queue method requires its checked logical payload constraint";
    method = SourceMethodView{owner, {}, {}, action, terminalInstance,
                              terminalOwnerRow, payload, constraint};
  } else {
    auto resolvedMethod = context ? context->method(owner, methodName, site, *fees)
                                  : lookupSourceMethod(owner, methodName, site);
    if (failed(resolvedMethod))
      return failure();
    method = *resolvedMethod;
  }
  if (auto visibility = method.getVisibility();
      visibility && visibility.getValue() == "private" &&
      (owner != caller || !path.empty()))
    return site->emitOpError()
           << "private Method requires a local lexical self call";
  auto receiver = DictionaryAttr::get(
      site->getContext(),
      {NamedAttribute(StringAttr::get(site->getContext(), "instance_path"),
                      ArrayAttr::get(site->getContext(), occurrences))});
  return ResolvedSourceMethod{method, bindings, receiver};
}
} // namespace
FailureOr<ResolvedSourceMethod>
resolveSourceMethodCall(SourceMethodCallOp call) {
  auto caller = call->getParentOfType<ModuleOp>();
  HardwareBindings parent;
  parent.owner = caller;
  auto package = call->getParentOfType<mlir::ModuleOp>();
  SymbolTableCollection symbols;
  auto lookup = [&](FlatSymbolRefAttr symbol) -> Operation * {
    return symbols.lookupSymbolIn(package, symbol);
  };
  return resolveSourceMethodCall(call, parent, lookup);
}
FailureOr<ResolvedSourceMethod>
resolveSourceMethodCall(SourceMethodCallOp call,
                        const HardwareBindings &parent,
                        SourceModuleLookup lookup,
                        SourceReceiverProofContext *context,
                        const SourceReceiverProofFees *fees) {
  auto caller = call->getParentOfType<ModuleOp>();
  if (!caller || parent.owner != caller.getOperation())
    return call.emitOpError()
           << "Method call parent bindings must belong to its lexical module";
  return resolve(caller, call.getCalleeAttr(), call.getInstancePath(), true,
                 parent, lookup, call, context, fees);
}
FailureOr<ResolvedSourceMethod>
resolveSourceMethodCall(Operation *caller, DictionaryAttr row,
                        const HardwareBindings &parent,
                        SourceModuleLookup lookup, Operation *site) {
  if (!row)
    return site->emitOpError() << "Method call requires a summary call row";
  auto receiver = row.getAs<DictionaryAttr>("receiver");
  auto resolved = resolve(caller, row.getAs<SymbolRefAttr>("callee"),
                          receiver ? receiver.getAs<ArrayAttr>("instance_path")
                                   : ArrayAttr(),
                          false, parent, lookup, site);
  if (failed(resolved))
    return failure();
  if (auto rows = row.getAs<ArrayAttr>("ac.formal_owner_bindings")) {
    auto proposals = resolved->method.getFormalProposals();
    for (Attribute raw : rows) {
      auto binding = cast<DictionaryAttr>(raw);
      if (!proposals ||
          !llvm::is_contained(proposals, binding.get("formal")))
        return site->emitOpError()
               << "owner binding must identify a callee writable formal";
    }
  }
  auto actuals = row.getAs<ArrayAttr>("arguments");
  auto formal = resolved->method.getFunctionType();
  if (!actuals || actuals.size() != formal.getNumInputs())
    return site->emitOpError()
           << "Method call argument arity must match signature";
  HardwareAnalysis hardware(caller->getParentOfType<mlir::ModuleOp>());
  for (auto [actual, type] : llvm::zip(actuals, formal.getInputs())) {
    auto value = dyn_cast<DictionaryAttr>(actual);
    auto fact = value ? value.getAs<DictionaryAttr>("fact") : DictionaryAttr();
    auto dependencies =
        value ? value.getAs<DictionaryAttr>("dependencies") : DictionaryAttr();
    auto kind = fact ? fact.getAs<StringAttr>("kind") : StringAttr();
    if (!value || value.size() != 2 || !fact || !dependencies || !kind)
      return site->emitOpError()
             << "Method argument requires a predicate fact and dependencies";
    // Local ordinal scope and dependency authority are checked by the existing
    // summary structure verifier. This resolver checks closed predicate shape
    // and bound formal widths without inventing an opaque actual payload type.
    if (kind.getValue() == "unknown") {
      if (fact.size() != 1)
        return site->emitOpError()
               << "unknown Method argument fact has extra fields";
      continue;
    }
    if (kind.getValue() == "constant") {
      if (fact.size() != 2 || !fact.getAs<BoolAttr>("value"))
        return site->emitOpError()
               << "constant Method argument fact requires BoolAttr";
    } else if (kind.getValue() == "argument") {
      auto ordinal = fact.getAs<IntegerAttr>("ordinal");
      if (fact.size() != 2 || !ordinal ||
          !ordinal.getType().isSignlessInteger(64) ||
          ordinal.getValue().isNegative())
        return site->emitOpError()
               << "Method argument fact requires a nonnegative i64 ordinal";
    } else {
      return site->emitOpError()
             << "unsupported Method argument predicate fact kind";
    }
    auto boundType = hardware.resolveType(type, resolved->bindings, site);
    if (failed(boundType))
      return failure();
    auto width = hardware.getPackedWidth(*boundType, resolved->bindings, site);
    if (failed(width))
      return failure();
    if (!isa<BitsType>(*boundType) || *width != 1)
      return site->emitOpError() << "known Method argument predicate requires "
                                    "a bound physical one-bit formal";
  }
  return resolved;
}
} // namespace acir::ac
