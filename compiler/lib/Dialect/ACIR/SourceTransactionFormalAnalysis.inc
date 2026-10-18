// Invocation-relative writable formals. Source metadata identifies a proposed
// binding; actual SSA, current candidates, logical constraints and domains prove
// it. Ordinary saved values never acquire owner identity from equal payloads.
  LogicalResult sameFormalConstraint(Attribute actual, Attribute expected,
                                     Operation *site) {
    auto lhs = dyn_cast_or_null<DictionaryAttr>(actual);
    auto rhs = dyn_cast_or_null<DictionaryAttr>(expected);
    if (!rhs)
      return site->emitOpError() << "writable formal lacks a logical input constraint";
    if (!lhs) {
      if (rhs.getAs<StringAttr>("source_kind").getValue() == "fixed_bits" &&
          isa<BitsType>(rhs.getAs<TypeAttr>("type").getValue()))
        return success();
      return site->emitOpError() << "writable owner lacks its logical payload constraint";
    }
    if (lhs.get("source_kind") != rhs.get("source_kind") ||
        lhs.get("domain") != rhs.get("domain"))
      return site->emitOpError() << "writable formal requires the owner's exact logical kind and domain";
    return success();
  }

  LogicalResult ensureFormalProof(Operation *function) {
      if (!formalProof) {
        if (!budget.chargeStorage<detail::SourceFormalProposalProof>(function, 1)) return failure();
        formalProof = std::make_unique<detail::SourceFormalProposalProof>(plan, hardware,
            [&](BoundValueId id, TransactionPoint context) -> FailureOr<BoundValueId> {
              auto normalized = normalize(id, plan.transaction);
              if (failed(normalized) || failed(requireAvailable(*normalized, context, plan.transaction)))
                return failure();
              return *normalized;
            },
            [&](Value original, LexicalInstanceId anchor, InvocationFrameId invocation) {
              return value(original, anchor, invocation);
            }, [&](uint64_t work) { return budget.charge(plan.transaction, work); });
      }
    return success();
  }

  FailureOr<bool> sameBoundValueAt(BoundValueId left, BoundValueId right,
                                   TransactionPoint context, Operation *site) {
    if (formalProof) return formalProof->equal(left, right, context);
    auto lhs = normalize(left, site), rhs = normalize(right, site);
    if (failed(lhs) || failed(rhs)) return failure();
    return *lhs == *rhs;
  }
  FailureOr<bool> sameBoundValue(BoundValueId left, BoundValueId right,
                                 Operation *site) {
    return sameBoundValueAt(left, right, point(), site);
  }

  LogicalResult bindFormalOwners(InvocationFrameId child,
                                  ArrayRef<BoundValueId> actuals) {
    auto &frame = plan.frames[child];
    auto call = frame.call;
    SourceMethodView method{plan.instances[frame.anchor].definition, frame.method, {}};
    if (!budget.chargeStorage<std::optional<StateOwnerId>>(call, actuals.size()) ||
        !budget.reserve(call, frame.formalOwners, actuals.size())) return failure();
    frame.formalOwners.resize(actuals.size());
    auto mappings = call->getAttrOfType<ArrayAttr>("ac.formal_owner_bindings");
    auto declaredProposals = method.getFormalProposals();
    if (declaredProposals && !declaredProposals.empty() && failed(ensureFormalProof(call)))
      return failure();
    if (mappings && !mappings.empty()) {
      for (Attribute raw : mappings) {
        if (!budget.charge(call)) return failure();
        auto formal = cast<DictionaryAttr>(raw).get("formal");
        if (!declaredProposals || !llvm::is_contained(declaredProposals, formal))
          return call.emitOpError() << "owner binding must identify an actual callee formal proposal";
      }
    }
    llvm::DenseSet<StateOwnerId> boundOwners;
    llvm::DenseSet<uint64_t> callerFormals;
    if (mappings) for (Attribute raw : mappings) {
      if (!budget.charge(call, 8)) return failure();
      auto row = cast<DictionaryAttr>(raw);
      unsigned formal = row.getAs<IntegerAttr>("formal").getInt();
      if (formal >= actuals.size()) return failure();
      auto constraint = cast<DictionaryAttr>(method.getParameters()[formal]).get("constraint");
      auto parent = *frame.parent;
      std::optional<StateOwnerId> ownerId;
      if (auto name = row.getAs<StringAttr>("state")) {
        auto physical = owner(call, plan.frames[parent].anchor, name);
        if (failed(physical)) return failure();
        ownerId = *physical;
        auto normalized = formalProof->identity(actuals[formal], point());
        if (failed(normalized)) return failure();
        const StateReadBinding *read = nullptr;
        for (const auto &candidate : plan.reads) {
          if (!budget.charge(call)) return failure();
          if (candidate.result == *normalized) { read = &candidate; break; }
        }
        if (!read || read->owner != *ownerId ||
            read->version != candidates[*ownerId])
          return call.emitOpError() << "writable owner actual must be its current whole candidate read";
        if (failed(sameFormalConstraint(plan.owners[*ownerId].declaration.getPayloadConstraint(),
                                         constraint, call))) return failure();
      } else {
        unsigned callerFormal = row.getAs<IntegerAttr>("caller_formal").getInt();
        if (callerFormal >= formalValues[parent].size() ||
            !callerFormals.insert(callerFormal).second)
          return call.emitOpError() << "overlapping or unresolved writable caller formal";
        auto same = sameBoundValue(actuals[formal], formalValues[parent][callerFormal], call);
        if (failed(same) || !*same)
          return call.emitOpError() << "writable caller formal actual must be its current SSA candidate";
        SourceMethodView caller{plan.instances[plan.frames[parent].anchor].definition,
                                plan.frames[parent].method, {}};
        auto parentConstraint = cast<DictionaryAttr>(caller.getParameters()[callerFormal]).get("constraint");
        if (failed(sameFormalConstraint(parentConstraint, constraint, call))) return failure();
        ownerId = plan.frames[parent].formalOwners[callerFormal];
      }
      if (ownerId) {
        if (!boundOwners.insert(*ownerId).second)
          return call.emitOpError() << "overlapping writable owner aliases";
        auto type = payloadType(actuals[formal], call);
        if (failed(type) ||
            !areEquivalentHardwareTypes(*type, plan.owners[*ownerId].payloadType))
          return call.emitOpError() << "writable owner actual has a different physical payload type";
      }
      frame.formalOwners[formal] = ownerId;
    }
    // Hidden proposals can drive an owner only with a matching binding. Removing
    // or repointing metadata cannot convert an reached write into a value call.
    auto proposals = method.getFormalProposals();
    if (!proposals) return success();
    for (auto [ordinal, rawFormal] : llvm::enumerate(proposals)) {
      unsigned formal = cast<IntegerAttr>(rawFormal).getInt();
      Value data = call->getResult(method.getNumPublicResults() + 2 * ordinal);
      Value intent = call->getResult(method.getNumPublicResults() + 2 * ordinal + 1);
      auto mapping = [&]() -> DictionaryAttr {
        if (mappings) for (Attribute raw : mappings) {
          auto row = cast<DictionaryAttr>(raw);
          if (row.getAs<IntegerAttr>("formal").getInt() == formal) return row;
        }
        return {};
      }();
      unsigned consumers = 0;
      for (Operation *user : data.getUsers()) {
        if (!budget.charge(call)) return failure();
        if (auto write = dyn_cast<SourceStateWriteOp>(user)) {
          if (write.getValue() != data || write.getPath() != intent || !mapping ||
              mapping.get("state") != write.getStateAttr())
            return call.emitOpError() << "owner proposal consumer requires its matching state binding";
          ++consumers;
        } else if (auto write = dyn_cast<SourceFormalWriteOp>(user)) {
          if (write.getPayload() != data) continue;
          if (!write.getPrevious() || write.getPath() != intent || !mapping ||
              mapping.get("caller_formal") != write.getFormalAttr())
            return call.emitOpError() << "forwarded proposal requires its matching caller formal binding";
          ++consumers;
        }
      }
      if (mapping && consumers != 1)
        return call.emitOpError() << "formal owner binding requires exactly one actual proposal effect consumer";
    }
    return success();
  }

  LogicalResult formalWrite(SourceFormalWriteOp operation,
                             InvocationFrameId frame,
                             SmallVectorImpl<StateOwnerId> &touched,
                             llvm::DenseSet<StateOwnerId> &seen) {
    auto anchor = plan.frames[frame].anchor;
    unsigned formal = operation.getFormalAttr().getInt();
    if (formal >= formalValues[frame].size()) return failure();
    if (failed(detail::verifySourceFormalCandidateSelection(operation, hardware,
          plan.frames[frame].bindings,
          [&](Value previous) -> FailureOr<bool> {
            auto bound = value(previous, anchor, frame);
            if (failed(bound)) return failure();
            return sameBoundValue(*bound, formalValues[frame][formal], operation);
          }, [&](uint64_t work) { return success(budget.charge(operation, work)); }))) return failure();
    auto payload = useValue(operation.getPayload(), anchor, frame, operation);
    auto path = useValue(operation.getPath(), anchor, frame, operation);
    auto candidate = useValue(operation.getCandidate(), anchor, frame, operation);
    if (failed(payload) || failed(path) || failed(candidate) ||
        formal >= formalValues[frame].size()) return failure();
    auto indices = writeIndices(operation, operation.getIndices(), frame);
    auto rootType = payloadType(formalValues[frame][formal], operation);
    auto payloadTypeAtWrite = payloadType(*payload, operation);
    auto selectedType = failed(rootType) || failed(indices) ? FailureOr<Type>(failure())
        : selectedWriteType(*rootType, operation.getSelectors(), *indices, frame, operation);
    auto pathFact = predicate(*path, operation);
    if (failed(pathFact) || failed(indices) || failed(rootType) || failed(payloadTypeAtWrite) ||
        failed(selectedType) || !areEquivalentHardwareTypes(*payloadTypeAtWrite, *selectedType))
      return operation.emitOpError() << "formal selected RHS differs from its actual owner slice";
    TransactionFormalWrite record;
    record.operation = operation;
    record.frame = frame;
    record.formal = formal;
    record.payload = *payload;
    record.authoredPath = *path;
    record.candidate = *candidate;
    record.owner = plan.frames[frame].formalOwners[formal];
    record.point = point();
    record.occurrence = operation.getOccurrence();
    record.location = operation.getSourceLocation();
    record.selectors = operation.getSelectors();
    record.indices = std::move(*indices);
    if (Value previous = operation.getPrevious()) {
      auto proposal = operation.getPayload().getDefiningOp<SourceMethodCallOp>();
      bool matched = false;
      if (proposal && proposal->getBlock() == operation->getBlock()) {
        auto binding = proposal->getAttrOfType<ArrayAttr>("ac.formal_owner_bindings");
        if (binding) for (Attribute raw : binding) {
          if (!budget.charge(operation)) return failure();
          auto row = cast<DictionaryAttr>(raw);
          if (row.get("caller_formal") == operation.getFormalAttr()) {
            auto resolved = resolutions.find({anchor, proposal});
            if (resolved == resolutions.end()) return failure();
            auto formals = resolved->second.method.getFormalProposals();
            auto ordinal = llvm::find(formals, row.get("formal")) - formals.begin();
            if (ordinal < static_cast<ptrdiff_t>(formals.size())) {
              unsigned base = resolved->second.method.getNumPublicResults() + 2 * ordinal;
              matched = proposal->getResult(base) == operation.getPayload() &&
                        proposal->getResult(base + 1) == operation.getPath();
            }
          }
        }
      }
      if (!matched) return operation.emitOpError() << "forwarded formal requires the actual bound callee proposal data and intent";
      auto old = useValue(previous, anchor, frame, operation);
      auto snapshot = value(operation.getSnapshot(), anchor, frame);
      if (failed(old) || failed(snapshot)) return failure();
      auto same = sameBoundValue(*old, formalValues[frame][formal], operation);
      if (failed(same) || !*same)
        return operation.emitOpError() << "forwarded formal previous is not its current snapshot";
      record.previous = *old;
      record.snapshot = *snapshot;
      auto selected = record.owner ? *candidate : *old;
      if (failed(assignValidity(*snapshot, point(), operation)) ||
          failed(alias(*snapshot, selected, operation))) return failure();
      formalValues[frame][formal] = *snapshot;
    } else formalValues[frame][formal] = *candidate;
    if (!formalProof || formal >= formalIntents[frame].size()) return failure();
    auto intent = formalProof->write(formalIntents[frame][formal], *path, *pathFact,
                                     bool(operation.getPrevious()));
    if (failed(intent)) return failure();
    formalIntents[frame][formal] = *intent;
    if (!budget.chargeStorage<BoundValueId>(operation, 2 * record.indices.size()) ||
        !budget.chargeStorage<TransactionFormalWrite>(operation, 1) ||
        !budget.reserve(operation, plan.formalWrites, 1)) return failure();
    auto id = plan.formalWrites.size();
    plan.formalWrites.push_back(record);
    if (record.owner) {
      StateOwnerId state = *record.owner;
      auto type = payloadType(*candidate, operation);
      if (failed(type) || !areEquivalentHardwareTypes(*type, plan.owners[state].payloadType))
        return operation.emitOpError() << "formal candidate differs from bound owner type";
      if (!budget.chargeStorage<StateWriteIntent>(operation, 1) ||
          !budget.reserve(operation, plan.writes, 1) ||
          !budget.chargeStorage<StateVersion>(operation, 1) ||
          !budget.reserve(operation, plan.versions, 1)) return failure();
      StateWriteId write = plan.writes.size();
      StateVersionId version = plan.versions.size();
      StateVersionId predecessor = candidates[state];
      plan.writes.push_back({operation, frame, state, predecessor, version,
                            *candidate, *path, *pathFact,
                            record.selectors, record.indices, operation.getOccurrence(),
                            record.payload, bool(operation.getPrevious())});
      plan.versions.push_back({plan.transaction, state, ++ownerOrdinals[state],
          StateVersionKind::Write, predecessor, write, std::nullopt, *candidate,
          plan.owners[state].payloadType, point()});
      plan.formalWrites.back().write = write;
      candidates[state] = version;
      if (failed(touch(touched, seen, state, operation))) return failure();
    }
    return event(TransactionStateEventKind::FormalWrite, frame, id);
  }
