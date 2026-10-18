// Private methods of the existing transaction state-plan builder. These use
// its value/domain/frame machinery; no independent analyzer or FIFO exists.
  FailureOr<QueueOwnerId> queueOwner(Operation *effect,
                                     LexicalInstanceId anchor,
                                     InstanceOp allocation) {
    auto module = plan.instances[anchor].definition;
    if (!allocation || allocation->getParentOp() != module ||
        !isQueueInstance(allocation))
      return effect->emitOpError()
             << "transaction queue owner is not its actual canonical FIFO instance";
    auto key = OwnerKey{anchor, allocation};
    auto found = queueOwnerIds.find(key);
    if (found != queueOwnerIds.end())
      return found->second;
    auto row = getSourceQueueOwnerRow(allocation);
    if (failed(row))
      return failure();
    auto data = useValue(allocation.getOutputs()[2], anchor, plan.rootFrame, effect);
    auto valid = useValue(allocation.getOutputs()[1], anchor, plan.rootFrame, effect);
    auto ready = useValue(allocation.getOutputs()[0], anchor, plan.rootFrame, effect);
    auto localCapacity = useValue(allocation.getOutputs()[3], anchor, plan.rootFrame, effect);
    auto inValid = useValue(allocation.getInputs()[2], anchor, plan.rootFrame, effect);
    auto inData = useValue(allocation.getInputs()[3], anchor, plan.rootFrame, effect);
    auto outReady = useValue(allocation.getInputs()[4], anchor, plan.rootFrame, effect);
    auto clock = useValue(allocation.getInputs()[0], anchor, plan.rootFrame, effect);
    auto reset = useValue(allocation.getInputs()[1], anchor, plan.rootFrame, effect);
    if (failed(data) || failed(valid) || failed(ready) || failed(inValid) ||
        failed(inData) || failed(outReady) || failed(localCapacity) || failed(clock) || failed(reset))
      return failure();
    auto domain = getDomain(*clock, *reset, effect);
    auto type = payloadType(*data, effect);
    auto config = hardware.resolveQueue(allocation, plan.instances[anchor].bindings);
    if (failed(domain) || failed(type) || failed(config) ||
        failed(sameDomain(*domain, effect)) ||
        failed(equivalent(*data, *inData, effect)))
      return failure();
    if (!areEquivalentHardwareTypes(*type, config->payloadType))
      return effect->emitOpError() << "transaction queue configuration payload type mismatch";
    auto constraint = row->get("payload_constraint");
    if (!budget.chargeOccurrence(effect, allocation.getOccurrence()) ||
        !budget.chargeAttribute(effect, constraint) ||
        !budget.chargeStorage<uint64_t>(effect, config->tokenShape.size()) ||
        !budget.chargeStorage<TransactionQueueOwner>(effect, 1) ||
        !budget.reserve(effect, plan.queueOwners, 1) ||
        !budget.reserveMapInsert(effect, queueOwnerIds))
      return failure();
    QueueOwnerId id = plan.queueOwners.size();
    plan.queueOwners.push_back({
        anchor, plan.instances[anchor].occurrencePath, allocation,
        allocation.getOccurrence(), *type, constraint, *domain,
        std::move(*config),
        allocation->hasAttr("ac.source_unbound_push"),
        allocation->hasAttr("ac.source_unbound_pop"),
        *data, *valid, *ready, *localCapacity, *inValid, *inData, *outReady});
    queueOwnerIds.try_emplace(key, id);
    return id;
  }

  FailureOr<bool> queuePathUnreachable(const TransactionQueueAccess &access,
                                        Operation *site) {
    if (access.pathFact && !access.pathFact.getValue())
      return true;
    auto path = access.point.path;
    for (unsigned depth = 0; depth <= maxDepth; ++depth) {
      if (!budget.charge(site))
        return failure();
      if (!path.branch)
        return false;
      if (*path.branch >= plan.branches.size())
        return site->emitOpError() << "queue access has an invalid source path";
      const auto &branch = plan.branches[*path.branch];
      if (branch.conditionFact && branch.conditionFact.getValue() != path.whenTrue)
        return true;
      path = branch.entry.path;
    }
    return boundedError(site) << "queue source path depth exceeded";
  }

  FailureOr<bool> queuePathsExclusive(TransactionSourcePath left,
                                      TransactionSourcePath right,
                                      Operation *site) {
    for (unsigned leftDepth = 0; leftDepth <= maxDepth; ++leftDepth) {
      if (!budget.charge(site))
        return failure();
      if (!left.branch)
        return false;
      if (*left.branch >= plan.branches.size())
        return site->emitOpError() << "queue access has an invalid source path";
      auto other = right;
      for (unsigned rightDepth = 0; rightDepth <= maxDepth; ++rightDepth) {
        if (!budget.charge(site))
          return failure();
        if (!other.branch)
          break;
        if (*other.branch >= plan.branches.size())
          return site->emitOpError() << "queue access has an invalid source path";
        if (left.branch == other.branch && left.whenTrue != other.whenTrue)
          return true;
        other = plan.branches[*other.branch].entry.path;
        if (rightDepth == maxDepth)
          return boundedError(site) << "queue source path depth exceeded";
      }
      left = plan.branches[*left.branch].entry.path;
    }
    return boundedError(site) << "queue source path depth exceeded";
  }

  LogicalResult queueAccessImpl(Operation *operation, InvocationFrameId frame,
                                InstanceOp allocation,
                                TransactionQueueAccessKind kind,
                                BoundValueId authoredPath,
                                std::optional<BoundValueId> payloadValue,
                                Value resultValue) {
    auto anchor = plan.frames[frame].anchor;
    auto pathFact = predicate(authoredPath, operation);
    if (failed(pathFact))
      return failure();
    auto owner = queueOwner(operation, anchor, allocation);
    if (failed(owner))
      return failure();
    TransactionQueueAccess record;
    record.operation = operation;
    record.frame = frame;
    record.owner = *owner;
    record.kind = kind;
    record.authoredPath = authoredPath;
    record.pathFact = *pathFact;
    record.point = point();
    record.occurrence = operation->getAttrOfType<DictionaryAttr>("occurrence");
    record.location = operation->getAttrOfType<DictionaryAttr>("ac.location");
    if (!record.location)
      return operation->emitOpError()
             << "native queue terminal requires its authored source span";
    const auto &queue = plan.queueOwners[*owner];
    bool push = record.kind == TransactionQueueAccessKind::Push;
    if ((push && !queue.unboundPush) ||
        (record.kind == TransactionQueueAccessKind::Pop && !queue.unboundPop))
      return operation->emitOpError()
             << "queue access conflicts with an externally bound raw direction";
    if (push) {
      if (!payloadValue || failed(equivalent(*payloadValue, queue.data, operation)))
        return failure();
      record.payload = *payloadValue;
    } else {
      // Static points retain source ancestry, not arbitrary raw-path predicates.
      // A consumed result must have a reached producer; unused conditional
      // accesses remain recorded for resource demand/knownness checks.
      if (!resultValue ||
          (!resultValue.use_empty() &&
           (!record.pathFact || !record.pathFact.getValue())))
        return operation->emitOpError()
               << "used queue read result requires a proven-true authored path";
      auto result = value(resultValue, anchor, frame);
      if (failed(result) || failed(assignValidity(*result, point(), operation)) ||
          failed(equivalent(*result, queue.data, operation)))
        return failure();
      record.result = *result;
    }
    SmallVector<QueueAccessId, 0> *claims = nullptr;
    if (record.kind != TransactionQueueAccessKind::Peek) {
      auto unreachable = queuePathUnreachable(record, operation);
      if (failed(unreachable))
        return failure();
      if (!*unreachable) {
        auto key = std::make_pair(record.owner, unsigned(push));
        auto found = queuePortClaims.find(key);
        if (found != queuePortClaims.end()) {
          claims = &found->second;
          for (QueueAccessId id : *claims) {
            if (!budget.charge(operation))
              return failure();
            const auto &previous = plan.queueAccesses[id];
            auto exclusive = queuePathsExclusive(previous.point.path,
                                                  record.point.path, operation);
            if (failed(exclusive))
              return failure();
            if (!*exclusive)
              return operation->emitOpError()
                     << "queue port conflict: potentially simultaneous same-port accesses; "
                        "source path exclusion is required";
          }
        } else {
          if (!budget.reserveMapInsert(operation, queuePortClaims))
            return failure();
          claims = &queuePortClaims.try_emplace(key).first->second;
        }
        if (!budget.chargeStorage<QueueAccessId>(operation, 1) ||
            !budget.reserve(operation, *claims, 1))
          return failure();
      }
    }
    if (!budget.chargeOccurrence(operation, record.occurrence) ||
        !budget.chargeAttribute(operation, record.location) ||
        !budget.chargeStorage<TransactionQueueAccess>(operation, 1) ||
        !budget.reserve(operation, plan.queueAccesses, 1))
      return failure();
    QueueAccessId id = plan.queueAccesses.size();
    plan.queueAccesses.push_back(std::move(record));
    if (claims)
      claims->push_back(id);
    return event(TransactionStateEventKind::QueueAccess, frame, id);
  }

  LogicalResult queueAccess(SourceMethodCallOp operation,
                            InvocationFrameId frame,
                            const ResolvedSourceMethod &resolved,
                            BoundValueId authoredPath,
                            ArrayRef<BoundValueId> actuals) {
    auto action = resolved.method.nativeQueueAction;
    auto kind = action == NativeQueueAction::Push
                    ? TransactionQueueAccessKind::Push
                    : action == NativeQueueAction::Pop
                          ? TransactionQueueAccessKind::Pop
                          : TransactionQueueAccessKind::Peek;
    if (!resolved.method.nativeQueueInstance ||
        (action == NativeQueueAction::Push
             ? actuals.size() != 1 || operation.getNumResults() != 0
             : !actuals.empty() || operation.getNumResults() != 1))
      return operation.emitOpError()
             << "native queue terminal signature differs from its checked contract";
    return queueAccessImpl(
        operation, frame, resolved.method.nativeQueueInstance, kind,
        authoredPath,
        action == NativeQueueAction::Push
            ? std::optional<BoundValueId>(actuals.front())
            : std::nullopt,
        action == NativeQueueAction::Push ? Value() : operation.getResult(0));
  }

  LogicalResult verifyQueueRecords() {
    auto site = plan.transaction.getOperation();
    auto invalid = [&]() -> LogicalResult {
      return site->emitOpError() << "transaction queue owner/access inventory is invalid";
    };
    for (const auto &queue : plan.queueOwners) {
      auto allocation = queue.allocation;
      if (!budget.charge(site) || queue.anchor >= plan.instances.size() ||
          !queue.allocation ||
          queue.allocation->getParentOp() != plan.instances[queue.anchor].definition ||
          queue.instancePath != plan.instances[queue.anchor].occurrencePath ||
          queue.occurrence != allocation.getOccurrence() ||
          !(queue.domain == plan.domain) ||
          !areEquivalentHardwareTypes(queue.payloadType, queue.config.payloadType) ||
          queue.unboundPush != queue.allocation->hasAttr("ac.source_unbound_push") ||
          queue.unboundPop != queue.allocation->hasAttr("ac.source_unbound_pop"))
        return invalid();
      for (auto [id, value] : {
               std::pair<BoundValueId, Value>{queue.data, allocation.getOutputs()[2]},
               {queue.valid, allocation.getOutputs()[1]},
               {queue.ready, allocation.getOutputs()[0]},
               {queue.localCapacity, allocation.getOutputs()[3]},
               {queue.inValid, allocation.getInputs()[2]},
               {queue.inData, allocation.getInputs()[3]},
               {queue.outReady, allocation.getInputs()[4]}}) {
        if (!budget.charge(site) || id >= plan.values.size())
          return invalid();
        const auto &ref = plan.values[id];
        if (ref.value != value || ref.anchor != queue.anchor ||
            ref.frame != plan.rootFrame || !samePoint(ref.validity, {}))
          return invalid();
      }
      Attribute constraint = queue.allocation->getAttr("ac.payload_constraint");
      if ((constraint && constraint != queue.payloadConstraint) ||
          (!constraint && !isa<UnitAttr>(queue.payloadConstraint)))
        return invalid();
    }
    for (const auto &access : plan.queueAccesses) {
      auto call = dyn_cast<SourceMethodCallOp>(access.operation);
      Value sourcePath = call ? call.getPath() : access.operation->getOperand(0);
      if (!budget.charge(site) || !access.operation ||
          access.frame >= plan.frames.size() || access.owner >= plan.queueOwners.size() ||
          access.authoredPath >= plan.values.size() ||
          plan.values[access.authoredPath].value != sourcePath ||
          plan.frames[access.frame].anchor != plan.queueOwners[access.owner].anchor ||
          access.occurrence != access.operation->getAttr("occurrence") ||
          access.location != access.operation->getAttr("ac.location") ||
          failed(requireAvailable(access.authoredPath, access.point, site)))
        return invalid();
      bool push = access.kind == TransactionQueueAccessKind::Push;
      if (!call ||
          (push ? (call.getArguments().size() != 1 ||
                   call.getNumResults() != 0)
                : (!call.getArguments().empty() ||
                   call.getNumResults() != 1)) ||
          access.payload.has_value() != push || access.result.has_value() == push)
        return invalid();
      if (push) {
        Value payload = call ? call.getArguments().front()
                             : access.operation->getOperand(1);
        if (*access.payload >= plan.values.size() ||
            plan.values[*access.payload].value != payload ||
            failed(requireAvailable(*access.payload, access.point, site)))
          return invalid();
      } else if (*access.result >= plan.values.size() ||
                 plan.values[*access.result].value !=
                     (call ? call.getResults().front()
                           : access.operation->getResult(0)) ||
                 !samePoint(plan.values[*access.result].validity, access.point))
        return invalid();
    }
    return success();
  }
