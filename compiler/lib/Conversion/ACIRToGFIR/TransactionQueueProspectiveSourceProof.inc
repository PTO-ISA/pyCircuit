  Value originalQueueValid(ac::InstanceOp queue) {
    return queue.getOutputs()[1];
  }

  FailureOr<bool> originalQueueUnreachable(
      const ac::TransactionStatePlan &plan,
      const ac::TransactionQueueAccess &access) {
    // pathFact and every conditionFact are authenticated by SourceReplayValues
    // against intact SSA before the queue source proof consumes this result.
    bool zero = access.pathFact && !access.pathFact.getValue();
    auto path = access.point.path;
    while (path.branch && !zero) {
      if (!charge(budget, access.operation) || *path.branch >= plan.branches.size())
        return failure();
      const auto &branch = plan.branches[*path.branch];
      zero = branch.conditionFact && branch.conditionFact.getValue() != path.whenTrue;
      path = branch.entry.path;
    }
    return zero;
  }

  FailureOr<SmallVector<detail::TransactionQueueExternalPop, 0>>
  originalQueueExternalPops(const TransactionPreparation &preparation,
                            uint64_t firstCapture) {
    const auto &plan = *preparation.plan;
    SmallVector<ac::BoundValueId, 0> liveIns;
    for (auto [id, reference] : llvm::enumerate(plan.values)) {
      if (!charge(budget, plan.transaction) || !reference.value ||
          reference.anchor >= plan.instances.size()) return failure();
      if (moduleValue(reference.value, plan.instances[reference.anchor].definition) &&
          reference.value.getDefiningOp() != plan.transaction.operator->()) {
        if (!prepayAppend(budget, plan.transaction, liveIns)) return failure();
        liveIns.push_back(id);
      }
    }
    for (auto pin : {plan.clockValue, plan.resetValue})
      if (!llvm::is_contained(liveIns, pin)) {
        if (!prepayAppend(budget, plan.transaction, liveIns)) return failure();
        liveIns.push_back(pin);
      }
    if (!llvm::equal(liveIns, preparation.liveIns))
      return reject("original transaction lexical capture inventory/order differs");
    SmallVector<detail::TransactionQueueExternalPop, 0> expected;
    for (const auto &access : plan.queueAccesses) {
      if (!charge(budget, access.operation) || access.owner >= plan.queueOwners.size())
        return failure();
      if (access.kind != ac::TransactionQueueAccessKind::Push ||
          plan.queueOwners[access.owner].config.readyPolicy != ac::QueueReadyPolicy::DownstreamPop)
        continue;
      if (llvm::any_of(expected, [&](const auto &entry) { return entry.owner == access.owner; }))
        continue;
      if (!prepayAppend(budget, access.operation, expected)) return failure();
      expected.push_back({access.owner, firstCapture + expected.size()});
    }
    if (expected.size() != preparation.externalPops.size())
      return reject("original queue external-pop capture inventory differs");
    for (auto [derived, actual] : llvm::zip(expected, preparation.externalPops))
      if (derived.owner != actual.owner || derived.capture != actual.capture)
        return reject("original queue external-pop owner/capture order differs");
    return expected;
  }

  struct OriginalQueueRouting {
    ac::ModuleOp module;
    SmallVector<detail::QueueRoute, 0> incoming;
    SmallVector<detail::QueuePopSummaryRoute, 0> summaries;
  };

  FailureOr<SmallVector<OriginalQueueRouting, 0>> originalQueueRoutes() {
    SmallVector<OriginalQueueRouting, 0> result;
    for (auto module : prepared.original->getRegion(0).front().getOps<ac::ModuleOp>()) {
      if (!prepayAppend(budget, module, result)) return failure();
      result.push_back({module, {}, {}});
    }
    auto inventory = [&](ac::ModuleOp module) -> OriginalQueueRouting * {
      auto found = llvm::find_if(result, [&](const auto &entry) { return entry.module == module; });
      return found == result.end() ? nullptr : &*found;
    };
    auto demand = [&](ac::ModuleOp module, ArrayRef<ac::InstanceOp> path,
                      ac::InstanceOp queue, bool push,
                      bool summary) -> LogicalResult {
      auto *entry = inventory(module);
      if (!entry || !charge(budget, module, 8 + path.size())) return failure();
      auto key = routingKey(path, queue.getOutputs()[1]);
      bool present = summary
          ? llvm::any_of(entry->summaries, [&](const auto &route) {
              return routingKey(route.path, originalQueueValid(route.allocation)) == key;
            })
          : llvm::any_of(entry->incoming, [&](const auto &route) {
              return route.push == push && routingKey(route.path, originalQueueValid(route.allocation)) == key;
            });
      if (present) return success();
      ac::HardwareBindings bindings;
      bindings.owner = module;
      for (auto allocation : path) {
        auto next = hardware.bindInstance(allocation, bindings);
        if (failed(next)) return failure();
        bindings = *next;
      }
      auto predicate = hardware.resolveType(queue.getOutputs()[1].getType(),
                                            bindings, module);
      auto payload = hardware.resolveType(queue.getOutputs()[2].getType(),
                                          bindings, module);
      if (failed(predicate) || failed(payload)) return failure();
      if (summary) {
        if (!prepayAppend(budget, module, entry->summaries)) return failure();
        entry->summaries.push_back({SmallVector<ac::InstanceOp>(path), queue, *predicate});
      } else {
        if (!prepayAppend(budget, module, entry->incoming)) return failure();
        entry->incoming.push_back({SmallVector<ac::InstanceOp>(path), queue, push, *payload, *predicate});
      }
      return success();
    };
    // Registration occurrence and invocation inventories have already been
    // replayed from original source. Derive demands from those source accesses.
    for (auto &entry : result)
      for (uint64_t occurrenceIndex : prepared.definitions[prepared.definitionIndex.lookup(entry.module)].transactionIndices) {
        auto index = originalTransactionIndex(occurrenceIndex);
        if (failed(index)) return failure();
        const auto &transaction = prepared.transactions[*index];
        const auto &plan = *transaction.plan;
        if (plan.routingModule != entry.module) return reject("queue transaction routing owner differs");
        for (const auto &access : plan.queueAccesses) {
          if (access.kind == ac::TransactionQueueAccessKind::Peek) continue;
          const auto &owner = plan.queueOwners[access.owner];
          auto path = anchorPath(plan, owner.anchor);
          for (unsigned offset = 0; offset < path.size(); ++offset) {
            auto child = dyn_cast_or_null<ac::ModuleOp>(hardware.resolveCallee(path[offset]));
            if (!child || failed(demand(child, ArrayRef<ac::InstanceOp>(path).drop_front(offset + 1),
                                       owner.allocation, access.payload.has_value(), false))) return failure();
          }
        }
        auto external = originalQueueExternalPops(transaction,
            transaction.liveIns.size());
        if (failed(external)) return failure();
        for (const auto &capture : *external) {
          const auto &owner = plan.queueOwners[capture.owner];
          auto path = anchorPath(plan, owner.anchor);
          if (owner.unboundPop &&
              failed(demand(plan.routingModule, path, owner.allocation, false, false))) return failure();
          for (unsigned offset = 0; offset < path.size(); ++offset) {
            auto child = dyn_cast_or_null<ac::ModuleOp>(hardware.resolveCallee(path[offset]));
            if (!child || failed(demand(child, ArrayRef<ac::InstanceOp>(path).drop_front(offset + 1),
                                       owner.allocation, false, true))) return failure();
          }
        }
      }
    bool changed;
    do {
      changed = false;
      for (auto &entry : result)
        for (auto allocation : entry.module.getBody().front().getOps<ac::InstanceOp>()) {
          auto child = dyn_cast_or_null<ac::ModuleOp>(hardware.resolveCallee(allocation));
          if (!child) continue;
          auto *childRoutes = inventory(child);
          if (!childRoutes || !charge(budget, allocation, 8 + childRoutes->incoming.size())) return failure();
          SmallVector<detail::QueueRoute, 0> routes(childRoutes->incoming);
          for (const auto &route : routes) {
            if (route.push) continue;
            SmallVector<ac::InstanceOp> path{allocation};
            llvm::append_range(path, route.path);
            auto before = entry.incoming.size();
            if (failed(demand(entry.module, path, route.allocation, false, false))) return failure();
            changed |= entry.incoming.size() != before;
          }
        }
    } while (changed);
    for (const auto &entry : result) {
      const DefinitionRouting *definition = nullptr;
      for (const auto &possible : prepared.definitions)
        if (possible.module == entry.module) {
          if (definition) return reject("original queue routing definition is duplicated");
          definition = &possible;
        }
      if (!definition || definition->queueIncoming.size() != entry.incoming.size() ||
          definition->queuePopSummaries.size() != entry.summaries.size() ||
          definition->queueIncomingIndex.size() != entry.incoming.size() ||
          definition->queuePopSummaryIndex.size() != entry.summaries.size())
        return reject("original queue routing closure inventory differs");
      for (auto [ordinal, route] : llvm::enumerate(entry.incoming)) {
        const auto &actual = definition->queueIncoming[ordinal];
        detail::QueueRoutingKey key{routingKey(route.path, originalQueueValid(route.allocation)), route.push};
        auto index = definition->queueIncomingIndex.find(key);
        if (actual.path != route.path || actual.allocation != route.allocation || actual.push != route.push ||
            actual.payloadType != route.payloadType || actual.predicateType != route.predicateType ||
            index == definition->queueIncomingIndex.end() || index->second != ordinal)
          return reject("original queue incoming route order/identity differs");
      }
      for (auto [ordinal, route] : llvm::enumerate(entry.summaries)) {
        const auto &actual = definition->queuePopSummaries[ordinal];
        auto index = definition->queuePopSummaryIndex.find(routingKey(route.path, originalQueueValid(route.allocation)));
        if (actual.path != route.path || actual.allocation != route.allocation || actual.predicateType != route.predicateType ||
            index == definition->queuePopSummaryIndex.end() || index->second != ordinal)
          return reject("original queue summary route order/identity differs");
      }
    }
    return result;
  }

  LogicalResult originalQueueUnionProof(Value actual, ArrayRef<Value> terms,
      ac::ModuleOp originalModule, ac::InstanceOp queue, Type bit) {
    auto module = dyn_cast_or_null<ac::ModuleOp>(candidate.packageMap.lookupOrNull(originalModule.operator->()));
    if (!module || !actual || !isa<ac::BitsType>(bit) ||
        !isa<ac::BitsType>(actual.getType()) ||
        !ac::areEquivalentHardwareTypes(actual.getType(), bit)) return failure();
    for (Value term : terms)
      if (!term || !isa<ac::BitsType>(term.getType()) ||
          !ac::areEquivalentHardwareTypes(term.getType(), bit))
        return reject("original queue pop union contributor type differs");
    Builder b(queue.getContext());
    auto node = [&](Value output, StringRef name, ArrayRef<Value> operands,
                    DictionaryAttr attributes) -> LogicalResult {
      auto result = dyn_cast<OpResult>(output);
      Operation *operation = result ? result.getOwner() : nullptr;
      if (!result || result.getResultNumber() != 0 || !operation ||
          !operations.contains(operation) || operation->getNumRegions() ||
          operation->getNumResults() != 1 ||
          output.getType() != (operands.empty() ? bit : operands.front().getType()) ||
          operation->getName().getStringRef() != name ||
          operation->getBlock() != &module.getBody().front() ||
          operation->getLoc() != queue.getLoc() ||
          !llvm::equal(operation->getOperands(), operands) ||
          operation->getAttrs() != attributes.getValue())
        return reject("original queue pop union operation differs");
      if (Attribute raw = operation->getPropertiesAsAttribute()) {
        auto properties = dyn_cast<DictionaryAttr>(raw);
        if (!properties) return failure();
        for (auto property : properties)
          if (attributes.get(property.getName()) != property.getValue()) return failure();
      }
      return clones.contains(operation) ? success() : cover(operation);
    };
    if (terms.empty())
      return node(actual, ac::BitsConstantOp::getOperationName(), {},
          b.getDictionaryAttr({b.getNamedAttr("value", bitsLiteralAttribute(
              bit, queue.getOccurrence(), b.getContext(), false))}));
    Value cursor = actual;
    for (uint64_t count = terms.size(); count > 1; --count) {
      if (!charge(budget, queue)) return failure();
      auto join = cursor.getDefiningOp<ac::BitsBinaryOp>();
      if (!join || join.getRhs() != terms[count - 1] ||
          failed(node(cursor, ac::BitsBinaryOp::getOperationName(),
              {join.getLhs(), terms[count - 1]}, b.getDictionaryAttr({
                  b.getNamedAttr("opcode", b.getStringAttr("or"))}))))
        return reject("original queue pop contributor union differs");
      cursor = join.getLhs();
    }
    if (cursor != terms.front())
      return reject("original queue pop union entry differs");
    return success();
  }

  LogicalResult originalQueuePopRoutingProof(ArrayRef<OriginalQueueRouting> inventories) {
    auto inventory = [&](ac::ModuleOp module) -> const OriginalQueueRouting * {
      auto found = llvm::find_if(inventories, [&](const auto &entry) { return entry.module == module; });
      return found == inventories.end() ? nullptr : &*found;
    };
    auto definition = [&](ac::ModuleOp module) -> const DefinitionRouting * {
      auto found = llvm::find_if(prepared.definitions, [&](const auto &entry) { return entry.module == module; });
      return found == prepared.definitions.end() ? nullptr : &*found;
    };
    auto appendAncestor = [&](ac::ModuleOp module, ArrayRef<ac::InstanceOp> path,
        ac::InstanceOp queue, SmallVectorImpl<Value> &terms) -> LogicalResult {
      auto *routes = inventory(module);
      auto *preparedDefinition = definition(module);
      if (!routes || !preparedDefinition) return failure();
      uint64_t input = preparedDefinition->sourceInputs + 2 * preparedDefinition->downward.size();
      auto target = cast<ac::ModuleOp>(candidate.packageMap.lookup(module.operator->()));
      for (const auto &route : routes->incoming) {
        if (!charge(budget, queue)) return failure();
        if (!route.push && routingKey(route.path, originalQueueValid(route.allocation)) ==
                routingKey(path, queue.getOutputs()[1])) {
          if (input >= target.getBody().front().getNumArguments()) return failure();
          terms.push_back(target.getBody().front().getArgument(input));
        }
        input += route.push ? 2 : 1;
      }
      return success();
    };
    auto appendLocal = [&](ac::ModuleOp module, ArrayRef<ac::InstanceOp> path,
        ac::InstanceOp queue, SmallVectorImpl<Value> &terms,
        std::optional<uint64_t> excluding = std::nullopt) -> LogicalResult {
      // Enumerate actual original registrations, then source proposal order.
      // Equal definitions at two instances remain distinct path identities.
      for (uint64_t occurrenceIndex : prepared.definitions[prepared.definitionIndex.lookup(module)].transactionIndices) {
        if (excluding && occurrenceIndex == *excluding) continue;
        auto index = originalTransactionIndex(occurrenceIndex);
        if (failed(index)) return failure();
        const auto &transaction = prepared.transactions[*index];
        const auto &plan = *transaction.plan;
        if (plan.routingModule != module) return reject("queue local occurrence routing owner differs");
        uint64_t proposal = 0;
        for (const auto &access : plan.queueAccesses) {
          if (!charge(budget, access.operation)) return failure();
          if (access.kind == ac::TransactionQueueAccessKind::Peek) continue;
          const auto &owner = plan.queueOwners[access.owner];
          if (access.kind == ac::TransactionQueueAccessKind::Pop &&
              routingKey(anchorPath(plan, owner.anchor), originalQueueValid(owner.allocation)) ==
                  routingKey(path, queue.getOutputs()[1])) {
            if (proposal >= transaction.queueProposals.size()) return failure();
            auto result = transaction.queueProposals[proposal].pathResult;
            if (result >= candidate.transactions[*index].rule->getNumResults()) return failure();
            terms.push_back(candidate.transactions[*index].rule->getResult(result));
          }
          ++proposal;
        }
      }
      return success();
    };
    auto appendDescendant = [&](ac::ModuleOp module, ArrayRef<ac::InstanceOp> path,
        ac::InstanceOp queue, SmallVectorImpl<Value> &terms) -> LogicalResult {
      if (path.empty()) {
        if (!queue->hasAttr("ac.source_unbound_pop")) {
          auto raw = candidate.packageMap.lookupOrNull(queue.getInputs()[4]);
          if (!raw || !values.contains(raw)) return reject("original queue raw pop binding is absent");
          terms.push_back(raw);
        }
        return success();
      }
      auto child = dyn_cast_or_null<ac::ModuleOp>(hardware.resolveCallee(path.front()));
      auto *routes = child ? inventory(child) : nullptr;
      auto *preparedDefinition = child ? definition(child) : nullptr;
      if (!routes || !preparedDefinition) return failure();
      auto actual = cast<ac::InstanceOp>(candidate.packageMap.lookup(path.front().operator->()));
      for (auto [ordinal, route] : llvm::enumerate(routes->summaries))
        if (routingKey(route.path, originalQueueValid(route.allocation)) ==
            routingKey(path.drop_front(), queue.getOutputs()[1])) {
          uint64_t result = preparedDefinition->sourceOutputs + preparedDefinition->upward.size() + ordinal;
          if (result >= actual->getNumResults()) return failure();
          terms.push_back(actual->getResult(result));
          return success();
        }
      return reject("original queue descendant pop summary is absent");
    };
    for (const auto &routes : inventories) {
      auto *preparedDefinition = definition(routes.module);
      if (!preparedDefinition) return failure();
      auto module = cast<ac::ModuleOp>(candidate.packageMap.lookup(routes.module.operator->()));
      auto yield = cast<ac::YieldOp>(module.getBody().front().getTerminator());
      for (auto [ordinal, summary] : llvm::enumerate(routes.summaries)) {
        SmallVector<Value> terms;
        if (failed(appendLocal(routes.module, summary.path, summary.allocation, terms)) ||
            failed(appendDescendant(routes.module, summary.path, summary.allocation, terms))) return failure();
        uint64_t output = preparedDefinition->sourceOutputs + preparedDefinition->upward.size() + ordinal;
        if (output >= yield->getNumOperands() || failed(originalQueueUnionProof(
                yield->getOperand(output), terms, routes.module, summary.allocation, summary.predicateType)))
          return reject("original queue upward pop summary differs");
      }
      for (uint64_t occurrenceIndex : preparedDefinition->transactionIndices) {
        auto index = originalTransactionIndex(occurrenceIndex);
        if (failed(index)) return failure();
        const auto &transaction = prepared.transactions[*index];
        const auto &plan = *transaction.plan;
        if (plan.routingModule != routes.module) return reject("queue external occurrence routing owner differs");
        auto external = originalQueueExternalPops(transaction,
            transaction.liveIns.size());
        if (failed(external)) return failure();
        auto rule = candidate.transactions[*index].rule;
        for (const auto &capture : *external) {
          const auto &owner = plan.queueOwners[capture.owner];
          auto path = anchorPath(plan, owner.anchor);
          SmallVector<Value> terms;
          if (failed(appendAncestor(routes.module, path, owner.allocation, terms)) ||
              failed(appendLocal(routes.module, path, owner.allocation, terms, *index)) ||
              failed(appendDescendant(routes.module, path, owner.allocation, terms))) return failure();
          if (capture.capture >= rule->getNumOperands() || failed(originalQueueUnionProof(
                  rule->getOperand(capture.capture), terms, routes.module, owner.allocation,
                  rule.getBody().front().getArgument(capture.capture).getType())))
            return reject("original queue external accepted-pop contributors differ");
        }
      }
      for (auto original : routes.module->getRegion(0).front()
                               .getOps<ac::InstanceOp>()) {
        if (!ac::isQueueInstance(original)) continue;
        if (!original->hasAttr("ac.source_unbound_pop")) continue;
        SmallVector<Value> terms;
        if (failed(appendAncestor(routes.module, {}, original, terms)) ||
            failed(appendLocal(routes.module, {}, original, terms))) return failure();
        auto queue = cast<ac::InstanceOp>(
            candidate.packageMap.lookup(original.operator->()));
        if (failed(originalQueueUnionProof(
                queue.getInputs()[4], terms, routes.module, original,
                queue.getOutputs()[1].getType())))
          return failure();
      }
      for (auto allocation : routes.module->getRegion(0).front().getOps<ac::InstanceOp>()) {
        auto child = dyn_cast_or_null<ac::ModuleOp>(hardware.resolveCallee(allocation));
        if (!child) continue;
        auto *childRoutes = inventory(child);
        auto *childDefinition = definition(child);
        if (!childRoutes || !childDefinition) return failure();
        auto actual = cast<ac::InstanceOp>(candidate.packageMap.lookup(allocation.operator->()));
        uint64_t input = allocation->getNumOperands() + 2 * childDefinition->downward.size();
        for (const auto &route : childRoutes->incoming) {
          if (!route.push) {
            SmallVector<ac::InstanceOp> path{allocation};
            llvm::append_range(path, route.path);
            SmallVector<Value> terms;
            if (failed(appendAncestor(routes.module, path, route.allocation, terms)) ||
                failed(appendLocal(routes.module, path, route.allocation, terms)) ||
                input >= actual->getNumOperands() || failed(originalQueueUnionProof(
                    actual->getOperand(input), terms, routes.module, route.allocation,
                    actual->getOperand(input).getType()))) return failure();
          }
          input += route.push ? 2 : 1;
        }
      }
    }
    return success();
  }
