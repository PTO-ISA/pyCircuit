// Included beside OrderedPrefixProof so queue proposals reuse the admission
// grammar and its four-state knownness proof.
static LogicalResult proveQueueProposalPrefixes(
    const SampledAdmissionView &view, const PreparedSampledBoundaries &prepared,
    const HardwareAnalysis &analysis, SourceCheckBudget &budget, OrderedPrefixProof &proof) {
  if (view.queueProposals.empty() && view.memoryProposals.empty()) return success();
  struct Factor { HardwareIdentityEndpoint value; bool positive; bool expandedSelector = false; bool literal = false; };
  struct Factors { SmallVector<Factor> atoms; bool alwaysFalse = false; };
  auto factors = [&](Value value) -> FailureOr<Factors> {
    Factors result;
    SmallVector<std::pair<Value, bool>> pending{{value, true}};
    while (!pending.empty()) {
      if (failed(budget.debit(1, view.rule))) return failure();
      auto [value, positive] = pending.pop_back_val();
      auto endpoint = proof.normalize(value);
      if (failed(endpoint)) return failure();
      auto known = literal(*endpoint, analysis, budget);
      if (failed(known)) return failure();
      if (known->has_value()) {
        result.alwaysFalse |= (**known != positive);
        // Preserve the actual selector SSA and its polarity even when known.
        // An inactive literal arm still witnesses its opposite source arm;
        // the false-path flag alone never authorizes an omission.
        result.atoms.push_back({*endpoint, positive, false, true});
        continue;
      }
      if (auto conjunction = endpoint->value.getDefiningOp<BitsBinaryOp>();
          positive && conjunction && conjunction.getOpcode() == "and") {
        result.atoms.push_back({*endpoint, true, true});
        pending.push_back({conjunction.getLhs(), true});
        pending.push_back({conjunction.getRhs(), true});
      } else if (auto invert = endpoint->value.getDefiningOp<BitsUnaryOp>();
                 invert && invert.getOpcode() == "not") {
        result.atoms.push_back({*endpoint, positive, true});
        pending.push_back({invert.getInput(), !positive});
      } else result.atoms.push_back({*endpoint, positive});
    }
    return result;
  };
  auto normalizedControl = [&](uint64_t index) {
    return proof.normalize(view.controls[index].raw);
  };
  Prefix root;
  SmallVector<ExpectedSelectionPaths, 0> selections;
  SmallVector<Prefix, 0> acceptances;
  uint64_t selectionCount = llvm::count_if(view.controls, [](const auto &control) {
    return control.kind == SampledControlKind::BranchSelect;
  });
  if (failed(charge(budget, view.rule, selectionCount, sizeof(ExpectedSelectionPaths))) ||
      failed(charge(budget, view.rule, view.controls.size(), sizeof(Prefix)))) return failure();
  selections.reserve(selectionCount);
  acceptances.reserve(view.controls.size());
  auto selectedPath = [&](SampledPathRef path) -> FailureOr<const Prefix *> {
    if (!path.selectionControl) return &root;
    for (const auto &selection : selections) {
      if (failed(budget.debit(1, view.rule))) return failure();
      if (selection.control == *path.selectionControl)
        return path.whenTrue ? &selection.whenTrue : &selection.whenFalse;
    }
    return view.rule->emitOpError() << "queue prefix references an unavailable selection";
  };
  for (auto [index, control] : llvm::enumerate(view.controls)) {
    auto path = selectedPath(control.executionPath);
    auto raw = normalizedControl(index);
    if (failed(path) || failed(raw)) return failure();
    auto accepted = proof.knownValue(*raw,
        control.kind == SampledControlKind::BranchSelect ? KnownTest::Either : KnownTest::One,
        view.rule);
    if (failed(accepted)) return failure();
    if (control.kind == SampledControlKind::BranchSelect) {
      auto zero = proof.knownValue(*raw, KnownTest::Zero, view.rule);
      auto one = proof.knownValue(*raw, KnownTest::One, view.rule);
      if (failed(zero) || failed(one)) return failure();
      auto whenTrue = proof.conjunction(**path, *one, view.rule);
      auto whenFalse = proof.conjunction(**path, *zero, view.rule);
      if (failed(whenTrue) || failed(whenFalse)) return failure();
      selections.push_back({index, std::move(*whenTrue), std::move(*whenFalse)});
    }
    if (control.executionPath.selectionControl) {
      auto bypass = proof.conditional(**path, *accepted, view.rule);
      if (failed(bypass)) return failure();
      accepted = std::move(*bypass);
    }
    acceptances.push_back(std::move(*accepted));
  }
  for (const auto &proposal : view.queueProposals) {
    auto pathFactors = factors(proposal.rawPath);
    if (failed(pathFactors)) return failure();
    // Each omitted control needs a concrete opposite-arm witness. Unknown
    // selectors are still checked by a retained common branch_select control.
    for (uint64_t omitted : proposal.excludedControls) {
      SampledPathRef ancestry = view.controls[omitted].executionPath;
      bool disjoint = false;
      while (ancestry.selectionControl) {
        uint64_t branch = *ancestry.selectionControl;
        if (failed(budget.debit(1, view.rule))) return failure();
        if (!llvm::is_contained(proposal.excludedControls, branch)) {
          auto selector = normalizedControl(branch);
          if (failed(selector)) return failure();
          auto known = literal(*selector, analysis, budget);
          if (failed(known)) return failure();
          // A retained selector with a verified literal value independently
          // makes its opposite source arm unreachable. Unknown selectors still
          // require the concrete opposite-polarity SSA witness below.
          if (known->has_value() && **known != ancestry.whenTrue) disjoint = true;
          for (const auto &factor : pathFactors->atoms)
            if (sameEndpoint(factor.value, *selector) &&
                factor.positive != ancestry.whenTrue) disjoint = true;
        }
        ancestry = view.controls[branch].executionPath;
      }
      if (!disjoint)
        return view.rule->emitOpError()
               << "queue proposal excludes a control without an opposite-arm SSA witness";
    }
    Prefix expected;
    for (auto [index, control] : llvm::enumerate(view.controls)) {
      if (failed(budget.debit(1, view.rule))) return failure();
      if (llvm::is_contained(proposal.excludedControls, index)) continue;
      // The cached path is prefix-free. Reusing it is valid only when each
      // retained control keeps its immediate selection; this condition is
      // checked for every retained parent as well, covering the full ancestry.
      if (control.executionPath.selectionControl &&
          llvm::is_contained(proposal.excludedControls, *control.executionPath.selectionControl))
        return view.rule->emitOpError() << "queue prefix references an omitted selection";
      auto next = proof.conjunction(expected, acceptances[index], view.rule);
      if (failed(next)) return failure();
      expected = std::move(*next);
    }
    auto enable = proof.normalize(proposal.path);
    auto rawPath = proof.normalize(proposal.rawPath);
    if (failed(enable) || failed(rawPath)) return failure();
    auto combine = enable->value.getDefiningOp<BitsBinaryOp>();
    if (!combine || combine.getOpcode() != "and")
      return view.rule->emitOpError() << "queue proposal requires admission AND raw path";
    auto selected = proof.normalize(combine.getRhs());
    auto prefix = proof.parse(combine.getLhs());
    if (failed(selected) || failed(prefix) || !sameEndpoint(*selected, *rawPath) ||
        failed(proof.equal(*prefix, expected, view.rule)))
      return view.rule->emitOpError() << "queue proposal differs from its retained admission and raw path";
    if (proposal.resourceControl) {
      auto raw = normalizedControl(*proposal.resourceControl);
      auto select = succeeded(raw) ? raw->value.getDefiningOp<BitsSelectOp>() : BitsSelectOp();
      auto predicate = select ? proof.normalize(select.getCondition())
                              : FailureOr<HardwareIdentityEndpoint>(failure());
      if (failed(predicate) || !sameEndpoint(*predicate, *rawPath) ||
          llvm::is_contained(proposal.excludedControls, *proposal.resourceControl))
        return view.rule->emitOpError() << "queue proposal must retain its exact path-selected resource";
    }

  }
#include "HardwareMemoryProposalProof.inc"
  return success();
}
