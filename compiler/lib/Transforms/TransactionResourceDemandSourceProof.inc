  LogicalResult resourceDemandProof(
      const TransactionPreparation &preparation,
      const MaterializedTransaction &record, ArrayRef<Value> sourceRaw,
      llvm::function_ref<Value(ac::BoundValueId)> bound) {
    const auto &plan = *preparation.plan;
    const auto &demand = record.resourceDemand;
    auto transaction = plan.transaction;
    auto rule = record.rule;
    if (demand.generated.empty() ||
        demand.reached.size() != preparation.controls.size())
      return reject("original resource demand inventory differs");
    Type bit = record.rule->getOperand(preparation.clockCapture).getType();
    Block *parent = &rule.getBody().front();
    Builder b(transaction.getContext());
    uint64_t cursor = 0;
    Operation *last = nullptr;
    auto take = [&](StringRef name, ArrayRef<Value> operands,
                    ArrayRef<Type> types, DictionaryAttr attributes,
                    Location location) -> FailureOr<Value> {
      if (cursor >= demand.generated.size() ||
          !charge(budget, plan.transaction, 4))
        return failure();
      Operation *operation = demand.generated[cursor++];
      if (!operation || !operations.contains(operation))
        return reject("original resource demand locator is not live");
      for (Value operand : operands)
        if (!operand || !values.contains(operand))
          return reject("original resource demand operand is not live");
      if (failed(generatedProof(operation, name, operands, types, attributes,
                                location, parent)) ||
          (last && !last->isBeforeInBlock(operation)))
        return reject("original resource demand operation order differs");
      last = operation;
      if (operation->getNumResults() != 1 ||
          !values.contains(operation->getResult(0)))
        return reject("original resource demand result is not live");
      return operation->getResult(0);
    };
    auto literal = [&](bool one) -> FailureOr<Value> {
      if (cursor >= demand.generated.size())
        return failure();
      Operation *operation = demand.generated[cursor];
      if (!operation || !operations.contains(operation) ||
          operation->getNumResults() != 1 ||
          failed(literalProof(operation->getResult(0), bit,
                              transaction.getOccurrence(),
                              transaction.getLoc(), one, parent)) ||
          (last && !last->isBeforeInBlock(operation)))
        return reject("original resource demand literal differs");
      last = operation;
      ++cursor;
      return operation->getResult(0);
    };
    auto conjunction = [&](Value raw, Value continuation,
                           Location location) -> FailureOr<Value> {
      return take(ac::BitsBinaryOp::getOperationName(), {raw, continuation},
                  {bit},
                  b.getDictionaryAttr({b.getNamedAttr(
                      "opcode", b.getStringAttr("and"))}),
                  location);
    };
    auto comparison = [&](Value lhs, Value rhs,
                          Location location) -> FailureOr<Value> {
      return take(ac::BitsCompareOp::getOperationName(), {lhs, rhs}, {bit},
                  b.getDictionaryAttr({b.getNamedAttr(
                      "predicate", b.getStringAttr("case_eq"))}),
                  location);
    };
    auto choice = [&](Value condition, Value whenTrue, Value whenFalse,
                      Location location) -> FailureOr<Value> {
      if (whenTrue == whenFalse)
        return whenTrue;
      return take(ac::BitsSelectOp::getOperationName(),
                  {condition, whenTrue, whenFalse}, {bit},
                  b.getDictionaryAttr({}), location);
    };

    auto one = literal(true);
    if (failed(one))
      return failure();
    auto boundary = [&](Value raw, Value continuation,
                        Location location) -> FailureOr<Value> {
      if (continuation == *one)
        return *one;
      auto knownTrue = comparison(raw, *one, location);
      if (failed(knownTrue))
        return failure();
      return choice(*knownTrue, continuation, *one, location);
    };
    using DemandSite = std::pair<ac::InvocationFrameId, Operation *>;
    llvm::DenseMap<DemandSite, SmallVector<uint64_t, 2>> requirementSites;
    for (auto [id, requirement] : llvm::enumerate(preparation.requirements)) {
      auto &sites = requirementSites[{requirement.frame, requirement.site}];
      if (!prepayAppend(budget, requirement.site, sites))
        return failure();
      sites.push_back(id);
    }
    auto requirementsAt = [&](Operation *site, ac::InvocationFrameId frame,
                              Value continuation) -> FailureOr<Value> {
      auto found = requirementSites.find({frame, site});
      if (found == requirementSites.end())
        return continuation;
      for (uint64_t id : llvm::reverse(found->second)) {
        if (id >= record.requirements.size() || !record.requirements[id])
          return reject("original resource demand value boundary is absent");
        auto requirement = record.requirements[id];
        auto next = boundary(requirement.getResult(), continuation,
                             site->getLoc());
        if (failed(next))
          return failure();
        continuation = *next;
      }
      return continuation;
    };

    // Rewalk intact source blocks and invocation frames.  Candidate demand
    // metadata never supplies a node kind, queue identity, branch ancestry or
    // continuation edge; the locator vector is accepted only after every
    // operation below is authenticated and globally covered.
    std::function<FailureOr<Value>(ac::InvocationFrameId, Value)> invocation;
    std::function<FailureOr<Value>(Block &, ac::InvocationFrameId, Value,
                                   unsigned)>
        statements;
    statements = [&](Block &source, ac::InvocationFrameId frame,
                     Value continuation, unsigned depth) -> FailureOr<Value> {
      if (depth > 128 || !charge(budget, plan.transaction))
        return failure();
      for (Operation &operation : llvm::reverse(source.without_terminator())) {
        if (!charge(budget, &operation))
          return failure();
        if (auto split = dyn_cast<ac::SourceBranchOp>(operation)) {
          auto found = llvm::find_if(plan.branches, [&](const auto &branch) {
            return branch.frame == frame && branch.operation == split;
          });
          if (found == plan.branches.end())
            return reject("original resource demand branch identity differs");
          if (found->selection) {
            auto whenTrue = statements(split.getThenRegion().front(), frame,
                                       continuation, depth + 1);
            auto whenFalse = statements(split.getElseRegion().front(), frame,
                                        continuation, depth + 1);
            Value condition = bound(found->condition);
            if (failed(whenTrue) || failed(whenFalse) || !condition)
              return failure();
            auto joined = choice(condition, *whenTrue, *whenFalse,
                                 split.getLoc());
            if (failed(joined))
              return failure();
            continuation = *joined;
          } else {
            auto whenFalse = statements(split.getElseRegion().front(), frame,
                                        continuation, depth + 1);
            if (failed(whenFalse))
              return failure();
            auto whenTrue = statements(split.getThenRegion().front(), frame,
                                       *whenFalse, depth + 1);
            if (failed(whenTrue))
              return failure();
            continuation = *whenTrue;
          }
        } else if (auto call = dyn_cast<ac::SourceMethodCallOp>(operation)) {
          auto access = llvm::find_if(plan.queueAccesses, [&](const auto &item) {
            return item.frame == frame && item.operation == call;
          });
          if (access != plan.queueAccesses.end()) {
            uint64_t accessId =
                std::distance(plan.queueAccesses.begin(), access);
            auto resource = llvm::find_if(
                preparation.resources, [&](const auto &candidate) {
                  return candidate.access == accessId;
                });
            if (resource == preparation.resources.end())
              return reject("original resource demand queue leaf is absent");
            uint64_t resourceId =
                std::distance(preparation.resources.begin(), resource);
            if (resourceId >= record.resources.size() ||
                !record.resources[resourceId])
              return reject(
                  "original resource demand queue resource is absent");
            auto selectedResource = record.resources[resourceId];
            Value raw = selectedResource.getTrueValue();
            if (!raw) return failure();
            auto next = conjunction(raw, continuation, call.getLoc());
            if (failed(next)) return failure();
            Value localPath = bound(access->authoredPath);
            if (!localPath) return failure();
            auto selected = choice(localPath, *next, continuation,
                                   call.getLoc());
            if (failed(selected)) return failure();
            continuation = *selected;
          } else {
            auto found = llvm::find_if(plan.calls, [&](const auto &candidate) {
              return candidate.caller == frame && candidate.operation == call;
            });
            if (found == plan.calls.end())
              return reject("original resource demand invocation differs");
            auto nested = invocation(found->callee, continuation);
            if (failed(nested))
              return failure();
            continuation = *nested;
          }
        } else if (isa<ac::SourceQueuePopOp, ac::SourceQueuePeekOp,
                       ac::SourceQueuePushOp>(operation)) {
          return reject("retired dedicated queue effect survived source proof");
        }
        auto required = requirementsAt(&operation, frame, continuation);
        if (failed(required))
          return failure();
        continuation = *required;
      }
      return continuation;
    };
    invocation = [&](ac::InvocationFrameId frame,
                     Value continuation) -> FailureOr<Value> {
      if (frame >= plan.frames.size())
        return reject("original resource demand frame is out of range");
      const auto &binding = plan.frames[frame];
      Operation *function = binding.method ? binding.method.operator->()
                                           : plan.transaction.operator->();
      auto bodyTail = requirementsAt(
          function->getRegion(1).front().getTerminator(), frame, continuation);
      if (failed(bodyTail))
        return failure();
      auto body = statements(function->getRegion(1).front(), frame, *bodyTail, 0);
      if (failed(body) || binding.guard >= plan.guards.size())
        return failure();
      const auto &guard = plan.guards[binding.guard];
      if (guard.frame != frame || guard.control >= sourceRaw.size() ||
          !sourceRaw[guard.control])
        return reject("original resource demand guard identity differs");
      auto guarded = boundary(sourceRaw[guard.control], *body,
                              function->getLoc());
      if (failed(guarded))
        return failure();
      auto guardTail = requirementsAt(
          function->getRegion(0).front().getTerminator(), frame, *guarded);
      if (failed(guardTail))
        return failure();
      return statements(function->getRegion(0).front(), frame, *guardTail, 0);
    };

    auto expectedDemand = invocation(plan.rootFrame, *one);
    auto zero = literal(false);
    if (failed(expectedDemand) || failed(zero))
      return failure();
    auto isZero = comparison(*expectedDemand, *zero, transaction.getLoc());
    if (failed(isZero))
      return failure();
    auto expectedMay = take(
        ac::BitsUnaryOp::getOperationName(), {*isZero}, {bit},
        b.getDictionaryAttr(
            {b.getNamedAttr("opcode", b.getStringAttr("not"))}),
        transaction.getLoc());
    if (failed(expectedMay) || demand.demand != *expectedDemand ||
        demand.mayExecute != *expectedMay)
      return reject("original resource demand root differs");
    for (auto [id, control] : llvm::enumerate(record.controls)) {
      auto reached = conjunction(control.reached, *expectedMay,
                                 transaction.getLoc());
      if (failed(reached) || demand.reached[id] != *reached)
        return reject("original resource demand reached gate differs");
    }
    if (cursor != demand.generated.size())
      return reject("original resource demand has extra operations");
    return success();
  }
