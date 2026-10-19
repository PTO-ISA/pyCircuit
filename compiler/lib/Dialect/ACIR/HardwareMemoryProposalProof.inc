  // Each argument has an actual required-use witness. Async addresses stop at
  // the reached prefix immediately after those witnesses, before loaded data
  // can affect admission. Requests and writes retain the complete final fire.
  llvm::DenseSet<uint64_t> memoryWitnesses;
  for (const auto &proposal : view.memoryProposals) {
    auto actual = proof.normalize(proposal.path), raw = proof.normalize(proposal.rawPath);
    if (failed(actual) || failed(raw)) return failure();
    auto combine = actual->value.getDefiningOp<BitsBinaryOp>();
    if (!combine || combine.getOpcode() != "and")
      return view.rule->emitOpError() << "Memory proposal requires admission AND raw path";
    auto selected = proof.normalize(combine.getRhs());
    auto prefix = proof.parse(combine.getLhs());
    if (failed(selected) || failed(prefix) || !sameEndpoint(*selected, *raw))
      return view.rule->emitOpError() << "Memory proposal raw path differs from its actual enable";
    auto rawFactors = factors(proposal.rawPath);
    if (failed(rawFactors)) return failure();
    bool witnessed = false;
    for (size_t first = 0; first < view.requirements.size(); ++first) {
      if (failed(budget.debit(1, view.rule))) return failure();
      if (first + proposal.arguments.size() > view.requirements.size()) break;
      const auto &start = view.requirements[first];
      if (start.kind.getValue() != "memory_argument" || start.operand != 0 ||
          start.occurrence != proposal.occurrence || start.location != proposal.location ||
          memoryWitnesses.contains(first)) continue;
      bool matches = true;
      uint64_t lastControl = start.control;
      for (auto [ordinal, argument] : llvm::enumerate(proposal.arguments)) {
        const auto &requirement = view.requirements[first + ordinal];
        if (requirement.kind.getValue() != "memory_argument" || requirement.operand != ordinal ||
            requirement.occurrence != proposal.occurrence || requirement.location != proposal.location ||
            requirement.control != start.control + ordinal || memoryWitnesses.contains(first + ordinal)) {
          matches = false; break;
        }
        auto payload = proof.normalize(requirement.operation->getOperand(0)), expectedPayload = proof.normalize(argument);
        auto requiredPath = factors(requirement.operation->getOperand(2));
        if (failed(payload) || failed(expectedPayload) || failed(requiredPath)) return failure();
        if (requiredPath->alwaysFalse != rawFactors->alwaysFalse || !sameEndpoint(*payload, *expectedPayload)) { matches = false; break; }
        auto contains = [&](const Factors &haystack, const Factor &needle) {
          return llvm::any_of(haystack.atoms, [&](const Factor &atom) {
            return !atom.expandedSelector && !atom.literal && atom.positive == needle.positive &&
                   sameEndpoint(atom.value, needle.value);
          });
        };
        for (const auto &factor : requiredPath->atoms)
          if (!factor.expandedSelector && !factor.literal && !contains(*rawFactors, factor)) matches = false;
        for (const auto &factor : rawFactors->atoms)
          if (!factor.expandedSelector && !factor.literal && !contains(*requiredPath, factor)) matches = false;
        if (!matches) break;
        lastControl = requirement.control;
      }
      if (!matches) continue;
      Prefix expected, full;
      bool read = proposal.kind.getValue() == "read", anyControlFalse = false;
      uint64_t count = read ? lastControl + 1 : view.controls.size();
      SmallVector<uint8_t, 0> omittable;
      if (read) {
        if (failed(charge(budget, view.rule, count, sizeof(uint8_t)))) return failure();
        omittable.resize(count, false);
      }
      for (uint64_t index = 0; index < count; ++index) {
        if (read) {
          // Keep the common selector's knownness check. Only controls inside
          // an independently opposite source arm can leave the address-demand
          // prefix; their original reached diagnostic paths stay untouched.
          auto ancestry = view.controls[index].executionPath;
          while (ancestry.selectionControl) {
            uint64_t branch = *ancestry.selectionControl;
            if (failed(budget.debit(1, view.rule))) return failure();
            if (!omittable[branch]) {
              auto selector = normalizedControl(branch);
              if (failed(selector)) return failure();
              for (const auto &factor : rawFactors->atoms) {
                if (failed(budget.debit(1, view.rule))) return failure();
                if (sameEndpoint(factor.value, *selector) && factor.positive != ancestry.whenTrue)
                  omittable[index] = true;
              }
            }
            ancestry = view.controls[branch].executionPath;
          }
          anyControlFalse |= acceptances[index].impossible;
          if (!acceptances[index].impossible) {
            auto all = proof.conjunction(full, acceptances[index], view.rule);
            if (failed(all)) return failure();
            full = std::move(*all);
          }
          if (omittable[index]) continue;
        }
        auto next = proof.conjunction(expected, acceptances[index], view.rule);
        if (failed(next)) return failure();
        expected = std::move(*next);
      }
      if (failed(budget.debit(1 + prefix->atoms.size() + expected.atoms.size() + full.atoms.size(), view.rule))) return failure();
      if (!read && (prefix->impossible != expected.impossible || prefix->atoms != expected.atoms)) continue;
      // Prefix atoms are canonical sorted conjunctions. The exact original
      // prefix remains legal; removing any control requires the witness above.
      // Acceptance factories produce one atom (or a closed constant) per
      // control, so these bounds cannot authorize partial control omission.
      if (read && ((expected.impossible && !prefix->impossible) ||
          (prefix->impossible && !anyControlFalse) ||
          !std::includes(prefix->atoms.begin(), prefix->atoms.end(), expected.atoms.begin(), expected.atoms.end()) ||
          !std::includes(full.atoms.begin(), full.atoms.end(), prefix->atoms.begin(), prefix->atoms.end()))) continue;
      for (uint64_t ordinal = 0; ordinal < proposal.arguments.size(); ++ordinal) {
        if (failed(charge(budget, view.rule, 1, sizeof(uint64_t) * 3))) return failure();
        memoryWitnesses.insert(first + ordinal);
      }
      witnessed = true; break;
    }
    if (!witnessed)
      return view.rule->emitOpError() << "Memory proposal differs from exact argument witnesses or reached/atomic admission";
  }
  for (auto [ordinal, requirement] : llvm::enumerate(view.requirements)) {
    if (failed(budget.debit(1, view.rule))) return failure();
    if (requirement.kind.getValue() == "memory_argument" && !memoryWitnesses.contains(ordinal))
      return view.rule->emitOpError() << "Memory argument witness lacks its exact issue proposal";
  }
