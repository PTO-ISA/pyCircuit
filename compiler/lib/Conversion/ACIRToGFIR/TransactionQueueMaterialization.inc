  FailureOr<Value> queueRawPath(const ac::TransactionQueueAccess &access,
                                 OpBuilder &builder) {
    return rawStatementPath(access.operation, access.authoredPath,
                            access.point.path, builder);
  }

  FailureOr<Value> queueBareReadiness(const detail::TransactionQueueResource &resource,
                                       OpBuilder &builder) {
    auto capacity = bound(resource.predicate);
    if (failed(capacity)) return failure();
    if (!resource.externalPop) return *capacity;
    const auto &access = plan.queueAccesses[resource.access];
    const auto &owner = plan.queueOwners[access.owner];
    Value credit;
    for (uint64_t ordinal : resource.priorPops) {
      Value raw = queuePaths[prepared.queueProposals[ordinal].access];
      if (!raw) return access.operation->emitOpError() << "prospective prior pop path is not materialized";
      if (!credit) credit = raw;
      else {
        auto joined = binary(credit, raw, "or", access.operation->getLoc(), builder);
        if (failed(joined)) return failure();
        credit = *joined;
      }
    }
    Value external = builder.getBlock()->getArgument(prepared.externalPops[*resource.externalPop].capture);
    if (!credit) credit = external;
    else {
      auto joined = binary(credit, external, "or", access.operation->getLoc(), builder);
      if (failed(joined)) return failure();
      credit = *joined;
    }
    auto valid = bound(owner.valid);
    if (failed(valid)) return failure();
    auto pop = binary(*valid, credit, "and", access.operation->getLoc(), builder);
    if (failed(pop)) return failure();
    return binary(*capacity, *pop, "or", access.operation->getLoc(), builder);
  }

  LogicalResult emitQueueAccess(Operation *operation, ac::InvocationFrameId frame,
                                 OpBuilder &builder) {
    auto found = llvm::find_if(plan.queueAccesses, [&](const auto &access) {
      return access.operation == operation && access.frame == frame;
    });
    if (found == plan.queueAccesses.end())
      return operation->emitOpError() << "transaction queue access plan is missing";
    auto id = std::distance(plan.queueAccesses.begin(), found);
    auto path = queueRawPath(*found, builder);
    if (failed(path)) return failure();
    queuePaths[id] = *path;
    const auto &owner = plan.queueOwners[found->owner];
    if (found->result) {
      auto data = bound(owner.data);
      Value original = plan.values[*found->result].value;
      if (failed(data) || !prepayValueMapping(budget, operation, frames[frame], original))
        return failure();
      frames[frame].map(original, *data);
    }
    for (auto [ordinal, resource] : llvm::enumerate(prepared.resources)) {
      if (resource.memoryAccess || resource.access != static_cast<uint64_t>(id)) continue;
      auto raw = queueBareReadiness(resource, builder);
      auto one = intentLiteral(prefix.getType(), true, operation->getLoc(), builder);
      if (failed(raw) || failed(one)) return failure();
      auto selected = select(*path, *raw, *one, prefix.getType(), operation->getLoc(), builder);
      if (failed(selected)) return failure();
      resources[ordinal] = *selected;
      if (failed(admitControl(resource.control, selected->getResult(), operation->getLoc(),
                              builder, true))) return failure();
    }
    return success();
  }

  SmallVector<std::pair<uint64_t, Value>, 0> queuePrefixes;

  // Construct a prefix using the independently checked raw-control bypasses.
  // Queue proposals use the final boundary; async reads use their source point.
  FailureOr<Value> retainedPrefix(Operation *site, ArrayRef<uint64_t> excluded,
                                  uint64_t end, OpBuilder &builder) {
    if (end >= prefixes.size() || end > controls.size()) return failure();
    if (excluded.empty()) return prefixes[end];
    uint64_t first = excluded.front();
    if (first >= end) return failure();
    Value complete = prefixes[first];
    uint64_t omitted = 0;
    for (uint64_t ordinal = first; ordinal < end; ++ordinal) {
      if (!charge(budget, site)) return failure();
      if (omitted < excluded.size() && excluded[omitted] == ordinal) { ++omitted; continue; }
      const auto &scheduled = prepared.controls[ordinal];
      auto sourcePath = scheduled.source ? plan.controls[*scheduled.source].path
          : scheduled.requirement ? prepared.requirements[*scheduled.requirement].point.path
          : resourcePoint(plan, prepared.resources[*scheduled.resource]).path;
      auto projected = sampledPath(plan, sourcePath, budget);
      while (succeeded(projected) && !projected->empty()) {
        uint64_t source = projected->getAs<IntegerAttr>("selection").getInt();
        if (llvm::is_contained(excluded, prepared.sourceControls[source]))
          return site->emitOpError() << "retained resource bypass references an omitted ancestor selector";
        projected = sampledPath(plan, plan.controls[source].path, budget);
      }
      if (failed(projected)) return failure();
      // The bypass depends on its exact raw control and selector ancestry,
      // independently of the full reached/admission prefix.
      auto next = binary(complete, controls[ordinal].bypass, "and", site->getLoc(), builder);
      if (failed(next)) return failure();
      complete = *next;
    }
    if (omitted != excluded.size()) return failure();
    return complete;
  }

  FailureOr<Value> queueProposalPath(const TransactionQueueProposal &proposal,
                                      OpBuilder &builder) {
    const auto &access = plan.queueAccesses[proposal.access];
    if (!proposal.excludedControls.empty()) {
      for (auto [previous, value] : queuePrefixes) {
        if (!charge(budget, access.operation, 1 + proposal.excludedControls.size())) return failure();
        if (prepared.queueProposals[previous].excludedControls == proposal.excludedControls)
          return binary(value, queuePaths[proposal.access], "and", access.operation->getLoc(), builder);
      }
    }
    auto complete = retainedPrefix(access.operation, proposal.excludedControls, controls.size(), builder);
    if (failed(complete)) return failure();
    if (!proposal.excludedControls.empty()) {
      if (!prepayAppend(budget, access.operation, queuePrefixes)) return failure();
      queuePrefixes.push_back({uint64_t(&proposal - prepared.queueProposals.data()), *complete});
    }
    return binary(*complete, queuePaths[proposal.access], "and", access.operation->getLoc(), builder);
  }
