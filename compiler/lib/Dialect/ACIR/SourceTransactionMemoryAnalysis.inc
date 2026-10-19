// Private Memory records in the existing transaction builder. State, values,
// invocation identity, source-path proofs and budgets remain shared with Queue.
  FailureOr<bool> sameMemoryReadAddress(BoundValueId left, BoundValueId right,
                                         TransactionPoint context, Operation *site) {
    if (failed(ensureFormalProof(site))) return failure();
    auto same = sameBoundValueAt(left, right, context, site);
    if (failed(same) || *same) return same;
    return detail::memoryAddressesEqual(plan, left, right, context, hardware,
        [&](BoundValueId id, TransactionPoint at) { return formalProof->identity(id, at); },
        [&](uint64_t work) { return success(budget.charge(site, work)); });
  }

  FailureOr<MemoryOwnerId> memoryOwner(SourceMethodCallOp operation,
                                       LexicalInstanceId anchor,
                                       InstanceOp allocation) {
    if (!allocation || allocation->getParentOp() != plan.instances[anchor].definition ||
        !isMemoryInstance(allocation))
      return operation.emitOpError() << "Memory terminal requires its actual lexical allocation";
    auto key = OwnerKey{anchor, allocation};
    auto found = memoryOwnerIds.find(key);
    if (found != memoryOwnerIds.end()) return found->second;
    auto row = getSourceMemoryOwnerRow(allocation);
    auto layout = getMemoryPrimitiveLayout(allocation, plan.instances[anchor].bindings);
    if (failed(row) || failed(layout)) return failure();
    TransactionMemoryOwner owner;
    owner.anchor = anchor;
    owner.instancePath = plan.instances[anchor].occurrencePath;
    owner.allocation = allocation;
    owner.occurrence = allocation.getOccurrence();
    owner.payloadType = layout->payloadType;
    owner.payloadConstraint = row->get("payload_constraint");
    owner.layout = std::move(*layout);
    auto bind = [&](ValueRange actuals, SmallVectorImpl<BoundValueId> &bound) -> LogicalResult {
      if (!budget.chargeStorage<BoundValueId>(operation, actuals.size()) ||
          !budget.reserve(operation, bound, actuals.size())) return failure();
      for (Value actual : actuals) {
        auto id = useValue(actual, anchor, plan.rootFrame, operation);
        if (failed(id)) return failure();
        bound.push_back(*id);
      }
      return success();
    };
    if (failed(bind(allocation.getInputs(), owner.inputs)) ||
        failed(bind(allocation.getOutputs(), owner.outputs))) return failure();
    auto domain = getDomain(owner.inputs[0], owner.inputs[1], operation);
    if (failed(domain) || failed(sameDomain(*domain, operation))) return failure();
    owner.domain = *domain;
    if (!budget.chargeOccurrence(operation, owner.occurrence) ||
        !budget.chargeAttribute(operation, owner.payloadConstraint) ||
        !budget.chargeStorage<TransactionMemoryOwner>(operation, 1) ||
        !budget.reserve(operation, plan.memoryOwners, 1) ||
        !budget.reserveMapInsert(operation, memoryOwnerIds)) return failure();
    MemoryOwnerId id = plan.memoryOwners.size();
    plan.memoryOwners.push_back(std::move(owner));
    memoryOwnerIds.try_emplace(key, id);
    return id;
  }

  LogicalResult memoryAccess(SourceMethodCallOp operation,
                              InvocationFrameId frame,
                              const ResolvedSourceMethod &resolved,
                              BoundValueId authoredPath,
                              ArrayRef<BoundValueId> actuals) {
    auto anchor = plan.frames[frame].anchor;
    auto ownerId = memoryOwner(operation, anchor, resolved.method.nativeMemoryInstance);
    auto pathFact = predicate(authoredPath, operation);
    if (failed(ownerId) || failed(pathFact)) return failure();
    const auto &owner = plan.memoryOwners[*ownerId];
    TransactionMemoryAccess record;
    record.operation = operation;
    record.frame = frame;
    record.owner = *ownerId;
    record.port = resolved.method.nativeMemoryPort;
    record.authoredPath = authoredPath;
    record.pathFact = *pathFact;
    record.point = point();
    record.occurrence = operation.getOccurrence();
    record.location = operation->getAttrOfType<DictionaryAttr>("ac.location");
    auto action = resolved.method.nativeMemoryAction;
    record.kind = action == NativeMemoryAction::Request ? TransactionMemoryAccessKind::Request
                  : action == NativeMemoryAction::Result ? TransactionMemoryAccessKind::Result
                  : action == NativeMemoryAction::Read ? TransactionMemoryAccessKind::Read
                                                       : TransactionMemoryAccessKind::Write;
    bool read = record.kind == TransactionMemoryAccessKind::Read ||
                record.kind == TransactionMemoryAccessKind::Result;
    unsigned arguments = record.kind == TransactionMemoryAccessKind::Write ? 3
                         : record.kind == TransactionMemoryAccessKind::Result ? 0 : 1;
    if (!record.location || record.port >= owner.layout.readPorts ||
        actuals.size() != arguments || operation.getNumResults() != unsigned(read))
      return operation.emitOpError() << "Memory terminal differs from its checked action/port signature";
    if (!budget.chargeStorage<BoundValueId>(operation, actuals.size()) ||
        !budget.reserve(operation, record.arguments, actuals.size())) return failure();
    record.arguments.assign(actuals.begin(), actuals.end());
    for (unsigned ordinal = 0; ordinal < actuals.size(); ++ordinal) {
      unsigned pin = record.kind == TransactionMemoryAccessKind::Write
                         ? owner.layout.writeAddressOperand + ordinal
                         : owner.layout.readAddressOperands[record.port];
      if (failed(equivalent(actuals[ordinal], owner.inputs[pin], operation))) return failure();
    }
    if (read) {
      Value source = operation.getResult(0);
      if (!source.use_empty() && (!record.pathFact || !record.pathFact.getValue()))
        return operation.emitOpError() << "used Memory result requires a proven-true authored path";
      auto id = value(source, anchor, frame);
      if (failed(id) || failed(assignValidity(*id, point(), operation)) ||
          failed(equivalent(*id, owner.outputs[record.port], operation))) return failure();
      record.result = *id;
    }
    SmallVector<MemoryAccessId, 0> *claims = nullptr;
    if (record.kind != TransactionMemoryAccessKind::Result) {
      auto unreachable = resourcePathUnreachable(record, operation);
      if (failed(unreachable)) return failure();
      if (!*unreachable) {
        unsigned port = record.kind == TransactionMemoryAccessKind::Write
                            ? owner.layout.readPorts : record.port;
        auto key = std::make_pair(record.owner, port);
        auto found = memoryPortClaims.find(key);
        if (found == memoryPortClaims.end()) {
          if (!budget.reserveMapInsert(operation, memoryPortClaims)) return failure();
          claims = &memoryPortClaims.try_emplace(key).first->second;
        } else {
          claims = &found->second;
          for (MemoryAccessId id : *claims) {
            if (!budget.charge(operation)) return failure();
            const auto &previous = plan.memoryAccesses[id];
            auto exclusive = resourcePathsExclusive(previous.point.path,
                                                     record.point.path, operation);
            if (failed(exclusive)) return failure();
            if (*exclusive) continue;
            // Byte reads share the combinational address pin when their
            // actual invocation-relative SSA addresses are proven equal.
            // Issue requests and writes never acquire this exemption.
            if (record.kind == TransactionMemoryAccessKind::Read &&
                previous.kind == TransactionMemoryAccessKind::Read) {
              auto same = sameMemoryReadAddress(previous.arguments.front(),
                                          record.arguments.front(), record.point,
                                          operation);
              if (failed(same)) return failure();
              if (*same) continue;
            }
            return operation.emitOpError() << "Memory port conflict: potentially simultaneous same-port accesses";
          }
        }
        if (!budget.chargeStorage<MemoryAccessId>(operation, 1) ||
            !budget.reserve(operation, *claims, 1)) return failure();
      }
    }
    if (!budget.chargeOccurrence(operation, record.occurrence) ||
        !budget.chargeAttribute(operation, record.location) ||
        !budget.chargeStorage<TransactionMemoryAccess>(operation, 1) ||
        !budget.reserve(operation, plan.memoryAccesses, 1)) return failure();
    MemoryAccessId id = plan.memoryAccesses.size();
    plan.memoryAccesses.push_back(std::move(record));
    if (claims) claims->push_back(id);
    return event(TransactionStateEventKind::MemoryAccess, frame, id);
  }

  LogicalResult verifyMemoryRecords() {
    auto site = plan.transaction.getOperation();
    auto invalid = [&]() -> LogicalResult {
      return site->emitOpError() << "transaction Memory owner/access inventory is invalid";
    };
    for (const auto &owner : plan.memoryOwners) {
      if (!budget.charge(site) || owner.anchor >= plan.instances.size() ||
          !owner.allocation || owner.allocation->getParentOp() != plan.instances[owner.anchor].definition ||
          owner.instancePath != plan.instances[owner.anchor].occurrencePath ||
          owner.occurrence != owner.allocation->getAttr("occurrence") ||
          owner.payloadConstraint != owner.allocation->getAttr("ac.payload_constraint") ||
          !(owner.domain == plan.domain)) return invalid();
      auto layout = getMemoryPrimitiveLayout(owner.allocation, plan.instances[owner.anchor].bindings);
      if (failed(layout) || layout->kind != owner.layout.kind ||
          layout->allocation != owner.layout.allocation ||
          layout->primitive != owner.layout.primitive ||
          !detail::sameHardwareBindings(layout->bindings, owner.layout.bindings) ||
          !areEquivalentHardwareTypes(layout->payloadType, owner.layout.payloadType) ||
          !areEquivalentHardwareTypes(layout->addressType, owner.layout.addressType) ||
          !areEquivalentHardwareTypes(layout->maskType, owner.layout.maskType) ||
          !areEquivalentHardwareTypes(layout->predicateType, owner.layout.predicateType) ||
          layout->readPorts != owner.layout.readPorts ||
          layout->readEnableOperands != owner.layout.readEnableOperands ||
          layout->readAddressOperands != owner.layout.readAddressOperands ||
          layout->writeEnableOperand != owner.layout.writeEnableOperand ||
          layout->writeAddressOperand != owner.layout.writeAddressOperand ||
          layout->writeDataOperand != owner.layout.writeDataOperand ||
          layout->writeMaskOperand != owner.layout.writeMaskOperand ||
          !areEquivalentHardwareTypes(layout->payloadType, owner.payloadType) ||
          owner.inputs.size() != owner.allocation->getNumOperands() ||
          owner.outputs.size() != owner.allocation->getNumResults()) return invalid();
      auto bindings = [&](ArrayRef<BoundValueId> ids, ValueRange values) -> LogicalResult {
        for (auto [id, actual] : llvm::zip(ids, values)) {
          if (!budget.charge(site) || id >= plan.values.size()) return invalid();
          const auto &ref = plan.values[id];
          if (ref.value != actual || ref.anchor != owner.anchor || ref.frame != plan.rootFrame ||
              !samePoint(ref.validity, {})) return invalid();
        }
        return success();
      };
      if (failed(bindings(owner.inputs, owner.allocation->getOperands())) ||
          failed(bindings(owner.outputs, owner.allocation->getResults()))) return failure();
    }
    for (const auto &access : plan.memoryAccesses) {
      auto call = dyn_cast_or_null<SourceMethodCallOp>(access.operation);
      if (!budget.charge(site) || !call || access.owner >= plan.memoryOwners.size() ||
          access.frame >= plan.frames.size() || access.authoredPath >= plan.values.size() ||
          plan.values[access.authoredPath].value != call.getPath() ||
          access.occurrence != call.getOccurrence() || access.location != call->getAttr("ac.location") ||
          failed(requireAvailable(access.authoredPath, access.point, site))) return invalid();
      const auto &owner = plan.memoryOwners[access.owner];
      if (plan.frames[access.frame].anchor != owner.anchor || access.port >= owner.layout.readPorts ||
          access.arguments.size() != call.getArguments().size()) return invalid();
      for (auto [id, source] : llvm::zip(access.arguments, call.getArguments()))
        if (!budget.charge(site) || id >= plan.values.size() || plan.values[id].value != source ||
            failed(requireAvailable(id, access.point, site))) return invalid();
      bool read = access.kind == TransactionMemoryAccessKind::Result || access.kind == TransactionMemoryAccessKind::Read;
      if (access.result.has_value() != read || call.getNumResults() != unsigned(read)) return invalid();
      if (read && (*access.result >= plan.values.size() ||
          plan.values[*access.result].value != call.getResult(0) ||
          !samePoint(plan.values[*access.result].validity, access.point))) return invalid();
    }
    return success();
  }
