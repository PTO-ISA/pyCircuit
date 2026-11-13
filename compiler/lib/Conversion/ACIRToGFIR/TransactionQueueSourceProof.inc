    // Replay the raw source path independently. Prefix/knownness and private
    // pin closure are checked by the final resource verifier on actual SSA.
    auto rawStatementPathProof = [&](Value path, ac::BoundValueId authoredPath,
                                     ac::TransactionSourcePath source,
                                     Operation *operation) -> LogicalResult {
      auto peel = [&](Value expected) -> LogicalResult {
        auto conjunction = path.getDefiningOp<ac::BitsBinaryOp>();
        if (!conjunction || conjunction.getOpcode() != "and" ||
            conjunction.getRhs() != expected ||
            failed(binaryProof(path, conjunction.getLhs(), expected, "and", operation->getLoc())))
          return reject("original queue raw path differs");
        path = conjunction.getLhs(); return success();
      };
      if (failed(peel(bound(authoredPath)))) return failure();
      SmallVector<std::pair<ac::BranchId, bool>> ancestry;
      while (source.branch) {
        if (!charge(budget, operation) || ancestry.size() > 128) return failure();
        ancestry.push_back({*source.branch, source.whenTrue});
        source = plan.branches[*source.branch].entry.path;
      }
      for (auto [branch, positive] : llvm::reverse(ancestry)) {
        Value selector = bound(plan.branches[branch].condition);
        if (!positive) {
          auto conjunction = path.getDefiningOp<ac::BitsBinaryOp>();
          auto invert = conjunction ? conjunction.getRhs().getDefiningOp<ac::BitsUnaryOp>() : ac::BitsUnaryOp();
          if (!invert || invert.getInput() != selector ||
              failed(generatedProof(invert, ac::BitsUnaryOp::getOperationName(), {selector}, {selector.getType()},
                b.getDictionaryAttr({b.getNamedAttr("opcode", b.getStringAttr("not"))}),
                operation->getLoc(), &rule.getBody().front()))) return failure();
          selector = invert.getResult();
        }
        if (failed(peel(selector))) return failure();
      }
      if (failed(literalProof(path, bit, transaction.getOccurrence(), operation->getLoc(),
                              true, &rule.getBody().front()))) return failure();
      return success();
    };
    for (auto [id, access] : llvm::enumerate(plan.queueAccesses))
      if (failed(rawStatementPathProof(record.queuePaths[id], access.authoredPath,
                                       access.point.path, access.operation)))
        return failure();
    auto sourceExclusions = [&](ac::TransactionSourcePath accessPath, uint64_t end,
                                Operation *site) -> FailureOr<SmallVector<uint64_t, 0>> {
      if (end > preparation.controls.size()) return failure();
      SmallVector<uint64_t, 0> excluded;
      for (uint64_t ordinal = 0; ordinal < end; ++ordinal) {
        const auto &scheduled = preparation.controls[ordinal];
        auto controlPath = scheduled.source ? plan.controls[*scheduled.source].path
            : scheduled.requirement ? preparation.requirements[*scheduled.requirement].point.path
            : detail::resourcePoint(plan, preparation.resources[*scheduled.resource]).path;
        auto left = accessPath;
        bool disjoint = false;
        for (unsigned depth = 0; left.branch; ++depth) {
          if (depth > 128 || !charge(budget, site)) return failure();
          auto right = controlPath;
          for (unsigned otherDepth = 0; right.branch; ++otherDepth) {
            if (otherDepth > 128 || !charge(budget, site)) return failure();
            disjoint |= left.branch == right.branch && left.whenTrue != right.whenTrue;
            right = plan.branches[*right.branch].entry.path;
          }
          left = plan.branches[*left.branch].entry.path;
        }
        if (disjoint) {
          if (!prepayAppend(budget, site, excluded)) return failure();
          excluded.push_back(ordinal);
        }
      }
      return excluded;
    };
    auto prospectiveBinaryProof = [&](Value output, Value lhs, Value rhs,
                                      StringRef opcode, Location location) -> LogicalResult {
      auto result = dyn_cast_or_null<OpResult>(output);
      if (!result || result.getResultNumber() != 0 || !lhs || !rhs ||
          !isa<ac::BitsType>(lhs.getType()) || !isa<ac::BitsType>(rhs.getType()) ||
          !ac::areEquivalentHardwareTypes(lhs.getType(), bit) ||
          !ac::areEquivalentHardwareTypes(rhs.getType(), bit))
        return reject("original prospective queue binary type differs");
      // RuleMaterializer::binary preserves the exact lhs type, including its
      // original static-width provenance. Equivalent Bits1 operands may come
      // from a different lexical Module's clock/type source.
      return generatedProof(result.getOwner(), ac::BitsBinaryOp::getOperationName(),
          {lhs, rhs}, {lhs.getType()}, b.getDictionaryAttr({
              b.getNamedAttr("opcode", b.getStringAttr(opcode))}),
          location, &rule.getBody().front());
    };
    SmallVector<Attribute> resourceRows(preparation.resources.size()), proposalRows;
    uint64_t resourceIndex = 0, proposalIndex = 0;
    for (auto [accessId, access] : llvm::enumerate(plan.queueAccesses)) {
      const auto &owner = plan.queueOwners[access.owner];
      auto identity = b.getDictionaryAttr({b.getNamedAttr("owner", b.getDictionaryAttr({
          b.getNamedAttr("instance_path", routePath(transaction.getContext(), anchorPath(plan, owner.anchor)))})),
          b.getNamedAttr("declaration", owner.occurrence)});
      // Locate by original access identity; Memory resources interleave in
      // source order and are authenticated separately below.
      auto resourceEntry = llvm::find_if(preparation.resources, [&](const auto &entry) {
        return !entry.memoryAccess && entry.access == accessId;
      });
      resourceIndex = std::distance(preparation.resources.begin(), resourceEntry);
      if (resourceIndex >= preparation.resources.size() ||
          resourceIndex >= record.resources.size() || resourceRows[resourceIndex])
        return reject("original queue resource inventory is incomplete");
      std::optional<uint64_t> resource = resourceIndex;
      const auto &entry = preparation.resources[resourceIndex];
      auto selected = record.resources[resourceIndex];
      auto predicate = access.payload ? owner.localCapacity : owner.valid;
      auto kind = b.getStringAttr(access.payload ? "queue_ready" : "queue_valid");
      if (entry.access != accessId || entry.predicate != predicate || entry.kind != kind ||
          !selected || !operations.contains(selected.operator->()) ||
          selected.getCondition() != record.queuePaths[accessId])
        return reject("original queue resource predicate/path differs");
      Value bare = bound(predicate);
      NamedAttrList prospective;
      SmallVector<uint64_t, 0> priorPops;
      std::optional<uint64_t> externalPop;
      if (access.payload) {
        auto capacity = llvm::find(preparation.liveIns, owner.localCapacity);
        if (capacity == preparation.liveIns.end())
          return reject("original queue capacity capture is absent");
        prospective.append("capacity", b.getI64IntegerAttr(
            std::distance(preparation.liveIns.begin(), capacity)));
        if (owner.config.readyPolicy == ac::QueueReadyPolicy::DownstreamPop) {
          uint64_t priorOrdinal = 0;
          for (uint64_t previous = 0; previous < accessId; ++previous) {
            const auto &earlier = plan.queueAccesses[previous];
            if (!charge(budget, access.operation)) return failure();
            if (earlier.kind == ac::TransactionQueueAccessKind::Peek) continue;
            auto zero = originalQueueUnreachable(plan, earlier);
            if (failed(zero)) return failure();
            if (earlier.kind == ac::TransactionQueueAccessKind::Pop &&
                earlier.owner == access.owner && !*zero) {
              if (!prepayAppend(budget, access.operation, priorPops)) return failure();
              priorPops.push_back(priorOrdinal);
            }
            ++priorOrdinal;
          }
          for (auto [ordinal, external] : llvm::enumerate(preparation.externalPops))
            if (external.owner == access.owner) {
              if (externalPop) return reject("original queue external capture is duplicated");
              externalPop = ordinal;
            }
          auto valid = llvm::find(preparation.liveIns, owner.valid);
          if (!externalPop || valid == preparation.liveIns.end())
            return reject("original prospective queue capture is absent");
          const auto &external = preparation.externalPops[*externalPop];
          if (external.capture >= rule->getNumOperands() ||
              external.capture >= rule.getBody().front().getNumArguments() ||
              !isa<ac::BitsType>(rule->getOperand(external.capture).getType()) ||
              !isa<ac::BitsType>(rule.getBody().front().getArgument(external.capture).getType()) ||
              !ac::areEquivalentHardwareTypes(rule->getOperand(external.capture).getType(),
                                              bound(owner.valid).getType()) ||
              !ac::areEquivalentHardwareTypes(
                  rule.getBody().front().getArgument(external.capture).getType(),
                  bound(owner.valid).getType()) ||
              !ac::areEquivalentHardwareTypes(bound(owner.valid).getType(), bit))
            return reject("original prospective queue external capture type differs");
          prospective.append("valid", b.getI64IntegerAttr(
              std::distance(preparation.liveIns.begin(), valid)));
          prospective.append("external_pop", b.getI64IntegerAttr(external.capture));
          SmallVector<Attribute> priorRows;
          SmallVector<Value> credit;
          for (uint64_t ordinal : priorPops) {
            if (ordinal >= preparation.queueProposals.size()) return failure();
            const auto &prior = preparation.queueProposals[ordinal];
            if (!prepayAppend(budget, access.operation, credit)) return failure();
            credit.push_back(record.queuePaths[prior.access]);
            priorRows.push_back(b.getI64IntegerAttr(ordinal));
          }
          if (!prepayAppend(budget, access.operation, credit)) return failure();
          credit.push_back(rule.getBody().front().getArgument(external.capture));
          prospective.append("prior_pops", b.getArrayAttr(priorRows));
          auto capacityOr = selected.getTrueValue().getDefiningOp<ac::BitsBinaryOp>();
          auto validAnd = capacityOr ? capacityOr.getRhs().getDefiningOp<ac::BitsBinaryOp>()
                                     : ac::BitsBinaryOp();
          if (!capacityOr || !validAnd || capacityOr.getLhs() != bare ||
              validAnd.getLhs() != bound(owner.valid))
            return reject("original prospective queue capacity/valid differs");
          Value joined = validAnd.getRhs();
          for (uint64_t index = credit.size(); index > 1; --index) {
            auto node = joined.getDefiningOp<ac::BitsBinaryOp>();
            if (!node || node.getRhs() != credit[index - 1] ||
                failed(prospectiveBinaryProof(joined, node.getLhs(), credit[index - 1], "or",
                                   access.operation->getLoc())))
              return reject("original prospective queue raw-pop union differs");
            joined = node.getLhs();
          }
          if (joined != credit.front() ||
              failed(prospectiveBinaryProof(validAnd.getResult(), bound(owner.valid), validAnd.getRhs(),
                                 "and", access.operation->getLoc())) ||
              failed(prospectiveBinaryProof(capacityOr.getResult(), bare, validAnd.getResult(),
                                 "or", access.operation->getLoc())))
            return reject("original prospective queue readiness differs");
          bare = capacityOr.getResult();
        }
      }
      if (entry.priorPops != priorPops || entry.externalPop != externalPop ||
          selected.getTrueValue() != bare ||
          failed(literalProof(selected.getFalseValue(), bit, transaction.getOccurrence(),
              access.operation->getLoc(), true, &rule.getBody().front())) ||
          failed(generatedProof(selected, ac::BitsSelectOp::getOperationName(),
              {record.queuePaths[accessId], bare, selected.getFalseValue()}, {bit},
              b.getDictionaryAttr({}), access.operation->getLoc(), &rule.getBody().front())))
        return reject("original queue prospective resource differs");
      NamedAttrList resourceRow;
      resourceRow.append("control", b.getI64IntegerAttr(entry.control));
      resourceRow.append("kind", kind);
      resourceRow.append("owner", identity.get("owner"));
      resourceRow.append("declaration", owner.occurrence);
      resourceRow.append("occurrence", access.occurrence);
      resourceRow.append("location", access.location);
      if (access.payload) resourceRow.append("prospective", prospective.getDictionary(b.getContext()));
      resourceRows[resourceIndex] = resourceRow.getDictionary(b.getContext());
      ++resourceIndex;
      if (access.kind == ac::TransactionQueueAccessKind::Peek) {
        if (!resource) return reject("original queue peek lacks resource admission");
        continue;
      }
      if (proposalIndex >= preparation.queueProposals.size())
        return reject("original queue proposal inventory is incomplete");
      const auto &proposal = preparation.queueProposals[proposalIndex++];
      if (proposal.access != accessId || proposal.resource != resource ||
          bool(proposal.dataResult) != bool(access.payload) ||
          yield->getOperand(proposal.rawResult) != record.queuePaths[accessId] ||
          (proposal.dataResult && yield->getOperand(*proposal.dataResult) != bound(*access.payload)))
        return reject("original queue proposal access/data differs");
      // The sharing key is the exact independently replayed exclusion set,
      // never a proposal label, cached prefix or candidate certificate.
      auto exclusions = sourceExclusions(access.point.path, preparation.controls.size(), access.operation);
      if (failed(exclusions) || *exclusions != proposal.excludedControls)
        return reject("original queue retained-control sharing identity differs");
      NamedAttrList row;
      row.append("queue", identity); row.append("kind", b.getStringAttr(access.payload ? "push" : "pop"));
      row.append("occurrence", access.occurrence); row.append("location", access.location);
      row.append("raw_path", b.getI64IntegerAttr(proposal.rawResult)); row.append("path", b.getI64IntegerAttr(proposal.pathResult));
      if (resource) row.append("resource_control", b.getI64IntegerAttr(preparation.resources[*resource].control));
      auto unreachable = originalQueueUnreachable(plan, access);
      if (failed(unreachable) || proposal.unreachable != *unreachable)
        return reject("original queue unreachable proposal differs");
      if (!resource) return reject("original queue proposal lacks admission");
      if (proposal.dataResult) row.append("data", b.getI64IntegerAttr(*proposal.dataResult));
      if (!proposal.excludedControls.empty()) {
        SmallVector<Attribute> excluded;
        for (auto control : proposal.excludedControls) excluded.push_back(b.getI64IntegerAttr(control));
        row.append("excluded_controls", b.getArrayAttr(excluded));
      }
      proposalRows.push_back(row.getDictionary(b.getContext()));
    }
#include "TransactionMemorySourceProof.inc"
    if (llvm::any_of(resourceRows, [](Attribute row) { return !row; }) ||
        proposalIndex != preparation.queueProposals.size() ||
        (resourceRows.empty() ? bool(preparation.expectedAdmission.get("resources"))
          : preparation.expectedAdmission.get("resources") != b.getArrayAttr(resourceRows)) ||
        (proposalRows.empty() ? bool(preparation.expectedAdmission.get("queue_proposals"))
          : preparation.expectedAdmission.get("queue_proposals") != b.getArrayAttr(proposalRows)))
      return reject("original queue resource/proposal metadata differs");
