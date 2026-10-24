    auto sameAddress = [&](const Proposal &left, const Proposal &right) -> FailureOr<bool> {
      auto a = grantValue(left.proposal->arguments.front(), left.scope),
           b = grantValue(right.proposal->arguments.front(), right.scope);
      if (failed(a) || failed(b)) return failure();
      if (*a == *b) return true;
      auto lhs = grantInspect(*a), rhs = grantInspect(*b);
      if (failed(lhs) || failed(rhs)) return failure();
      return lhs->fullyKnown && rhs->fullyKnown && lhs->width == rhs->width &&
          llvm::APSInt::compareValues(llvm::APSInt(lhs->fullyKnown->value.getCanonicalValue()),
                                      llvm::APSInt(rhs->fullyKnown->value.getCanonicalValue())) == 0;
    };
    auto sameDemandAtom = [&](const HardwareIdentityEndpoint &left,
                              const HardwareIdentityEndpoint &right) -> FailureOr<bool> {
      if (same(left, right)) return true;
      if (failed(step(module, 4))) return failure();
      auto a = left.value.getDefiningOp<BitsUnaryOp>(),
           b = right.value.getDefiningOp<BitsUnaryOp>();
      if (!a || !b || a.getOpcode() != "not" || b.getOpcode() != "not")
        return false;
      auto lhs = grantBind(left), rhs = grantBind(right);
      if (failed(lhs) || failed(rhs)) return failure();
      // grantInspect verifies the bound unary operation and interns its
      // normalized operands using exact value, scope, binding and type identity.
      auto first = grantInspect(*lhs), second = grantInspect(*rhs);
      if (failed(first) || failed(second)) return failure();
      if (first->kind != GrantCore::NodeView::Not ||
          second->kind != GrantCore::NodeView::Not ||
          first->width != 1 || second->width != 1 ||
          first->operands.size() != 1 || second->operands.size() != 1)
        return false;
      // Retain both NOTs. No recursion or double-NOT/Boolean rewriting can
      // equate a raw Z selector with its complemented four-state result.
      return first->operands.front() == second->operands.front();
    };
    // Four-state absorption: AND-only factors preserve exact X/Z behavior.
    // Boolean implication or NOT expansion is insufficient for pin elision.
    struct DemandFactors {
      bool zero = false;
      SmallVector<HardwareIdentityEndpoint, 0> atoms;
    };
    if (failed(pay(SourceCheckBudget::multiply(proposals.size(),
        sizeof(std::optional<DemandFactors>) + sizeof(uint8_t))))) return failure();
    SmallVector<std::optional<DemandFactors>, 0> demands(proposals.size());
    SmallVector<uint8_t, 0> coveredReads(proposals.size(), false);
    auto demandFactors = [&](size_t ordinal) -> FailureOr<const DemandFactors *> {
      if (demands[ordinal]) return &*demands[ordinal];
      DemandFactors result;
      SmallVector<HardwareIdentityEndpoint, 0> pending, seen;
      auto first = normal(proposals[ordinal].proposal->path, proposals[ordinal].scope);
      if (failed(first) || failed(pay(sizeof(HardwareIdentityEndpoint)))) return failure();
      pending.push_back(*first);
      while (!pending.empty()) {
        auto actual = pending.pop_back_val();
        bool visited = false;
        for (const auto &prior : seen) {
          if (failed(step(module))) return failure();
          visited |= same(prior, actual);
        }
        if (visited) continue;
        if (failed(pay(sizeof(HardwareIdentityEndpoint)))) return failure();
        seen.push_back(actual);
        auto known = literal(actual);
        if (failed(known)) return failure();
        if (*known) { result.zero |= !**known; continue; }
        if (auto conjunction = actual.value.getDefiningOp<BitsBinaryOp>();
            conjunction && conjunction.getOpcode() == "and") {
          for (Value operand : conjunction->getOperands()) {
            auto child = normal(operand, actual.scope);
            if (failed(child) || failed(pay(sizeof(HardwareIdentityEndpoint)))) return failure();
            pending.push_back(*child);
          }
        } else {
          if (failed(pay(sizeof(HardwareIdentityEndpoint)))) return failure();
          result.atoms.push_back(actual);
        }
      }
      demands[ordinal] = std::move(result);
      return &*demands[ordinal];
    };
    for (size_t later = 0; later < proposals.size(); ++later) {
      const auto &b = proposals[later];
      if (b.proposal->kind.getValue() != "read") continue;
      for (size_t earlier = 0; earlier < later; ++earlier) {
        if (failed(step(module))) return failure();
        const auto &a = proposals[earlier];
        if (coveredReads[earlier] || a.proposal->kind.getValue() != "read" ||
            a.view->rule != b.view->rule || a.scope.root != b.scope.root || a.scope.path != b.scope.path ||
            !same(a.identity, b.identity) || a.proposal->port != b.proposal->port) continue;
        auto equalAddress = sameAddress(a, b);
        if (failed(equalAddress)) return failure();
        if (!*equalAddress) continue;
        auto prior = demandFactors(earlier), current = demandFactors(later);
        if (failed(prior) || failed(current)) return failure();
        bool covered = (*current)->zero;
        if (!covered && !(*prior)->zero) {
          covered = true;
          for (const auto &factor : (*prior)->atoms) {
            bool present = false;
            for (const auto &candidate : (*current)->atoms) {
              if (failed(step(module))) return failure();
              auto equal = sameDemandAtom(factor, candidate);
              if (failed(equal)) return failure();
              present |= *equal;
            }
            covered &= present;
          }
        }
        if (covered) { coveredReads[later] = true; break; }
      }
    }
