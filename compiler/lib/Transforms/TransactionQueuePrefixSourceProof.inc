    auto retainedPrefixProof = [&](Value complete, ArrayRef<uint64_t> excluded,
                                   uint64_t end, Operation *site) -> LogicalResult {
      if (end >= record.prefixes.size() || end > preparation.controls.size()) return failure();
      if (excluded.empty())
        return complete == record.prefixes[end] ? success()
            : reject("original resource complete prefix differs");
      uint64_t first = excluded.front();
      if (first >= end) return failure();
      Value cursor = complete;
      for (uint64_t index = end; index > first; --index) {
        uint64_t control = index - 1;
        if (!charge(budget, site)) return failure();
        if (llvm::is_contained(excluded, control)) continue;
        // The ordinary control loop above has already independently proved
        // this bypass from its exact raw predicate and pure selector ancestry.
        const auto &scheduled = preparation.controls[control];
        auto sourcePath = scheduled.source ? plan.controls[*scheduled.source].path
            : scheduled.requirement ? preparation.requirements[*scheduled.requirement].point.path
            : detail::resourcePoint(plan, preparation.resources[*scheduled.resource]).path;
        auto projected = sampledPath(plan, sourcePath, budget);
        while (succeeded(projected) && !projected->empty()) {
          uint64_t source = projected->getAs<IntegerAttr>("selection").getInt();
          if (llvm::is_contained(excluded, preparation.sourceControls[source]))
            return reject("original queue bypass references an omitted selector");
          projected = sampledPath(plan, plan.controls[source].path, budget);
        }
        if (failed(projected)) return failure();
        auto joined = cursor.getDefiningOp<ac::BitsBinaryOp>();
        if (!joined || joined.getOpcode() != "and" || joined.getRhs() != record.controls[control].bypass ||
            failed(binaryProof(cursor, joined.getLhs(), record.controls[control].bypass,
                               "and", site->getLoc())))
          return reject("original queue retained bypass differs");
        cursor = joined.getLhs();
      }
      if (cursor != record.prefixes[first])
        return reject("original queue retained prefix has a foreign entry");
      return success();
    };
    SmallVector<std::pair<uint64_t, Value>, 0> queuePrefixProofs;
    for (auto [ordinal, proposal] : llvm::enumerate(preparation.queueProposals)) {
      const auto &access = plan.queueAccesses[proposal.access];
      auto request = yield->getOperand(proposal.pathResult).getDefiningOp<ac::BitsBinaryOp>();
      if (!request || request.getOpcode() != "and" ||
          request.getRhs() != record.queuePaths[proposal.access])
        return reject("original queue request raw path differs");
      Value complete = request.getLhs();
      if (failed(binaryProof(request.getResult(), complete, record.queuePaths[proposal.access],
                              "and", access.operation->getLoc()))) return failure();
      if (proposal.excludedControls.empty()) {
        if (complete != prefix) return reject("original queue complete prefix differs");
        continue;
      }
      bool shared = false;
      for (auto [previous, expected] : queuePrefixProofs) {
        if (!charge(budget, access.operation, 1 + proposal.excludedControls.size())) return failure();
        if (preparation.queueProposals[previous].excludedControls != proposal.excludedControls) continue;
        if (complete != expected) return reject("original queue shared retained-control prefix differs");
        shared = true;
        break;
      }
      if (shared) continue;
      if (failed(retainedPrefixProof(complete, proposal.excludedControls,
                                      preparation.controls.size(), access.operation)))
        return failure();
      if (!prepayAppend(budget, access.operation, queuePrefixProofs)) return failure();
      queuePrefixProofs.push_back({ordinal, complete});
    }
