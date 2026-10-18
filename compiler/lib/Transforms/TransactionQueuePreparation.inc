  LogicalResult queueControl(TransactionPreparation &prepared, ac::QueueAccessId id) {
    const auto &plan = *prepared.plan;
    const auto &access = plan.queueAccesses[id];
    const auto &owner = plan.queueOwners[access.owner];
    bool push = access.kind == ac::TransactionQueueAccessKind::Push;
    detail::TransactionQueueResource resource;
    resource.access = id;
    resource.control = prepared.controls.size();
    resource.predicate = push ? owner.localCapacity : owner.valid;
    resource.kind = Builder(package.getContext()).getStringAttr(push ? "queue_ready" : "queue_valid");
    if (push && owner.config.readyPolicy == ac::QueueReadyPolicy::DownstreamPop) {
      for (auto [ordinal, proposal] : llvm::enumerate(prepared.queueProposals)) {
        if (!charge(budget, access.operation)) return failure();
        const auto &prior = plan.queueAccesses[proposal.access];
        if (!proposal.unreachable && prior.owner == access.owner &&
            prior.kind == ac::TransactionQueueAccessKind::Pop) {
          if (!prepayAppend(budget, access.operation, resource.priorPops)) return failure();
          resource.priorPops.push_back(ordinal);
        }
      }
      if (!charge(budget, access.operation, prepared.externalPops.size())) return failure();
      auto external = llvm::find_if(prepared.externalPops, [&](const auto &item) {
        return item.owner == access.owner;
      });
      if (external == prepared.externalPops.end()) {
        if (!prepayAppend(budget, access.operation, prepared.externalPops)) return failure();
        resource.externalPop = prepared.externalPops.size();
        prepared.externalPops.push_back({access.owner,
            prepared.liveIns.size() + 2 * prepared.forwarding.size() + prepared.externalPops.size()});
      } else resource.externalPop = std::distance(prepared.externalPops.begin(), external);
    }
    if (!prepayAppend(budget, access.operation, prepared.resources) ||
        !prepayAppend(budget, access.operation, prepared.controls)) return failure();
    uint64_t resourceIndex = prepared.resources.size();
    prepared.resources.push_back(std::move(resource));
    prepared.controls.push_back({std::nullopt, std::nullopt, resourceIndex});
    bool unreachable = access.pathFact && !access.pathFact.getValue();
    auto ancestry = access.point.path;
    for (unsigned depth = 0; ancestry.branch && !unreachable; ++depth) {
      if (depth > 128 || !charge(budget, access.operation)) return failure();
      const auto &branch = plan.branches[*ancestry.branch];
      unreachable = branch.conditionFact && branch.conditionFact.getValue() != ancestry.whenTrue;
      ancestry = branch.entry.path;
    }
    if (access.kind != ac::TransactionQueueAccessKind::Peek) {
      if (!charge(budget, access.operation, 16)) return failure();
      TransactionQueueProposal proposal;
      proposal.access = id;
      proposal.unreachable = unreachable;
      proposal.resource = resourceIndex;
      prepared.queueProposals.push_back(proposal);
    }
    return success();
  }

  DictionaryAttr queueID(ArrayRef<ac::InstanceOp> path,
                         ac::InstanceOp queue) {
    Builder b(package.getContext());
    return b.getDictionaryAttr({
        b.getNamedAttr("owner", b.getDictionaryAttr({b.getNamedAttr(
            "instance_path", routePath(package.getContext(), path))})),
        b.getNamedAttr("declaration", queue.getOccurrence())});
  }

  uint64_t queueInputCount(const DefinitionRouting &definition) const {
    uint64_t count = 0;
    for (const auto &route : definition.queueIncoming) count += route.push ? 2 : 1;
    return count;
  }

  LogicalResult demandQueue(ac::ModuleOp module, ArrayRef<ac::InstanceOp> path,
                              ac::InstanceOp queue, bool push) {
    if (!charge(budget, module, 16 + path.size())) return failure();
    auto &definition = routing(module);
    QueueRoutingKey key{{routePath(package.getContext(), path),
                         queue.getOutputs()[1]}, push};
    if (definition.queueIncomingIndex.contains(key)) return success();
    ac::HardwareBindings bindings;
    bindings.owner = module;
    for (auto allocation : path) {
      auto child = hardware.bindInstance(allocation, bindings);
      if (failed(child)) return failure();
      bindings = *child;
    }
    auto payload = hardware.resolveType(queue.getOutputs()[2].getType(),
                                        bindings, module);
    auto predicate = hardware.resolveType(queue.getOutputs()[1].getType(),
                                          bindings, module);
    if (failed(payload) || failed(predicate)) return failure();
    definition.queueIncomingIndex.try_emplace(key, definition.queueIncoming.size());
    definition.queueIncoming.push_back({SmallVector<ac::InstanceOp>(path), queue,
                                       push, *payload, *predicate});
    return success();
  }

  LogicalResult demandQueuePopSummary(ac::ModuleOp module,
      ArrayRef<ac::InstanceOp> path, ac::InstanceOp queue) {
    if (!charge(budget, module, 16 + path.size())) return failure();
    auto &definition = routing(module);
    RoutingKey key{routePath(package.getContext(), path), queue.getOutputs()[1]};
    if (definition.queuePopSummaryIndex.contains(key)) return success();
    ac::HardwareBindings bindings;
    bindings.owner = module;
    for (auto allocation : path) {
      auto child = hardware.bindInstance(allocation, bindings);
      if (failed(child)) return failure();
      bindings = *child;
    }
    auto type = hardware.resolveType(queue.getOutputs()[1].getType(), bindings,
                                     module);
    if (failed(type)) return failure();
    definition.queuePopSummaryIndex.try_emplace(key, definition.queuePopSummaries.size());
    definition.queuePopSummaries.push_back({SmallVector<ac::InstanceOp>(path), queue, *type});
    return success();
  }

  LogicalResult queueProspectiveRoutes() {
    // Demand closure follows actual lexical Instance edges. Each parent may be
    // used as a root, so every ancestor input keeps its explicit zero default.
    bool changed;
    do {
      changed = false;
      for (auto &parent : definitions)
        for (auto allocation : parent.module.getBody().front().getOps<ac::InstanceOp>()) {
          if (!charge(budget, allocation)) return failure();
          auto child = dyn_cast_or_null<ac::ModuleOp>(hardware.resolveCallee(allocation));
          if (!child) continue;
          if (child == parent.module)
            return allocation.emitOpError() << "recursive Queue routing is invalid";
          for (const auto &route : routing(child).queueIncoming) {
            if (route.push) continue;
            SmallVector<ac::InstanceOp> path{allocation};
            llvm::append_range(path, route.path);
            auto before = parent.queueIncoming.size();
            if (failed(demandQueue(parent.module, path, route.allocation, false))) return failure();
            changed |= parent.queueIncoming.size() != before;
          }
        }
    } while (changed);
    return success();
  }

  LogicalResult queueRoutes(TransactionPreparation &prepared) {
    const auto &plan = *prepared.plan;
    for (const auto &proposal : prepared.queueProposals) {
      const auto &access = plan.queueAccesses[proposal.access];
      const auto &owner = plan.queueOwners[access.owner];
      auto path = anchorPath(plan, owner.anchor);
      for (unsigned offset = 0; offset < path.size(); ++offset) {
        auto child = dyn_cast<ac::ModuleOp>(hardware.resolveCallee(path[offset]));
        if (!child || failed(demandQueue(child,
              ArrayRef<ac::InstanceOp>(path).drop_front(offset + 1), owner.allocation,
              access.kind == ac::TransactionQueueAccessKind::Push))) return failure();
      }
    }
    for (const auto &external : prepared.externalPops) {
      const auto &owner = plan.queueOwners[external.owner];
      auto path = anchorPath(plan, owner.anchor);
      if (owner.unboundPop &&
          failed(demandQueue(plan.lexicalModule, path, owner.allocation, false))) return failure();
      for (unsigned offset = 0; offset < path.size(); ++offset) {
        auto child = dyn_cast<ac::ModuleOp>(hardware.resolveCallee(path[offset]));
        auto suffix = ArrayRef<ac::InstanceOp>(path).drop_front(offset + 1);
        if (!child || failed(demandQueuePopSummary(child, suffix, owner.allocation))) return failure();
      }
    }
    return success();
  }

  LogicalResult queueWiringMetadata(DefinitionRouting &definition) {
    Builder b(package.getContext());
    SmallVector<Attribute> queues, incoming, writers, summaries, externalPops;
    for (auto index : definition.queueIndices) {
      if (!charge(budget, definition.module, 8)) return failure();
      auto queue = this->queues[index];
      queues.push_back(b.getDictionaryAttr({
          b.getNamedAttr("declaration", queue.getOccurrence()),
          b.getNamedAttr("unbound_push", b.getBoolAttr(queue->hasAttr("ac.source_unbound_push"))),
          b.getNamedAttr("unbound_pop", b.getBoolAttr(queue->hasAttr("ac.source_unbound_pop")))}));
    }
    uint64_t input = definition.sourceInputs + 2 * definition.downward.size();
    for (const auto &route : definition.queueIncoming) {
      if (!charge(budget, definition.module, 32 + 8 * route.path.size())) return failure();
      NamedAttrList row;
      row.append("queue", queueID(route.path, route.allocation));
      row.append("kind", b.getStringAttr(route.push ? "push" : "pop"));
      row.append("path", b.getI64IntegerAttr(input++));
      if (route.push) row.append("data", b.getI64IntegerAttr(input++));
      incoming.push_back(row.getDictionary(b.getContext()));
    }
    for (auto index : definition.transactionIndices) {
      const auto &transaction = transactions[index];
      if (transaction.queueProposals.empty()) continue;
      const auto &plan = *transaction.plan;
      SmallVector<Attribute> proposals;
      for (const auto &proposal : transaction.queueProposals) {
        const auto &access = plan.queueAccesses[proposal.access];
        const auto &owner = plan.queueOwners[access.owner];
        if (!charge(budget, access.operation, 32 + 8 * owner.instancePath.size())) return failure();
        NamedAttrList row;
        row.append("queue", queueID(anchorPath(plan, owner.anchor), owner.allocation));
        row.append("kind", b.getStringAttr(access.kind == ac::TransactionQueueAccessKind::Push ? "push" : "pop"));
        row.append("path", b.getI64IntegerAttr(proposal.pathResult));
        if (proposal.dataResult) row.append("data", b.getI64IntegerAttr(*proposal.dataResult));
        proposals.push_back(row.getDictionary(b.getContext()));
      }
      writers.push_back(b.getDictionaryAttr({
          b.getNamedAttr("registration", plan.transaction->getAttr("occurrence")),
          b.getNamedAttr("proposals", b.getArrayAttr(proposals))}));
    }
    for (auto [ordinal, route] : llvm::enumerate(definition.queuePopSummaries)) {
      if (!charge(budget, definition.module, 32 + 8 * route.path.size())) return failure();
      summaries.push_back(b.getDictionaryAttr({
          b.getNamedAttr("queue", queueID(route.path, route.allocation)),
          b.getNamedAttr("output", b.getI64IntegerAttr(definition.sourceOutputs + definition.upward.size() + ordinal))}));
    }
    for (auto index : definition.transactionIndices) {
      const auto &transaction = transactions[index];
      for (const auto &external : transaction.externalPops) {
        const auto &plan = *transaction.plan;
        const auto &owner = plan.queueOwners[external.owner];
        if (!charge(budget, definition.module, 32 + 8 * owner.instancePath.size())) return failure();
        externalPops.push_back(b.getDictionaryAttr({
            b.getNamedAttr("registration", plan.transaction->getAttr("occurrence")),
            b.getNamedAttr("queue", queueID(anchorPath(plan, owner.anchor), owner.allocation)),
            b.getNamedAttr("capture", b.getI64IntegerAttr(external.capture))}));
      }
    }
    if (!queues.empty() || !incoming.empty() || !writers.empty() || !summaries.empty() || !externalPops.empty()) {
      NamedAttrList wiring;
      wiring.append("queues", b.getArrayAttr(queues));
      wiring.append("incoming", b.getArrayAttr(incoming));
      wiring.append("writers", b.getArrayAttr(writers));
      if (!summaries.empty()) wiring.append("pop_summaries", b.getArrayAttr(summaries));
      if (!externalPops.empty()) wiring.append("external_pops", b.getArrayAttr(externalPops));
      definition.expectedQueueWiring = wiring.getDictionary(b.getContext());
    }
    return success();
  }


  FailureOr<bool> queuePathsDisjoint(const ac::TransactionStatePlan &plan,
      ac::TransactionSourcePath left, ac::TransactionSourcePath right, Operation *site) {
    for (unsigned depth = 0; left.branch; ++depth) {
      if (depth > 128 || !charge(budget, site)) return failure();
      auto other = right;
      for (unsigned otherDepth = 0; other.branch; ++otherDepth) {
        if (otherDepth > 128 || !charge(budget, site)) return failure();
        if (left.branch == other.branch && left.whenTrue != other.whenTrue) return true;
        other = plan.branches[*other.branch].entry.path;
      }
      left = plan.branches[*left.branch].entry.path;
    }
    return false;
  }

  LogicalResult queueSpecializations(TransactionPreparation &prepared) {
    const auto &plan = *prepared.plan;
    for (auto &proposal : prepared.queueProposals) {
      const auto &access = plan.queueAccesses[proposal.access];
      for (auto [ordinal, control] : llvm::enumerate(prepared.controls)) {
        ac::TransactionSourcePath path;
        if (control.source) path = plan.controls[*control.source].path;
        else if (control.requirement) path = prepared.requirements[*control.requirement].point.path;
        else if (control.resource)
          path = plan.queueAccesses[prepared.resources[*control.resource].access].point.path;
        else return failure();
        auto disjoint = queuePathsDisjoint(plan, access.point.path, path, access.operation);
        if (failed(disjoint)) return failure();
        if (*disjoint) {
          if (!charge(budget, access.operation, 2 + SourceCheckBudget::bytes(sizeof(uint64_t))))
            return failure();
          proposal.excludedControls.push_back(ordinal);
        }
      }
    }
    return success();
  }

  LogicalResult queueAdmissionMetadata(TransactionPreparation &prepared) {
    const auto &plan = *prepared.plan;
    Builder b(package.getContext());
    SmallVector<Attribute> resources, proposals;
    for (const auto &resource : prepared.resources) {
      const auto &access = plan.queueAccesses[resource.access];
      const auto &owner = plan.queueOwners[access.owner];
      auto identity = queueID(anchorPath(plan, owner.anchor), owner.allocation);
      NamedAttrList row;
      row.append("control", b.getI64IntegerAttr(resource.control));
      row.append("kind", resource.kind);
      row.append("owner", identity.get("owner"));
      row.append("declaration", identity.get("declaration"));
      row.append("occurrence", access.occurrence);
      row.append("location", access.location);
      if (access.kind == ac::TransactionQueueAccessKind::Push) {
        NamedAttrList prospective;
        auto capture = [&](ac::BoundValueId value) -> uint64_t {
          return llvm::find(prepared.liveIns, value) - prepared.liveIns.begin();
        };
        prospective.append("capacity", b.getI64IntegerAttr(capture(owner.localCapacity)));
        if (owner.config.readyPolicy == ac::QueueReadyPolicy::DownstreamPop) {
          prospective.append("valid", b.getI64IntegerAttr(capture(owner.valid)));
          prospective.append("external_pop", b.getI64IntegerAttr(prepared.externalPops[*resource.externalPop].capture));
          SmallVector<Attribute> prior;
          for (uint64_t ordinal : resource.priorPops) prior.push_back(b.getI64IntegerAttr(ordinal));
          prospective.append("prior_pops", b.getArrayAttr(prior));
        }
        row.append("prospective", prospective.getDictionary(b.getContext()));
      }
      resources.push_back(row.getDictionary(b.getContext()));
    }
    for (const auto &proposal : prepared.queueProposals) {
      const auto &access = plan.queueAccesses[proposal.access];
      const auto &owner = plan.queueOwners[access.owner];
      NamedAttrList row;
      row.append("queue", queueID(anchorPath(plan, owner.anchor), owner.allocation));
      row.append("kind", b.getStringAttr(access.payload ? "push" : "pop"));
      row.append("occurrence", access.occurrence); row.append("location", access.location);
      row.append("raw_path", b.getI64IntegerAttr(proposal.rawResult));
      row.append("path", b.getI64IntegerAttr(proposal.pathResult));
      if (proposal.resource)
        row.append("resource_control", b.getI64IntegerAttr(prepared.resources[*proposal.resource].control));
      if (proposal.dataResult) row.append("data", b.getI64IntegerAttr(*proposal.dataResult));
      if (!proposal.excludedControls.empty()) {
        SmallVector<Attribute> excluded;
        for (uint64_t control : proposal.excludedControls) excluded.push_back(b.getI64IntegerAttr(control));
        row.append("excluded_controls", b.getArrayAttr(excluded));
      }
      proposals.push_back(row.getDictionary(b.getContext()));
    }
    NamedAttrList admission(prepared.expectedAdmission);
    if (!resources.empty()) admission.append("resources", b.getArrayAttr(resources));
    if (!proposals.empty()) admission.append("queue_proposals", b.getArrayAttr(proposals));
    prepared.expectedAdmission = admission.getDictionary(b.getContext());
    return success();
  }
