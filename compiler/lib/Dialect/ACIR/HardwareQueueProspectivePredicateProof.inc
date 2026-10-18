  LogicalResult prospectivePredicate(const SampledAdmissionView &view,
      const SampledResourceView &resource, const QueueEndpoint &target, Value bare) {
    auto queue = target.queue;
    if (!resource.prospective)
      return module->emitOpError() << "queue_ready requires prospective capacity metadata";
    const auto &prospective = *resource.prospective;
    auto captureEndpoint = [&](uint64_t capture) -> FailureOr<HardwareIdentityEndpoint> {
      if (capture >= view.rule->getNumOperands()) return failure();
      return normal(view.rule->getOperand(capture), scope);
    };
    auto capacity = captureEndpoint(prospective.capacityCapture);
    auto actualCapacity = normal(queue.getOutputs()[3], target.scope);
    if (failed(capacity) || failed(actualCapacity) || !same(*capacity, *actualCapacity))
      return module->emitOpError() << "prospective Queue capacity must be the actual committed local_capacity result";
    auto config = analysis.resolveQueue(queue, target.bindings);
    if (failed(config)) return failure();
    bool downstream =
        config->readyPolicy == QueueReadyPolicy::DownstreamPop;
    if (!downstream) {
      if (config->readyPolicy != QueueReadyPolicy::LocalOccupancy ||
          prospective.validCapture ||
          prospective.externalPopCapture || !prospective.priorPops.empty())
        return module->emitOpError() << "local_occupancy cannot receive prospective pop credit";
      auto actual = normal(bare, scope);
      if (failed(actual) || !same(*actual, *capacity))
        return module->emitOpError() << "local_occupancy readiness must equal committed capacity";
      return success();
    }
    if (!prospective.validCapture || !prospective.externalPopCapture ||
        prospective.capacityCapture == *prospective.validCapture ||
        prospective.capacityCapture == *prospective.externalPopCapture ||
        *prospective.validCapture == *prospective.externalPopCapture)
      return module->emitOpError() << "downstream_pop requires dedicated capacity, valid and external-pop captures";
    auto valid = captureEndpoint(*prospective.validCapture);
    auto actualValid = normal(queue.getOutputs()[1], target.scope);
    if (failed(valid) || failed(actualValid) || !same(*valid, *actualValid))
      return module->emitOpError() << "prospective Queue valid must be the actual committed out_valid result";
    SmallVector<uint64_t> prior;
    Builder b(module.getContext());
    auto id = b.getDictionaryAttr({b.getNamedAttr("owner", resource.owner),
                                  b.getNamedAttr("declaration", resource.declaration)});
    for (auto [i, proposal] : llvm::enumerate(view.queueProposals)) {
      if (failed(step(view.rule))) return failure();
      if (proposal.kind.getValue() != "pop" || proposal.queue != id ||
          !proposal.resourceControl || *proposal.resourceControl >= resource.control) continue;
      auto path = factors(proposal.rawPath, scope);
      if (failed(path)) return failure();
      if (!path->falsePath) {
        if (failed(pay(sizeof(uint64_t)))) return failure();
        prior.push_back(i);
      }
    }
    if (prior != prospective.priorPops)
      return module->emitOpError() << "prospective prior_pops differs from every earlier reachable same-owner pop";
    auto normalized = normal(bare, scope);
    auto ready = succeeded(normalized) ? normalized->value.getDefiningOp<BitsBinaryOp>() : BitsBinaryOp();
    if (!ready || ready.getOpcode() != "or")
      return module->emitOpError() << "downstream_pop readiness requires C OR (V AND raw pop credit)";
    auto lhs = normal(ready.getLhs(), normalized->scope);
    auto rhs = normal(ready.getRhs(), normalized->scope);
    auto pop = succeeded(rhs) ? rhs->value.getDefiningOp<BitsBinaryOp>() : BitsBinaryOp();
    if (failed(lhs) || !same(*lhs, *capacity) || !pop || pop.getOpcode() != "and") return failure();
    auto nonempty = normal(pop.getLhs(), rhs->scope);
    if (failed(nonempty) || !same(*nonempty, *valid))
      return module->emitOpError() << "prospective pop credit requires actual committed validity";
    SmallVector<Value> credit;
    for (uint64_t i : prior) credit.push_back(view.queueProposals[i].rawPath);
    credit.push_back(view.rule->getOperand(*prospective.externalPopCapture));
    if (failed(unionOf(pop.getRhs(), credit))) return failure();
    // Record only the exact prospective expression's edges. Private capture
    // closure below includes the H graph only at an authenticated bare leaf.
    if (failed(prospectiveEdge(ready, 0)) || failed(prospectiveEdge(ready, 1)) ||
        failed(prospectiveEdge(pop, 0)) || failed(prospectiveEdge(pop, 1))) return failure();
    auto cursor = normal(pop.getRhs(), scope);
    if (failed(cursor)) return failure();
    for (size_t i = credit.size(); i > 1; --i) {
      auto op = cursor->value.getDefiningOp<BitsBinaryOp>();
      if (!op || failed(prospectiveEdge(op, 0)) || failed(prospectiveEdge(op, 1))) return failure();
      cursor = normal(op.getLhs(), cursor->scope);
      if (failed(cursor)) return failure();
    }
    auto argument = view.rule->getRegion(0).front().getArgument(*prospective.externalPopCapture);
    if (failed(pay(sizeof(Value) * 3))) return failure();
    prospectivePrivate.insert(argument);
    prospectivePrivate.insert(pop.getResult());
    prospectivePrivate.insert(ready.getResult());
    prospectiveLeaves.insert(ready.getResult());
    return success();
  }
  LogicalResult prospectiveEdge(Operation *operation, unsigned operand) {
    if (operand >= operation->getNumOperands() || failed(pay(sizeof(OpOperand *) * 3))) return failure();
    auto *edge = &operation->getOpOperand(operand);
    prospectiveAllowed.insert(edge);
    verifiedUses.insert(edge);
    // Only OR-credit nodes and the valid/capacity formula belong to this graph;
    // raw authored path operands remain public, independently checked inputs.
    if (auto binary = operation->getOperand(operand).getDefiningOp<BitsBinaryOp>();
        binary && binary.getOpcode() == "or") prospectivePrivate.insert(binary.getResult());
    return success();
  }
  LogicalResult prospectiveUseClosure(const SampledAdmissionView &view) {
    llvm::DenseSet<Operation *> diagnostic;
    SmallVector<Value> pending;
    for (const auto &control : view.controls) {
      auto reached = normal(control.reached, scope);
      auto wrapper = succeeded(reached) ? reached->value.getDefiningOp<BitsBinaryOp>() : BitsBinaryOp();
      if (!wrapper || wrapper.getOpcode() != "and") return failure();
      auto gate = normal(wrapper.getRhs(), scope);
      auto invert = succeeded(gate) ? gate->value.getDefiningOp<BitsUnaryOp>() : BitsUnaryOp();
      auto test = invert ? normal(invert.getInput(), scope) : FailureOr<HardwareIdentityEndpoint>(failure());
      auto compare = succeeded(test) ? test->value.getDefiningOp<BitsCompareOp>() : BitsCompareOp();
      if (!invert || invert.getOpcode() != "not" || !compare || compare.getPredicate() != "case_eq") return failure();
      pending.push_back(compare.getLhs());
    }
    while (!pending.empty()) {
      auto endpoint = normal(pending.pop_back_val(), scope);
      if (failed(endpoint) || failed(step(view.rule))) return failure();
      Operation *op = endpoint->value.getDefiningOp();
      if (!op || op->getParentOp() != view.rule ||
          !isa<BitsBinaryOp, BitsSelectOp>(op) || diagnostic.contains(op)) continue;
      if (failed(pay(sizeof(Operation *) * 3))) return failure();
      diagnostic.insert(op);
      if (auto binary = dyn_cast<BitsBinaryOp>(op)) {
        if (binary.getOpcode() != "and") return failure();
        // H's LHS is a bare readiness input, not another diagnostic node.
        pending.push_back(binary.getRhs());
      } else {
        auto select = cast<BitsSelectOp>(op);
        pending.push_back(select.getTrueValue()); pending.push_back(select.getFalseValue());
      }
    }
    for (Value value : prospectivePrivate) {
      if (value.getParentRegion() != &view.rule->getRegion(0)) continue;
      for (OpOperand &use : value.getUses()) {
        if (failed(step(use.getOwner()))) return failure();
        if (prospectiveAllowed.contains(&use)) continue;
        bool rawResource = false;
        if (prospectiveLeaves.contains(value)) {
          for (const auto &resource : view.resources) {
            auto endpoint = normal(view.controls[resource.control].raw, scope);
            auto select = succeeded(endpoint) ? endpoint->value.getDefiningOp<BitsSelectOp>() : BitsSelectOp();
            rawResource |= select && use.getOwner() == select && use.getOperandNumber() == 1;
          }
        }
        bool hLeaf = prospectiveLeaves.contains(value) && diagnostic.contains(use.getOwner()) &&
                     isa<BitsBinaryOp>(use.getOwner()) && use.getOperandNumber() == 0;
        if (!rawResource && !hLeaf)
          return use.getOwner()->emitOpError() << "private prospective Queue control escapes its exact readiness or diagnostic graph";
        if (failed(pay(sizeof(OpOperand *) * 3))) return failure();
        verifiedUses.insert(&use);
      }
    }
    return success();
  }
