  FailureOr<detail::MaterializedResourceDemand>
  materializeResourceDemand(OpBuilder &builder) {
    detail::MaterializedResourceDemand result;
    auto transaction = plan.transaction;
    auto create = [&](StringRef name, ArrayRef<Value> operands,
                      ArrayRef<Type> types,
                      ArrayRef<NamedAttribute> attributes,
                      Location location) -> FailureOr<Operation *> {
      if (!charge(budget, plan.transaction,
                  4 + operands.size() + types.size() + attributes.size()) ||
          !prepayAppend(budget, plan.transaction, result.generated))
        return failure();
      OperationState state(location, name);
      state.addOperands(operands);
      state.addTypes(types);
      state.addAttributes(attributes);
      Operation *operation = builder.create(state);
      result.generated.push_back(operation);
      return operation;
    };
    auto literal = [&](bool one) -> FailureOr<Value> {
      if (!charge(budget, plan.transaction, 16))
        return failure();
      Value value = bitsLiteral(prefix.getType(), transaction.getOccurrence(),
                                transaction.getLoc(), one, builder);
      if (!prepayAppend(budget, plan.transaction, result.generated))
        return failure();
      result.generated.push_back(value.getDefiningOp());
      return value;
    };
    auto conjunction = [&](Value raw, Value continuation,
                           Location location) -> FailureOr<Value> {
      auto operation = create(
          ac::BitsBinaryOp::getOperationName(), {raw, continuation},
          {prefix.getType()},
          {builder.getNamedAttr("opcode", builder.getStringAttr("and"))},
          location);
      if (failed(operation))
        return failure();
      return (*operation)->getResult(0);
    };
    auto comparison = [&](Value lhs, Value rhs,
                          Location location) -> FailureOr<Value> {
      auto operation = create(
          ac::BitsCompareOp::getOperationName(), {lhs, rhs},
          {prefix.getType()},
          {builder.getNamedAttr("predicate",
                                builder.getStringAttr("case_eq"))},
          location);
      if (failed(operation))
        return failure();
      return (*operation)->getResult(0);
    };
    auto choice = [&](Value condition, Value whenTrue, Value whenFalse,
                      Location location) -> FailureOr<Value> {
      // Identical SSA arms preserve every four-state selector result.  Elide
      // only this exact common continuation; do not factor Boolean leaves.
      if (whenTrue == whenFalse)
        return whenTrue;
      auto operation = create(ac::BitsSelectOp::getOperationName(),
                              {condition, whenTrue, whenFalse},
                              {prefix.getType()}, {}, location);
      if (failed(operation))
        return failure();
      return (*operation)->getResult(0);
    };

    auto one = literal(true);
    if (failed(one))
      return failure();
    auto boundary = [&](Value raw, Value continuation,
                        Location location) -> FailureOr<Value> {
      // A false/unknown boundary bypasses to one.  When the suffix is already
      // that same literal, the exact select result is the literal for all raw
      // four-state values.
      if (continuation == *one)
        return *one;
      auto knownTrue = comparison(raw, *one, location);
      if (failed(knownTrue))
        return failure();
      return choice(*knownTrue, continuation, *one, location);
    };
    auto requirementsAt = [&](Operation *site, ac::InvocationFrameId frame,
                              Value continuation) -> FailureOr<Value> {
      auto found = requirementSites.find({frame, site});
      if (found == requirementSites.end())
        return continuation;
      for (uint64_t id : llvm::reverse(found->second)) {
        if (id >= requirements.size() || !requirements[id])
          return site->emitOpError()
                 << "resource demand requires a materialized value boundary";
        auto next = boundary(requirements[id].getResult(), continuation,
                             site->getLoc());
        if (failed(next))
          return failure();
        continuation = *next;
      }
      return continuation;
    };

    // Replay the actual expanded source operations.  In particular, calls are
    // expanded at their invocation frame and both branch arms receive the same
    // continuation value.  This retains selector/resource correlation without
    // reusing the already path-selected admission controls.
    std::function<FailureOr<Value>(ac::InvocationFrameId, Value)> invocation;
    std::function<FailureOr<Value>(Block &, ac::InvocationFrameId, Value,
                                   unsigned)>
        statements;
    statements = [&](Block &source, ac::InvocationFrameId frame,
                     Value continuation, unsigned depth) -> FailureOr<Value> {
      if (depth > 128)
        return transaction.emitOpError()
               << "resource demand branch depth budget exceeded";
      if (!charge(budget, transaction,
                  1 + SourceCheckBudget::bytes(sizeof(Block *) +
                                               sizeof(frame) + sizeof(depth))))
        return failure();
      for (Operation &operation : llvm::reverse(source.without_terminator())) {
        if (!charge(budget, &operation))
          return failure();
        if (auto split = dyn_cast<ac::SourceBranchOp>(operation)) {
          if (!charge(budget, split, plan.branches.size()))
            return failure();
          auto found = llvm::find_if(plan.branches, [&](const auto &branch) {
            return branch.frame == frame && branch.operation == split;
          });
          if (found == plan.branches.end())
            return split.emitOpError()
                   << "resource demand branch plan is missing";
          if (found->selection) {
            auto whenTrue = statements(split.getThenRegion().front(), frame,
                                       continuation, depth + 1);
            auto whenFalse = statements(split.getElseRegion().front(), frame,
                                        continuation, depth + 1);
            auto condition = bound(found->condition);
            if (failed(whenTrue) || failed(whenFalse) || failed(condition))
              return failure();
            auto joined = choice(*condition, *whenTrue, *whenFalse,
                                 split.getLoc());
            if (failed(joined))
              return failure();
            continuation = *joined;
          } else {
            // Nonselected branches contain no queue/call/diagnostic effects.
            // Their ValueRequire results already include their raw source path,
            // so retain the prepared lexical order without inventing a final
            // BranchSelect control.
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
            if (!charge(budget, call,
                        plan.queueAccesses.size() + prepared.resources.size()))
              return failure();
            uint64_t accessId =
                std::distance(plan.queueAccesses.begin(), access);
            auto resource = llvm::find_if(
                prepared.resources,
                [&](const auto &item) { return item.access == accessId; });
            if (resource != prepared.resources.end()) {
              uint64_t ordinal =
                  std::distance(prepared.resources.begin(), resource);
              FailureOr<Value> raw = resources[ordinal].getTrueValue();
              if (failed(raw)) return failure();
              auto next = conjunction(*raw, continuation, call.getLoc());
              if (failed(next)) return failure();
              auto localPath = bound(access->authoredPath);
              if (failed(localPath)) return failure();
              auto selected = choice(*localPath, *next, continuation,
                                     call.getLoc());
              if (failed(selected)) return failure();
              continuation = *selected;
            }
          } else {
            auto found = calls.find({frame, call});
            if (found == calls.end())
              return call.emitOpError()
                     << "resource demand call plan is missing";
            auto nested = invocation(found->second->callee, continuation);
            if (failed(nested))
              return failure();
            continuation = *nested;
          }
        } else if (isa<ac::SourceQueuePopOp, ac::SourceQueuePeekOp,
                       ac::SourceQueuePushOp>(operation)) {
          return operation.emitOpError()
                 << "retired dedicated queue effects are not admitted";
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
      if (frame >= plan.frames.size() ||
          !charge(budget, transaction,
                  1 + SourceCheckBudget::bytes(sizeof(frame))))
        return failure();
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
      auto raw = bound(plan.guards[binding.guard].predicate);
      if (failed(raw))
        return failure();
      auto guarded = boundary(*raw, *body, function->getLoc());
      if (failed(guarded))
        return failure();
      auto guardTail = requirementsAt(
          function->getRegion(0).front().getTerminator(), frame, *guarded);
      if (failed(guardTail))
        return failure();
      return statements(function->getRegion(0).front(), frame, *guardTail, 0);
    };

    auto demand = invocation(plan.rootFrame, *one);
    auto zero = literal(false);
    if (failed(demand) || failed(zero))
      return failure();
    auto isZero = comparison(*demand, *zero, transaction.getLoc());
    if (failed(isZero))
      return failure();
    auto inverted = create(
        ac::BitsUnaryOp::getOperationName(), {*isZero}, {prefix.getType()},
        {builder.getNamedAttr("opcode", builder.getStringAttr("not"))},
        transaction.getLoc());
    if (failed(inverted))
      return failure();
    result.demand = *demand;
    result.mayExecute = (*inverted)->getResult(0);
    if (!charge(budget, plan.transaction,
                SourceCheckBudget::multiply(
                    controls.size(), SourceCheckBudget::bytes(sizeof(Value)))) ||
        controls.size() > result.reached.max_size())
      return failure();
    result.reached.reserve(controls.size());
    for (const auto &control : controls) {
      auto reached = conjunction(control.reached, result.mayExecute,
                                 transaction.getLoc());
      if (failed(reached))
        return failure();
      result.reached.push_back(*reached);
    }
    return result;
  }
