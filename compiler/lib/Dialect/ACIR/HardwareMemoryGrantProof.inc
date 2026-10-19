    // Adapt actual bound endpoints to the existing four-state owner-enable
    // implication algebra. This adds no separate predicate solver.
    using GrantCore = HardwareGrantProof;
    using GrantNode = GrantCore::Node;
    SmallVector<HardwareIdentityEndpoint, 0> grantNodes;
    auto grantBind = [&](const HardwareIdentityEndpoint &endpoint) -> FailureOr<GrantNode> {
      for (auto [ordinal, prior] : llvm::enumerate(grantNodes)) {
        if (failed(step(module))) return failure();
        if (same(prior, endpoint)) return ordinal;
      }
      if (failed(pay(sizeof(HardwareIdentityEndpoint) + endpoint.scope.path.size() * sizeof(InstanceOp)))) return failure();
      GrantNode id = grantNodes.size(); grantNodes.push_back(endpoint); return id;
    };
    auto grantInspect = [&](GrantNode id) -> FailureOr<GrantCore::NodeView> {
      if (id >= grantNodes.size() || failed(step(module))) return failure();
      // Operand interning may reallocate grantNodes.
      auto endpoint = grantNodes[id];
      auto bound = scopeBindings(endpoint.scope);
      if (failed(bound)) return failure();
      GrantCore::NodeView view;
      if (!isa<BitsType>(endpoint.resolvedType)) return view;
      auto width = analysis.getPackedWidth(endpoint.resolvedType, *bound, module);
      if (failed(width)) return failure();
      view.width = *width;
      auto *operation = endpoint.value.getDefiningOp();
      if (auto constant = dyn_cast_or_null<BitsConstantOp>(operation)) {
        auto value = analysis.evaluateStatic(constant.getValue(), *bound, constant);
        if (failed(value)) return failure();
        if (auto integer = dyn_cast<MathIntAttr>(*value)) {
          if (failed(step(module, SourceCheckBudget::add(integer.getCanonicalValue().size(), *width / 64 + 1)))) return failure();
          llvm::APSInt number(integer.getCanonicalValue());
          if (!number.isNegative() && number.getActiveBits() <= *width)
            view.fullyKnown = GrantCore::Constant{integer, *width};
        }
        return view;
      }
      if (auto unary = dyn_cast_or_null<BitsUnaryOp>(operation); unary && unary.getOpcode() == "not")
        view.kind = GrantCore::NodeView::Not;
      else if (auto binary = dyn_cast_or_null<BitsBinaryOp>(operation)) {
        if (binary.getOpcode() == "and") view.kind = GrantCore::NodeView::And;
        if (binary.getOpcode() == "or") view.kind = GrantCore::NodeView::Or;
      } else if (isa_and_nonnull<BitsSelectOp>(operation)) view.kind = GrantCore::NodeView::Select;
      else if (auto compare = dyn_cast_or_null<BitsCompareOp>(operation)) {
        if (compare.getPredicate() == "eq") view.kind = GrantCore::NodeView::Eq;
        if (compare.getPredicate() == "ne") view.kind = GrantCore::NodeView::Ne;
      }
      // case_eq remains opaque, matching the existing owner-enable adapter.
      if (view.kind != GrantCore::NodeView::Raw) {
        if (failed(analysis.verifyResolvedOperation(operation, *bound))) return failure();
        for (Value value : operation->getOperands()) {
          auto child = normal(value, endpoint.scope);
          if (failed(child)) return failure();
          auto node = grantBind(*child);
          if (failed(node)) return failure();
          view.operands.push_back(*node);
        }
      }
      return view;
    };
    GrantCore grantProof([&](GrantNode id, unsigned) -> FailureOr<GrantNode> {
      if (id >= grantNodes.size() || failed(step(module))) return failure();
      return id;
    }, grantInspect, [&](uint64_t work) { return succeeded(step(module, work)); });
    auto grantValue = [&](Value value, const HardwareIdentityScope &at) -> FailureOr<GrantNode> {
      auto endpoint = normal(value, at);
      if (failed(endpoint)) return failure();
      return grantBind(*endpoint);
    };
