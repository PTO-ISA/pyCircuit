    SmallVector<Attribute> memoryProposalRows;
    uint64_t memoryProposalIndex = 0;
    if (record.memoryPaths.size() != plan.memoryAccesses.size() ||
        record.memoryReadPrefixes.size() != plan.memoryAccesses.size())
      return reject("original Memory path inventory differs");
    for (auto [accessId, access] : llvm::enumerate(plan.memoryAccesses)) {
      if (failed(rawStatementPathProof(record.memoryPaths[accessId], access.authoredPath,
                                       access.point.path, access.operation))) return failure();
      const auto &owner = plan.memoryOwners[access.owner];
      auto identity = b.getDictionaryAttr({
          b.getNamedAttr("owner", b.getDictionaryAttr({b.getNamedAttr("instance_path",
              routePath(transaction.getContext(), anchorPath(plan, owner.anchor)))})),
          b.getNamedAttr("declaration", owner.occurrence)});
      auto resource = llvm::find_if(preparation.resources, [&](const auto &entry) {
        return entry.memoryAccess == std::optional<uint64_t>(accessId);
      });
      if (access.kind == ac::TransactionMemoryAccessKind::Result) {
        uint64_t ordinal = std::distance(preparation.resources.begin(), resource);
        if (resource == preparation.resources.end() || ordinal >= record.resources.size() ||
            resourceRows[ordinal]) return reject("original Memory live resource is absent");
        auto predicate = owner.outputs[owner.layout.liveResult(access.port)];
        auto selected = record.resources[ordinal];
        auto kind = b.getStringAttr("memory_live");
        if (resource->predicate != predicate || resource->kind != kind ||
            !resource->priorPops.empty() || resource->externalPop ||
            !selected || !operations.contains(selected.operator->()) ||
            selected.getCondition() != record.memoryPaths[accessId] ||
            selected.getTrueValue() != bound(predicate) ||
            failed(literalProof(selected.getFalseValue(), bit, transaction.getOccurrence(),
                access.operation->getLoc(), true, &rule.getBody().front())) ||
            failed(generatedProof(selected, ac::BitsSelectOp::getOperationName(),
                {record.memoryPaths[accessId], bound(predicate), selected.getFalseValue()},
                {bit}, b.getDictionaryAttr({}), access.operation->getLoc(), &rule.getBody().front())))
          return reject("original Memory availability is not its committed live state");
        resourceRows[ordinal] = b.getDictionaryAttr({
            b.getNamedAttr("control", b.getI64IntegerAttr(resource->control)),
            b.getNamedAttr("kind", kind),
            b.getNamedAttr("port", b.getI64IntegerAttr(access.port)),
            b.getNamedAttr("owner", identity.get("owner")),
            b.getNamedAttr("declaration", owner.occurrence),
            b.getNamedAttr("occurrence", access.occurrence),
            b.getNamedAttr("location", access.location)});
        continue;
      }
      if (resource != preparation.resources.end())
        return reject("original Memory issue invented a readiness resource");
      if (memoryProposalIndex >= preparation.memoryProposals.size())
        return reject("original Memory proposal inventory is incomplete");
      const auto &proposal = preparation.memoryProposals[memoryProposalIndex++];
      std::optional<uint64_t> coveredBy;
      if (access.kind == ac::TransactionMemoryAccessKind::Read) {
        if (!replayFormalProof) {
          if (!charge(budget, access.operation, 1 + SourceCheckBudget::bytes(
                  sizeof(ac::detail::SourceFormalProposalProof)))) return failure();
          replayFormalProof = std::make_unique<ac::detail::SourceFormalProposalProof>(plan, hardware,
              [&](ac::BoundValueId id, ac::TransactionPoint context) { return replay.identity(id, context); },
              [&](Value source, ac::LexicalInstanceId anchor, ac::InvocationFrameId invocation) {
                return replay.find(source, anchor, invocation);
              }, [&](uint64_t work) { return charge(budget, transaction, work); });
        }
        auto sameAddress = [&](ac::BoundValueId left, ac::BoundValueId right,
                               ac::TransactionPoint point) -> FailureOr<bool> {
          auto identical = replayFormalProof->equal(left, right, point);
          if (failed(identical) || *identical) return identical;
          return ac::detail::memoryAddressesEqual(plan, left, right, point, hardware,
              [&](ac::BoundValueId id, ac::TransactionPoint at) {
                return replayFormalProof->identity(id, at);
              }, [&](uint64_t work) { return success(charge(budget, transaction, work)); });
        };
        // Earlier entries have already been authenticated against intact
        // source order, values and paths. Only retained reads can cover one.
        for (const auto &earlier : ArrayRef(preparation.memoryProposals).take_front(memoryProposalIndex - 1)) {
          if (earlier.physicalReuse) continue;
          auto covered = ac::detail::memoryReadCoveredBy(plan, earlier.access, accessId,
              sameAddress, [&](uint64_t work) { return success(charge(budget, access.operation, work)); });
          if (failed(covered)) return failure();
          if (*covered) { coveredBy = earlier.access; break; }
        }
      }
      if (proposal.physicalReuse != coveredBy)
        return reject("original Memory read physical reuse lacks source coverage");
      auto exclusions = sourceExclusions(access.point.path,
          access.kind == ac::TransactionMemoryAccessKind::Read ? proposal.reachedControlEnd : 0,
          access.operation);
      if (failed(exclusions) || *exclusions != proposal.excludedControls ||
          (access.kind != ac::TransactionMemoryAccessKind::Read && proposal.reachedControlEnd))
        return reject("original Memory read exclusions differ from source ancestry");
      if (proposal.access != accessId || proposal.argumentResults.size() != access.arguments.size() ||
          proposal.rawResult >= yield.getNumOperands() || proposal.pathResult >= yield.getNumOperands() ||
          yield->getOperand(proposal.rawResult) != record.memoryPaths[accessId])
        return reject("original Memory proposal source binding differs");
      SmallVector<Attribute> arguments;
      for (auto [id, result] : llvm::zip(access.arguments, proposal.argumentResults)) {
        if (result >= yield.getNumOperands() || yield->getOperand(result) != bound(id))
          return reject("original Memory proposal argument snapshot differs");
        arguments.push_back(b.getI64IntegerAttr(result));
      }
      StringRef kind = access.kind == ac::TransactionMemoryAccessKind::Request ? "request"
          : access.kind == ac::TransactionMemoryAccessKind::Read ? "read" : "write";
      memoryProposalRows.push_back(b.getDictionaryAttr({
          b.getNamedAttr("memory", identity), b.getNamedAttr("kind", b.getStringAttr(kind)),
          b.getNamedAttr("port", b.getI64IntegerAttr(access.port)),
          b.getNamedAttr("occurrence", access.occurrence), b.getNamedAttr("location", access.location),
          b.getNamedAttr("raw_path", b.getI64IntegerAttr(proposal.rawResult)),
          b.getNamedAttr("path", b.getI64IntegerAttr(proposal.pathResult)),
          b.getNamedAttr("arguments", b.getArrayAttr(arguments))}));
    }
    if (memoryProposalIndex != preparation.memoryProposals.size() ||
        (memoryProposalRows.empty() ? bool(preparation.expectedAdmission.get("memory_proposals"))
           : preparation.expectedAdmission.get("memory_proposals") != b.getArrayAttr(memoryProposalRows)))
      return reject("original Memory proposal metadata differs");
