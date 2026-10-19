    for (const auto &proposal : preparation.memoryProposals) {
      const auto &access = plan.memoryAccesses[proposal.access];
      auto request = yield->getOperand(proposal.pathResult).getDefiningOp<ac::BitsBinaryOp>();
      if (!request || request.getOpcode() != "and" ||
          request.getRhs() != record.memoryPaths[proposal.access] ||
          failed(binaryProof(request.getResult(), request.getLhs(), request.getRhs(),
                              "and", access.operation->getLoc())))
        return reject("original Memory demand raw path differs");
      if (access.kind == ac::TransactionMemoryAccessKind::Read) {
        if (failed(retainedPrefixProof(request.getLhs(), proposal.excludedControls,
                                        proposal.reachedControlEnd, access.operation)))
          return failure();
      } else if (request.getLhs() != prefix)
        return reject("original Memory issue lacks final atomic admission");
    }
