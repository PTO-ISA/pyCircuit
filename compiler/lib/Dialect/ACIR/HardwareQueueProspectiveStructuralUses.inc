  // This is a bounded locator reader for structural use collection. It never
  // calls parseSampledAdmission, prefix validation, or transaction proof. The
  // full closed parsers and original-source replay remain separate authorities.
  LogicalResult structuralResourceUses(RuleOp rule) {
    auto row = rule->getAttrOfType<DictionaryAttr>("ac.sampled_admission");
    auto resourceRows = row ? row.getAs<ArrayAttr>("resources") : ArrayAttr();
    if (!resourceRows) return success();
    auto controls = row.getAs<ArrayAttr>("controls");
    auto observations = row.getAs<ArrayAttr>("observations");
    auto fire = ordinal(row, "fire_result", rule);
    auto clock = ordinal(row, "clock_capture", rule), reset = ordinal(row, "reset_capture", rule);
    auto yield = dyn_cast<YieldOp>(rule.getBody().front().getTerminator());
    if (!controls || !observations || failed(fire) || failed(clock) || failed(reset) ||
        !yield || *clock >= rule.getNumOperands() || *reset >= rule.getNumOperands()) return failure();
    uint64_t base = SourceCheckBudget::add(*fire, 1);
    for (Attribute raw : observations) {
      auto count = ordinal(dyn_cast<DictionaryAttr>(raw), "value_count", rule);
      if (failed(count)) return failure();
      base = SourceCheckBudget::add(base, SourceCheckBudget::add(1, *count));
    }
    SampledAdmissionView view;
    view.rule = rule; view.definition = module;
    view.clock = rule.getOperand(*clock); view.reset = rule.getOperand(*reset);
    for (auto [i, raw] : llvm::enumerate(controls)) {
      if (failed(pay(sizeof(SampledControlView)))) return failure();
      auto entry = dyn_cast<DictionaryAttr>(raw);
      auto kind = entry ? entry.getAs<StringAttr>("kind") : StringAttr();
      auto path = entry ? entry.getAs<DictionaryAttr>("path") : DictionaryAttr();
      uint64_t output = SourceCheckBudget::add(base, SourceCheckBudget::multiply(2, i));
      if (!kind || !path || output >= yield.getNumOperands() || output + 1 >= yield.getNumOperands()) return failure();
      SampledControlView control;
      if (kind.getValue() == "callable_guard") control.kind = SampledControlKind::CallableGuard;
      else if (kind.getValue() == "branch_select") control.kind = SampledControlKind::BranchSelect;
      else if (kind.getValue() == "value_available") control.kind = SampledControlKind::ValueAvailable;
      else if (kind.getValue() == "resource_available") control.kind = SampledControlKind::ResourceAvailable;
      else return failure();
      if (!path.empty()) {
        auto selected = ordinal(path, "selection", rule);
        auto arm = path.getAs<BoolAttr>("when_true");
        if (failed(selected) || *selected >= i || !arm) return failure();
        control.executionPath = {*selected, arm.getValue()};
      }
      control.raw = yield.getOperand(output); control.reached = yield.getOperand(output + 1);
      view.controls.push_back(control);
    }
    if (auto proposals = row.getAs<ArrayAttr>("queue_proposals"))
      for (Attribute raw : proposals) {
        if (failed(pay(sizeof(SampledQueueProposalView)))) return failure();
        auto entry = dyn_cast<DictionaryAttr>(raw);
        auto path = ordinal(entry, "raw_path", rule), resource = ordinal(entry, "resource_control", rule);
        auto kind = entry ? entry.getAs<StringAttr>("kind") : StringAttr();
        auto queue = entry ? entry.getAs<DictionaryAttr>("queue") : DictionaryAttr();
        if (failed(path) || failed(resource) || *path >= yield.getNumOperands() ||
            *resource >= view.controls.size() || !kind || !queue) return failure();
        SampledQueueProposalView proposal;
        proposal.queue = queue; proposal.kind = kind;
        proposal.rawPath = yield.getOperand(*path); proposal.resourceControl = *resource;
        view.queueProposals.push_back(std::move(proposal));
      }
    for (Attribute raw : resourceRows) {
      if (failed(pay(sizeof(SampledResourceView)))) return failure();
      auto entry = dyn_cast<DictionaryAttr>(raw);
      auto control = ordinal(entry, "control", rule);
      auto kind = entry ? entry.getAs<StringAttr>("kind") : StringAttr();
      auto owner = entry ? entry.getAs<DictionaryAttr>("owner") : DictionaryAttr();
      auto declaration = entry ? entry.getAs<DictionaryAttr>("declaration") : DictionaryAttr();
      if (failed(control) || *control >= view.controls.size() || !kind || !owner || !declaration) return failure();
      SampledResourceView resource{*control, kind, owner, declaration,
          entry.getAs<DictionaryAttr>("occurrence"), entry.getAs<DictionaryAttr>("location"), std::nullopt};
      if (auto prospective = entry.getAs<DictionaryAttr>("prospective")) {
        auto capacity = ordinal(prospective, "capacity", rule);
        if (failed(capacity)) return failure();
        SampledQueueProspectiveView parsed;
        parsed.capacityCapture = *capacity;
        if (prospective.get("external_pop")) {
          auto valid = ordinal(prospective, "valid", rule), external = ordinal(prospective, "external_pop", rule);
          auto prior = prospective.getAs<ArrayAttr>("prior_pops");
          if (failed(valid) || failed(external) || !prior) return failure();
          parsed.validCapture = *valid; parsed.externalPopCapture = *external;
          for (Attribute item : prior) {
            if (failed(pay(sizeof(uint64_t)))) return failure();
            auto integer = dyn_cast<IntegerAttr>(item);
            if (!integer || !integer.getType().isSignlessInteger(64) || integer.getInt() < 0) return failure();
            parsed.priorPops.push_back(integer.getInt());
          }
        }
        resource.prospective = std::move(parsed);
      }
      view.resources.push_back(std::move(resource));
    }
    return resources(view);
  }
