// Extend the ordinary private Module inputs and bind existing FIFO operands.
// This runs after the existing state materializer and before value-availability
// routing. No queue instance, kernel or alternative transaction is introduced.
class QueuePinMaterializer {
public:
  QueuePinMaterializer(const PreparedTransactionLowering &prepared,
                        MaterializedTransactionLowering &candidate,
                        SourceCheckBudget &budget)
      : prepared(prepared), candidate(candidate), budget(budget),
        hardware(prepared.original), routes(prepared, candidate.packageMap) {}

  LogicalResult run() {
    for (const auto &definition : prepared.definitions) {
      if (definition.queueIncoming.empty()) continue;
      auto module = cast<ac::ModuleOp>(candidate.packageMap.lookup(definition.module.operator->()));
      Builder b(module.getContext());
      auto signature = module.getFunctionType();
      SmallVector<Type> inputs(signature.getInputs());
      SmallVector<Attribute> names(module.getInputNames().getValue());
      SmallVector<Attribute> defaults;
      auto existing = module->getAttrOfType<DictionaryAttr>("ac.internal_ports");
      if (existing) llvm::append_range(defaults, existing.getAs<ArrayAttr>("root_inputs"));
      auto &arguments = incoming[definition.module];
      for (auto [ordinal, route] : llvm::enumerate(definition.queueIncoming)) {
        if (!charge(budget, module, 24)) return failure();
        auto add = [&](Type type, StringRef suffix) {
          std::string stem = (Twine("__ac_queue_") + suffix + "_" + Twine(ordinal)).str();
          std::string name = stem;
          uint64_t collision = 0;
          auto exists = [&] {
            return llvm::is_contained(names, b.getStringAttr(name)) ||
                   llvm::is_contained(module.getOutputNames().getValue(), b.getStringAttr(name));
          };
          while (exists()) name = (Twine(stem) + "_" + Twine(++collision)).str();
          names.push_back(b.getStringAttr(name));
          inputs.push_back(type);
          defaults.push_back(b.getI64IntegerAttr(0));
          return module.getBody().front().addArgument(type, module.getLoc());
        };
        Value path = add(route.predicateType, "path");
        Value data = route.push ? add(route.payloadType, "data") : Value();
        arguments.push_back({path, data});
      }
      module->setAttr("function_type", TypeAttr::get(FunctionType::get(module.getContext(),
          inputs, signature.getResults())));
      module->setAttr("input_names", b.getArrayAttr(names));
      module->setAttr("ac.internal_ports", b.getDictionaryAttr({
          b.getNamedAttr("source_inputs", b.getI64IntegerAttr(definition.sourceInputs)),
          b.getNamedAttr("source_outputs", b.getI64IntegerAttr(definition.sourceOutputs)),
          b.getNamedAttr("root_inputs", b.getArrayAttr(defaults))}));
    }
    for (auto [index, transaction] : llvm::enumerate(prepared.transactions)) {
      const auto &plan = *transaction.plan;
      auto rule = candidate.transactions[index].rule;
      for (const auto &proposal : transaction.queueProposals) {
        const auto &access = plan.queueAccesses[proposal.access];
        const auto &owner = plan.queueOwners[access.owner];
        auto queue = owner.allocation;
        QueueRoutingKey key{{routePath(prepared.original->getContext(), anchorPath(plan, owner.anchor)),
                              queue.getOutputs()[1]},
                             access.kind == ac::TransactionQueueAccessKind::Push};
        if (!charge(budget, access.operation, 16)) return failure();
        local[plan.routingModule][key].push_back({rule->getResult(proposal.pathResult),
            proposal.dataResult ? rule->getResult(*proposal.dataResult) : Value(),
            access.operation, plan.frames[access.frame].bindings, rule});
      }
    }
    // Upward summaries contain local/descendant proposals only. They are
    // never fed back into the downward ancestor drive.
    for (const auto &definition : prepared.definitions) {
      auto module = cast<ac::ModuleOp>(candidate.packageMap.lookup(definition.module.operator->()));
      auto yield = cast<ac::YieldOp>(module.getBody().front().getTerminator());
      OpBuilder builder(yield);
      SmallVector<Value> outputs(yield.getValues());
      for (const auto &route : definition.queuePopSummaries) {
        auto value = subtree(definition.module, route.path, route.allocation, builder);
        if (failed(value)) return failure();
        outputs.push_back(*value);
      }
      yield->setOperands(outputs);
    }
    for (auto [index, transaction] : llvm::enumerate(prepared.transactions)) {
      const auto &plan = *transaction.plan;
      auto rule = candidate.transactions[index].rule;
      OpBuilder builder(rule);
      SmallVector<Value> captures(rule.getCaptures());
      for (const auto &external : transaction.externalPops) {
        const auto &owner = plan.queueOwners[external.owner];
        auto value = externalPop(plan.routingModule, anchorPath(plan, owner.anchor),
                                 owner.allocation, rule, builder);
        if (failed(value)) return failure();
        captures[external.capture] = *value;
      }
      rule->setOperands(captures);
    }
    // Existing instances keep their source and state-route operands exactly;
    // append only the checked queue suffix. All actual occurrences participate.
    for (const auto &parent : prepared.definitions) {
      for (auto original : parent.module->getRegion(0).front().getOps<ac::InstanceOp>()) {
        auto child = dyn_cast_or_null<ac::ModuleOp>(hardware.resolveCallee(original));
        if (!child) continue;
        const auto &definition = prepared.definitions[prepared.definitionIndex.lookup(child)];
        if (definition.queueIncoming.empty()) continue;
        auto allocation = cast<ac::InstanceOp>(candidate.packageMap.lookup(original.operator->()));
        SmallVector<Value> operands(allocation.getOperands());
        OpBuilder builder(allocation);
        for (const auto &route : definition.queueIncoming) {
          SmallVector<ac::InstanceOp> path{original};
          llvm::append_range(path, route.path);
          auto pins = pinsFor(parent.module, path, route.allocation, route.push, builder);
          if (failed(pins)) return failure();
          operands.push_back(pins->first);
          if (route.push) operands.push_back(pins->second);
        }
        allocation->setOperands(operands);
      }
      auto module = cast<ac::ModuleOp>(candidate.packageMap.lookup(parent.module.operator->()));
      for (auto index : parent.queueIndices) {
        auto original = prepared.queues[index];
        auto queue = cast<ac::InstanceOp>(candidate.packageMap.lookup(original.operator->()));
        OpBuilder builder(queue);
        if (original->hasAttr("ac.source_unbound_push")) {
          auto pins = pinsFor(parent.module, {}, original, true, builder);
          if (failed(pins)) return failure();
          queue->setOperand(2, pins->first);
          queue->setOperand(3, pins->second);
        }
        if (original->hasAttr("ac.source_unbound_pop")) {
          auto pins = pinsFor(parent.module, {}, original, false, builder);
          if (failed(pins)) return failure();
          queue->setOperand(4, pins->first);
        }
        queue->removeAttr("ac.source_unbound_push");
        queue->removeAttr("ac.source_unbound_pop");
        queue->removeAttr("ac.payload_constraint");
      }
      if (parent.expectedQueueWiring) module->setAttr("ac.queue_wiring", parent.expectedQueueWiring);
    }
    return success();
  }

private:
  struct Proposal {
    Value path, data;
    Operation *source;
    ac::HardwareBindings bindings;
    ac::RuleOp rule;
  };
  QueueRoutingKey popKey(ArrayRef<ac::InstanceOp> path,
                         ac::InstanceOp queue) {
    return {{routePath(prepared.original->getContext(), path),
             queue.getOutputs()[1]}, false};
  }
  FailureOr<Value> popUnion(ac::ModuleOp module, ArrayRef<ac::InstanceOp> path,
      ac::InstanceOp queue, ArrayRef<Value> values, OpBuilder &builder) {
    if (values.empty()) {
      auto bit = routes.value(module, path, queue.getOutputs()[1]);
      if (failed(bit) || !charge(budget, queue, 16) ||
          !prepayAppend(budget, queue, candidate.queueDisabled)) return failure();
      Value zero = bitsLiteral(bit->getType(), queue.getOccurrence(), queue.getLoc(), false, builder);
      candidate.queueDisabled.push_back(zero);
      return zero;
    }
    Value result = values.front();
    for (Value value : values.drop_front()) {
      if (!charge(budget, queue, 16)) return failure();
      OperationState join(queue.getLoc(), ac::BitsBinaryOp::getOperationName());
      join.addOperands({result, value});
      join.addTypes(result.getType());
      join.addAttribute("opcode", builder.getStringAttr("or"));
      result = builder.create(join)->getResult(0);
    }
    return result;
  }
  LogicalResult appendAncestor(ac::ModuleOp module, const QueueRoutingKey &key,
                      SmallVectorImpl<Value> &values) {
    const auto &definition = prepared.definitions[prepared.definitionIndex.lookup(module)];
    auto route = definition.queueIncomingIndex.find(key);
    if (route != definition.queueIncomingIndex.end()) {
      if (!prepayAppend(budget, module, values)) return failure();
      values.push_back(incoming[module][route->second].first);
    }
    return success();
  }
  LogicalResult appendLocal(ac::ModuleOp module, const QueueRoutingKey &key,
                   SmallVectorImpl<Value> &values, ac::RuleOp excluding = {}) {
    auto found = local[module].find(key);
    if (found == local[module].end()) return success();
    for (const auto &proposal : found->second) {
      if (!charge(budget, proposal.source)) return failure();
      if (!excluding || proposal.rule != excluding) {
        if (!prepayAppend(budget, proposal.source, values)) return failure();
        values.push_back(proposal.path);
      }
    }
    return success();
  }
  FailureOr<Value> descendant(ac::ModuleOp module,
      ArrayRef<ac::InstanceOp> path, ac::InstanceOp queue) {
    if (path.empty()) {
      if (queue->hasAttr("ac.source_unbound_pop")) return Value();
      // The original authored out_ready is the authority for raw-bound pop.
      return routes.value(module, {}, queue.getInputs()[4]);
    }
    auto child = dyn_cast_or_null<ac::ModuleOp>(hardware.resolveCallee(path.front()));
    if (!child) return failure();
    const auto &definition = prepared.definitions[prepared.definitionIndex.lookup(child)];
    RoutingKey key{routePath(prepared.original->getContext(), path.drop_front()),
                   queue.getOutputs()[1]};
    auto found = definition.queuePopSummaryIndex.find(key);
    auto allocation = cast<ac::InstanceOp>(candidate.packageMap.lookup(path.front().operator->()));
    if (found == definition.queuePopSummaryIndex.end())
      return queue.emitOpError() << "prospective Queue descendant summary is missing";
    return allocation.getResult(definition.sourceOutputs + definition.upward.size() + found->second);
  }
  FailureOr<Value> subtree(ac::ModuleOp module, ArrayRef<ac::InstanceOp> path,
                           ac::InstanceOp queue, OpBuilder &builder) {
    SmallVector<Value> values;
    if (failed(appendLocal(module, popKey(path, queue), values))) return failure();
    auto child = descendant(module, path, queue);
    if (failed(child)) return failure();
    if (*child) {
      if (!prepayAppend(budget, queue, values)) return failure();
      values.push_back(*child);
    }
    return popUnion(module, path, queue, values, builder);
  }
  FailureOr<Value> externalPop(ac::ModuleOp module,
      ArrayRef<ac::InstanceOp> path, ac::InstanceOp queue, ac::RuleOp excluding,
      OpBuilder &builder) {
    auto key = popKey(path, queue);
    SmallVector<Value> values;
    if (failed(appendAncestor(module, key, values)) ||
        failed(appendLocal(module, key, values, excluding))) return failure();
    auto child = descendant(module, path, queue);
    if (failed(child)) return failure();
    if (*child) {
      if (!prepayAppend(budget, queue, values)) return failure();
      values.push_back(*child);
    }
    return popUnion(module, path, queue, values, builder);
  }
  FailureOr<std::pair<Value, Value>> pinsFor(ac::ModuleOp module,
      ArrayRef<ac::InstanceOp> path, ac::InstanceOp queue, bool push,
      OpBuilder &builder) {
    QueueRoutingKey key{{routePath(prepared.original->getContext(), path),
                         queue.getOutputs()[1]}, push};
    if (auto found = materialized[module].find(key); found != materialized[module].end())
      return found->second;
    std::pair<Value, Value> pins;
    if (!push) {
      SmallVector<Value> values;
      if (failed(appendAncestor(module, key, values)) ||
          failed(appendLocal(module, key, values))) return failure();
      auto value = popUnion(module, path, queue, values, builder);
      if (failed(value)) return failure();
      pins.first = *value;
      materialized[module].try_emplace(key, pins);
      return pins;
    }
    auto writers = local[module].find(key);
    if (writers != local[module].end()) {
      auto &proposals = writers->second;
      pins = {proposals.front().path, proposals.front().data};
      detail::MaterializedQueueMerge merge;
      merge.module = module;
      merge.key = key;
      for (const auto &proposal : ArrayRef<Proposal>(proposals).drop_front()) {
        if (!charge(budget, proposal.source, 16)) return failure();
        OperationState join(proposal.source->getLoc(), ac::BitsBinaryOp::getOperationName());
        join.addOperands({pins.first, proposal.path});
        join.addTypes(pins.first.getType());
        join.addAttribute("opcode", builder.getStringAttr("or"));
        pins.first = builder.create(join)->getResult(0);
        merge.enables.push_back(pins.first);
        if (push) {
          auto selected = materializeSelection(proposal.source, proposal.bindings,
              proposal.path, proposal.data, pins.second, pins.second.getType(), builder, budget);
          if (failed(selected)) return failure();
          pins.second = selected->value;
          merge.data.push_back(std::move(*selected));
        }
      }
      candidate.queueMerges.push_back(std::move(merge));
    } else {
      const auto &definition = prepared.definitions[prepared.definitionIndex.lookup(module)];
      auto route = definition.queueIncomingIndex.find(key);
      if (route != definition.queueIncomingIndex.end()) pins = incoming[module][route->second];
      else {
        auto bit = routes.value(module, path, queue.getOutputs()[1]);
        if (failed(bit)) return failure();
        if (!charge(budget, queue, 16)) return failure();
        pins.first = bitsLiteral(bit->getType(), queue.getOccurrence(), queue.getLoc(), false, builder);
        candidate.queueDisabled.push_back(pins.first);
        if (push) {
          auto data = routes.value(module, path, queue.getInputs()[3]);
          if (failed(data)) return failure();
          pins.second = *data;
        }
      }
    }
    materialized[module].try_emplace(key, pins);
    return pins;
  }
  const PreparedTransactionLowering &prepared;
  MaterializedTransactionLowering &candidate;
  SourceCheckBudget &budget;
  ac::HardwareAnalysis hardware;
  MaterializationRoutes routes;
  llvm::DenseMap<Operation *, SmallVector<std::pair<Value, Value>, 0>> incoming;
  llvm::DenseMap<Operation *, llvm::DenseMap<QueueRoutingKey, SmallVector<Proposal, 0>>> local;
  llvm::DenseMap<Operation *, llvm::DenseMap<QueueRoutingKey, std::pair<Value, Value>>> materialized;
};
