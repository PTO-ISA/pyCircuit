#include "TransactionQueueProspectiveSourceProof.inc"

  LogicalResult queueWiringProof() {
    Builder b(prepared.original->getContext());
    auto routeInventory = originalQueueRoutes();
    if (failed(routeInventory) || failed(originalQueuePopRoutingProof(*routeInventory)))
      return failure();
    SmallVector<Value> pending;
    for (const auto &definition : prepared.definitions) {
      auto module = cast<ac::ModuleOp>(candidate.packageMap.lookup(definition.module.operator->()));
      SmallVector<Attribute> inventory, incoming, writers, summaries, externalPops;
      uint64_t queueIndex = 0;
      for (auto original : definition.module->getRegion(0).front()
                               .getOps<ac::InstanceOp>()) {
        if (!ac::isQueueInstance(original)) continue;
        if (!charge(budget, original, 16) || queueIndex >= definition.queueIndices.size() ||
            prepared.queues[definition.queueIndices[queueIndex++]] != original) return failure();
        inventory.push_back(b.getDictionaryAttr({
            b.getNamedAttr("declaration", original.getOccurrence()),
            b.getNamedAttr("unbound_push", b.getBoolAttr(original->hasAttr("ac.source_unbound_push"))),
            b.getNamedAttr("unbound_pop", b.getBoolAttr(original->hasAttr("ac.source_unbound_pop")))}));
        auto queue = cast<ac::InstanceOp>(
            candidate.packageMap.lookup(original.operator->()));
        if (original->hasAttr("ac.source_unbound_push")) {
          pending.push_back(queue.getInputs()[2]);
          pending.push_back(queue.getInputs()[3]);
        }
        if (original->hasAttr("ac.source_unbound_pop"))
          pending.push_back(queue.getInputs()[4]);
      }
      if (queueIndex != definition.queueIndices.size()) return reject("original queue allocation inventory differs");
      uint64_t input = definition.sourceInputs + 2 * definition.downward.size();
      for (const auto &route : definition.queueIncoming) {
        NamedAttrList row;
        row.append("queue", b.getDictionaryAttr({b.getNamedAttr("owner", b.getDictionaryAttr({
            b.getNamedAttr("instance_path", routePath(module.getContext(), route.path))})),
            b.getNamedAttr("declaration", route.allocation->getAttr("occurrence"))}));
        row.append("kind", b.getStringAttr(route.push ? "push" : "pop"));
        row.append("path", b.getI64IntegerAttr(input++));
        if (route.push) row.append("data", b.getI64IntegerAttr(input++));
        incoming.push_back(row.getDictionary(module.getContext()));
      }
      for (uint64_t occurrenceIndex : definition.transactionIndices) {
        auto sourceIndex = originalTransactionIndex(occurrenceIndex);
        if (failed(sourceIndex)) return failure();
        uint64_t index = *sourceIndex;
        const auto &transaction = prepared.transactions[index];
        if (transaction.plan->routingModule != definition.module)
          return reject("queue writer occurrence routing owner differs");
        if (transaction.queueProposals.empty()) continue;
        SmallVector<Attribute> proposals;
        for (const auto &proposal : transaction.queueProposals) {
          const auto &access = transaction.plan->queueAccesses[proposal.access];
          const auto &owner = transaction.plan->queueOwners[access.owner];
          NamedAttrList row;
          row.append("queue", b.getDictionaryAttr({b.getNamedAttr("owner", b.getDictionaryAttr({
              b.getNamedAttr("instance_path", routePath(module.getContext(), anchorPath(*transaction.plan, owner.anchor)))})),
              b.getNamedAttr("declaration", owner.occurrence)}));
          row.append("kind", b.getStringAttr(access.payload ? "push" : "pop"));
          row.append("path", b.getI64IntegerAttr(proposal.pathResult));
          if (proposal.dataResult) row.append("data", b.getI64IntegerAttr(*proposal.dataResult));
          proposals.push_back(row.getDictionary(module.getContext()));
          auto rule = candidate.transactions[index].rule;
          pending.push_back(rule.getBody().front().getTerminator()->getOperand(proposal.pathResult));
        }
        writers.push_back(b.getDictionaryAttr({
            b.getNamedAttr("registration", routedOccurrence(*transaction.plan)),
            b.getNamedAttr("proposals", b.getArrayAttr(proposals))}));
      }
      for (auto [ordinal, route] : llvm::enumerate(definition.queuePopSummaries)) {
        auto identity = b.getDictionaryAttr({b.getNamedAttr("owner", b.getDictionaryAttr({
            b.getNamedAttr("instance_path", routePath(module.getContext(), route.path))})),
            b.getNamedAttr("declaration", route.allocation->getAttrOfType<DictionaryAttr>("occurrence"))});
        uint64_t output = definition.sourceOutputs + definition.upward.size() + ordinal;
        summaries.push_back(b.getDictionaryAttr({b.getNamedAttr("queue", identity),
            b.getNamedAttr("output", b.getI64IntegerAttr(output))}));
        auto yield = cast<ac::YieldOp>(module.getBody().front().getTerminator());
        if (output >= yield->getNumOperands()) return failure();
        pending.push_back(yield->getOperand(output));
      }
      for (uint64_t occurrenceIndex : definition.transactionIndices) {
        auto index = originalTransactionIndex(occurrenceIndex);
        if (failed(index)) return failure();
        const auto &transaction = prepared.transactions[*index];
        const auto &plan = *transaction.plan;
        if (plan.routingModule != definition.module) return reject("queue external occurrence routing owner differs");
        auto external = originalQueueExternalPops(transaction,
            transaction.liveIns.size());
        if (failed(external)) return failure();
        for (const auto &capture : *external) {
          const auto &owner = plan.queueOwners[capture.owner];
          auto identity = b.getDictionaryAttr({b.getNamedAttr("owner", b.getDictionaryAttr({
              b.getNamedAttr("instance_path", routePath(module.getContext(), anchorPath(plan, owner.anchor)))})),
              b.getNamedAttr("declaration", owner.occurrence)});
          externalPops.push_back(b.getDictionaryAttr({
              b.getNamedAttr("registration", routedOccurrence(plan)),
              b.getNamedAttr("queue", identity), b.getNamedAttr("capture", b.getI64IntegerAttr(capture.capture))}));
          pending.push_back(candidate.transactions[*index].rule->getOperand(capture.capture));
        }
      }
      DictionaryAttr expected;
      if (!inventory.empty() || !incoming.empty() || !writers.empty() || !summaries.empty() || !externalPops.empty()) {
        NamedAttrList metadata;
        metadata.append("queues", b.getArrayAttr(inventory));
        metadata.append("incoming", b.getArrayAttr(incoming));
        metadata.append("writers", b.getArrayAttr(writers));
        if (!summaries.empty()) metadata.append("pop_summaries", b.getArrayAttr(summaries));
        if (!externalPops.empty()) metadata.append("external_pops", b.getArrayAttr(externalPops));
        expected = metadata.getDictionary(b.getContext());
      }
      if (definition.expectedQueueWiring != expected || module->getAttr("ac.queue_wiring") != expected)
        return reject("original queue wiring inventory differs");
      for (auto original : definition.module->getRegion(0).front().getOps<ac::InstanceOp>()) {
        auto child = dyn_cast_or_null<ac::ModuleOp>(hardware.resolveCallee(original));
        if (!child) continue;
        const auto &routing = prepared.definitions[prepared.definitionIndex.lookup(child)];
        auto instance = cast<ac::InstanceOp>(candidate.packageMap.lookup(original.operator->()));
        uint64_t start = original->getNumOperands() + 2 * routing.downward.size();
        for (uint64_t i = 0; i < queueInputCount(routing); ++i)
          pending.push_back(instance->getOperand(start + i));
      }
    }
    // Cover the generated graph reachable from the source-authenticated pins.
    // verifyTransactionMaterialization also requires verifyRoutedPackage on this
    // unchanged candidate; that owns the full resource/merge/private-use proof
    // for every definition and actual binding before the candidate is accepted.
    DenseSet<Value> seen;
    while (!pending.empty()) {
      Value value = pending.pop_back_val();
      if (!charge(budget, prepared.original) || !value) return failure();
      if (!seen.insert(value).second || isa<BlockArgument>(value)) continue;
      auto *op = value.getDefiningOp();
      if (!op || clones.contains(op)) continue;
      if (auto rule = dyn_cast<ac::RuleOp>(op)) {
        pending.push_back(rule.getBody().front().getTerminator()->getOperand(cast<OpResult>(value).getResultNumber()));
        continue;
      }
      if (!ac::detail::isCombinationalHardwareOperation(op) || failed(cover(op)))
        return reject("original queue generated pin graph differs");
      llvm::append_range(pending, op->getOperands());
    }
    return success();
  }
