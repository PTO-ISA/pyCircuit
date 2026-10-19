// Included inside QueueWiringProof. Routing inventory and contributor identity
// are reconstructed separately from the complete admission-prefix proof.
  FailureOr<DictionaryAttr> queueID(ArrayAttr path, Attribute declaration) {
    if (failed(pay(sizeof(DictionaryAttr) * 4 + path.size() * sizeof(Attribute)))) return failure();
    Builder b(module.getContext());
    return b.getDictionaryAttr({b.getNamedAttr("owner", b.getDictionaryAttr({
        b.getNamedAttr("instance_path", path)})),
        b.getNamedAttr("declaration", declaration)});
  }
  FailureOr<InstanceOp> childFor(ModuleOp definition, Attribute occurrence) {
    InstanceOp result;
    for (auto instance : definition.getBody().front().getOps<InstanceOp>()) {
      if (failed(step(instance))) return failure();
      if (instance.getOccurrence() != occurrence) continue;
      if (result) return module->emitOpError() << "ambiguous prospective Queue instance";
      result = instance;
    }
    if (!result) return module->emitOpError() << "prospective Queue instance is missing";
    return result;
  }
  FailureOr<HardwareIdentityEndpoint> physicalQueue(
      DictionaryAttr id, ModuleOp definition, HardwareIdentityScope at) {
    auto owner = id.getAs<DictionaryAttr>("owner");
    if (!owner) return failure();
    auto path = owner.getAs<ArrayAttr>("instance_path");
    if (!path) return failure();
    for (Attribute occurrence : path) {
      auto instance = childFor(definition, occurrence);
      if (failed(instance) || failed(pay(sizeof(InstanceOp)))) return failure();
      at.path.push_back(*instance);
      definition = dyn_cast_or_null<ModuleOp>(analysis.resolveCallee(*instance));
      if (!definition || !definition.getBody().hasOneBlock()) return failure();
    }
    InstanceOp allocation;
    for (auto queue : definition.getBody().front().getOps<InstanceOp>()) {
      if (failed(step(queue))) return failure();
      if (!isQueueInstance(queue) ||
          queue.getOccurrence() != id.get("declaration")) continue;
      if (allocation) return module->emitOpError() << "ambiguous physical Queue declaration";
      allocation = queue;
    }
    if (!allocation) return module->emitOpError() << "prospective Queue has no actual allocation";
    return normal(allocation.getOutputs()[1], at);
  }
  LogicalResult disjointProposals() {
    struct Proposal {
      const SampledAdmissionView *view;
      const SampledQueueProposalView *proposal;
      HardwareIdentityScope scope;
      HardwareIdentityEndpoint queue;
      std::optional<PathFactors> rawFactors;
    };
    SmallVector<Proposal, 0> all;
    SmallVector<ModuleOp> active;
    std::function<LogicalResult(ModuleOp, const HardwareBindings &, HardwareIdentityScope)> collect =
        [&](ModuleOp definition, const HardwareBindings &bound, HardwareIdentityScope at) -> LogicalResult {
      if (active.size() >= 128 || llvm::is_contained(active, definition))
        return definition->emitOpError() << "cyclic prospective Queue contributor hierarchy";
      if (failed(pay(sizeof(ModuleOp)))) return failure();
      active.push_back(definition);
      for (auto rule : definition.getBody().front().getOps<RuleOp>()) {
        if (failed(step(rule))) return failure();
        if (!rule->hasAttr("ac.sampled_admission")) continue;
        auto parsed = sampleFor(rule, bound);
        if (failed(parsed)) return failure();
        const auto *view = *parsed;
        for (const auto &proposal : view->queueProposals) {
          auto target = physicalQueue(proposal.queue, definition, at);
          if (failed(target) || failed(pay(sizeof(Proposal) + at.path.size() * sizeof(InstanceOp)))) return failure();
          all.push_back({view, &proposal, at, *target, std::nullopt});
        }
      }
      for (auto instance : definition.getBody().front().getOps<InstanceOp>()) {
        auto child = dyn_cast_or_null<ModuleOp>(analysis.resolveCallee(instance));
        if (!child || !analysis.getPrimitiveKind(child).empty()) continue;
        auto nextBound = analysis.bindInstance(instance, bound);
        if (failed(nextBound) || failed(pay(sizeof(HardwareIdentityScope) + (at.path.size() + 1) * sizeof(InstanceOp)))) return failure();
        auto next = at; next.path.push_back(instance);
        if (failed(collect(child, *nextBound, std::move(next)))) return failure();
      }
      active.pop_back();
      return success();
    };
    if (failed(collect(module, bindings, scope))) return failure();
    auto rawFactors = [&](Proposal &proposal) -> FailureOr<const PathFactors *> {
      if (!proposal.rawFactors) {
        auto parsed = factors(proposal.proposal->rawPath, proposal.scope, true);
        if (failed(parsed)) return failure();
        proposal.rawFactors = std::move(*parsed);
      }
      return &*proposal.rawFactors;
    };
    for (size_t i = 0; i < all.size(); ++i)
      for (auto &right : MutableArrayRef(all).drop_front(i + 1)) {
        auto &left = all[i];
        if (failed(step(module))) return failure();
        if (left.proposal->kind != right.proposal->kind || !same(left.queue, right.queue)) continue;
        auto a = rawFactors(left), b = rawFactors(right);
        if (failed(a) || failed(b)) return failure();
        bool disjoint = (*a)->falsePath || (*b)->falsePath;
        for (const auto &x : (*a)->atoms)
          for (const auto &y : (*b)->atoms) {
            if (failed(step(module))) return failure();
            if (x.positive == y.positive || !same(x.value, y.value)) continue;
            auto knownLeft = retainedSelector(*left.view, *left.proposal, x.value, left.scope);
            auto knownRight = retainedSelector(*right.view, *right.proposal, x.value, right.scope);
            if (failed(knownLeft) || failed(knownRight)) return failure();
            disjoint |= *knownLeft && *knownRight;
          }
        if (!disjoint)
          return module->emitOpError() << "queue port conflict: actual-instance proposals lack opposite paths with retained selector knownness";
      }
    return success();
  }
  LogicalResult disabledChildPushes(InstanceOp instance, ArrayAttr incoming) {
    // A shared definition may have push-only ports demanded by another caller.
    // Its unused occurrence is legal without parent routing only when each
    // actual enable is independently proven zero. Pop ancestry still requires
    // the complete parent inventory and is never waived by this case.
    for (Attribute raw : incoming) {
      if (failed(step(instance))) return failure();
      auto row = dyn_cast<DictionaryAttr>(raw);
      auto kind = row ? row.getAs<StringAttr>("kind") : StringAttr();
      if (!kind || kind.getValue() != "push")
        return module->emitOpError() << "child Queue pop routes require owning Module queue wiring";
      if (failed(exact(row, {"queue", "kind", "path", "data"}, instance))) return failure();
      auto path = ordinal(row, "path", instance), data = ordinal(row, "data", instance);
      auto id = row.getAs<DictionaryAttr>("queue");
      if (failed(path) || failed(data) || *path >= instance.getNumOperands() ||
          *data >= instance.getNumOperands() || *data != *path + 1 ||
          failed(exact(id, {"owner", "declaration"}, instance))) return failure();
      auto owner = id.getAs<DictionaryAttr>("owner");
      if (failed(verifyOwnerRef(owner, error()))) return failure();
      auto sourcePath = owner.getAs<ArrayAttr>("instance_path");
      if (failed(pay((sourcePath.size() + 1) * sizeof(Attribute)))) return failure();
      SmallVector<Attribute> actualPath{instance.getOccurrence()};
      llvm::append_range(actualPath, sourcePath);
      Builder b(module.getContext());
      auto relative = queueID(b.getArrayAttr(actualPath), id.get("declaration"));
      if (failed(relative)) return failure();
      auto target = resolve(*relative);
      if (failed(target) || !target->unboundPush) return failure();
      Value enable = instance.getOperand(*path), payload = instance.getOperand(*data);
      auto enableType = analysis.resolveType(enable.getType(), bindings, instance);
      auto width = succeeded(enableType) ? analysis.getPackedWidth(*enableType, bindings, instance)
                                        : FailureOr<uint64_t>(failure());
      auto payloadType = analysis.resolveType(payload.getType(), bindings, instance);
      auto expectedType = analysis.resolveType(
          target->queue.getInputs()[3].getType(), target->bindings, instance);
      auto zero = literal(enable, scope, false);
      if (failed(enableType) || !isa<BitsType>(*enableType) || failed(width) || *width != 1 ||
          failed(payloadType) || failed(expectedType) ||
          !areEquivalentHardwareTypes(*payloadType, *expectedType) || failed(zero) || !*zero)
        return instance->emitOpError() << "unrouted child Queue push requires typed payload and known-zero actual enable";
      if (failed(allow(instance, *path, true, RouteEnable)) || failed(allow(instance, *data, true, RouteData))) return failure();
    }
    return success();
  }
  LogicalResult childSummaryRoots() {
    if (!module.getBody().hasOneBlock()) return success();
    for (auto instance : module.getBody().front().getOps<InstanceOp>()) {
      if (failed(step(instance))) return failure();
      auto child = dyn_cast_or_null<ModuleOp>(analysis.resolveCallee(instance));
      auto wiring = child ? child->getAttrOfType<DictionaryAttr>("ac.queue_wiring") : DictionaryAttr();
      auto rows = wiring ? wiring.getAs<ArrayAttr>("pop_summaries") : ArrayAttr();
      if (wiring && wiring.get("pop_summaries") && (!rows || rows.empty())) return failure();
      if (!rows) continue;
      llvm::DenseSet<uint64_t> outputs;
      for (Attribute raw : rows) {
        auto row = dyn_cast<DictionaryAttr>(raw);
        auto output = ordinal(row, "output", instance);
        if (failed(exact(row, {"queue", "output"}, instance)) || failed(output) ||
            *output >= instance.getNumResults() || !outputs.insert(*output).second) return failure();
        Value result = instance.getResult(*output);
        auto type = analysis.resolveType(result.getType(), bindings, instance);
        auto width = succeeded(type) ? analysis.getPackedWidth(*type, bindings, instance) : FailureOr<uint64_t>(failure());
        if (failed(type) || !isa<BitsType>(*type) || failed(width) || *width != 1 ||
            failed(privateRoot(result, RouteEnable)) || failed(pay(sizeof(Value) * 3))) return failure();
        unusedSummaryRoots.insert(result);
      }
    }
    return success();
  }
  // Locate the next upward summary without interpreting it as an incoming
  // accepted-pop route. The child runs this same independent subtree proof.
  FailureOr<Value> subtree(DictionaryAttr id) {
    auto path = id.getAs<DictionaryAttr>("owner").getAs<ArrayAttr>("instance_path");
    if (path.empty()) {
      auto target = resolve(id);
      if (failed(target)) return failure();
      return target->unboundPop ? Value() : target->queue.getInputs()[4];
    }
    auto instance = childFor(module, path[0]);
    if (failed(instance)) return failure();
    auto child = dyn_cast_or_null<ModuleOp>(analysis.resolveCallee(*instance));
    if (!child) return failure();
    Builder b(module.getContext());
    auto suffix = queueID(b.getArrayAttr(path.getValue().drop_front()), id.get("declaration"));
    if (failed(suffix)) return failure();
    auto wiring = child->getAttrOfType<DictionaryAttr>("ac.queue_wiring");
    auto summaries = wiring ? wiring.getAs<ArrayAttr>("pop_summaries") : ArrayAttr();
    std::optional<uint64_t> found;
    if (summaries) for (Attribute raw : summaries) {
      auto row = dyn_cast<DictionaryAttr>(raw);
      if (!row || row.get("queue") != *suffix) continue;
      auto output = ordinal(row, "output", child);
      if (failed(output) || found || *output >= (*instance).getNumResults()) return failure();
      found = *output;
    }
    if (!found) return module->emitOpError() << "prospective Queue requires its exact child subtree summary";
    Value value = (*instance)->getResult(*found);
    if (failed(privateRoot(value, RouteEnable))) return failure();
    return value;
  }
  LogicalResult unionOf(Value value, ArrayRef<Value> contributors) {
    auto actual = normal(value, scope);
    if (failed(actual)) return failure();
    if (contributors.empty()) {
      auto zero = literal(value, scope, false);
      if (failed(zero) || !*zero)
        return module->emitOpError() << "empty prospective Queue union requires literal zero";
      return success();
    }
    for (size_t i = contributors.size(); i > 1; --i) {
      auto op = actual->value.getDefiningOp<BitsBinaryOp>();
      if (!op || op.getOpcode() != "or")
        return module->emitOpError() << "prospective Queue union requires ordered four-state OR";
      auto rhs = normal(op.getRhs(), actual->scope), expected = normal(contributors[i - 1], scope);
      if (failed(rhs) || failed(expected)) return failure();
      if (!same(*rhs, *expected))
        return module->emitOpError() << "prospective Queue union contributor differs from its actual owner";
      if (failed(allow(op, 0, false, RouteEnable)) ||
          failed(allow(op, 1, false, RouteEnable))) return failure();
      actual = normal(op.getLhs(), actual->scope);
      if (failed(actual)) return failure();
    }
    auto expected = normal(contributors.front(), scope);
    if (failed(expected) || !same(*actual, *expected))
      return module->emitOpError() << "prospective Queue union differs from its complete contributor inventory";
    return success();
  }
  FailureOr<const QueueDemands *> routeDemands() {
    if (failed(step(module))) return failure();
    if (demandInventory.complete) {
      auto found = demandInventory.definitionIndex.find(module);
      if (found == demandInventory.definitionIndex.end()) return failure();
      return &demandInventory.definitions[found->second].demands;
    }
    SmallVector<QueueDemandDefinition, 0> definitions;
    llvm::DenseMap<Operation *, size_t> definitionIndex;
    for (auto definition : analysis.getPackage().getBody()->getOps<ModuleOp>()) {
      if (!definition.getBody().hasOneBlock()) continue;
      if (failed(pay(sizeof(QueueDemandDefinition) + sizeof(Operation *) * 3))) return failure();
      definitionIndex.try_emplace(definition, definitions.size());
      definitions.push_back({definition, {}});
    }
    auto lookup = [&](ModuleOp definition) -> QueueDemandDefinition * {
      auto found = definitionIndex.find(definition);
      return found == definitionIndex.end() ? nullptr : &definitions[found->second];
    };
    auto demand = [&](ModuleOp definition, DictionaryAttr id, bool push, bool summary) -> LogicalResult {
      auto *entry = lookup(definition);
      if (!entry || failed(step(definition, 1 + entry->demands.first.size() + entry->demands.second.size()))) return failure();
      if (summary) {
        if (!llvm::is_contained(entry->demands.second, id)) {
          if (failed(pay(sizeof(DictionaryAttr)))) return failure();
          entry->demands.second.push_back(id);
        }
      } else if (!llvm::is_contained(entry->demands.first, IncomingDemand{id, push})) {
        if (failed(pay(sizeof(IncomingDemand)))) return failure();
        entry->demands.first.push_back({id, push});
      }
      return success();
    };
    auto along = [&](ModuleOp definition, DictionaryAttr id, bool push, bool summary) -> LogicalResult {
      auto owner = id ? id.getAs<DictionaryAttr>("owner") : DictionaryAttr();
      auto path = owner ? owner.getAs<ArrayAttr>("instance_path") : ArrayAttr();
      if (!path || path.size() > 128) return failure();
      for (size_t i = 0; i < path.size(); ++i) {
        auto instance = childFor(definition, path[i]);
        if (failed(instance)) return failure();
        definition = dyn_cast_or_null<ModuleOp>(analysis.resolveCallee(*instance));
        if (!definition) return failure();
        Builder b(module.getContext());
        auto suffix = queueID(b.getArrayAttr(path.getValue().drop_front(i + 1)), id.get("declaration"));
        if (failed(suffix) || failed(demand(definition, *suffix, push, summary))) return failure();
      }
      return success();
    };
    for (auto &entry : definitions)
      for (auto rule : entry.module.getBody().front().getOps<RuleOp>()) {
        auto sampled = rule->getAttrOfType<DictionaryAttr>("ac.sampled_admission");
        if (!sampled) continue;
        if (auto proposals = sampled.getAs<ArrayAttr>("queue_proposals"))
          for (Attribute raw : proposals) {
            auto row = dyn_cast<DictionaryAttr>(raw);
            auto kind = row ? row.getAs<StringAttr>("kind") : StringAttr();
            auto id = row ? row.getAs<DictionaryAttr>("queue") : DictionaryAttr();
            if (!kind || !id || failed(along(entry.module, id, kind.getValue() == "push", false))) return failure();
          }
        auto resources = sampled.getAs<ArrayAttr>("resources");
        if (!resources) continue;
        for (Attribute raw : resources) {
          if (failed(step(rule))) return failure();
          auto row = dyn_cast<DictionaryAttr>(raw);
          auto prospective = row ? row.getAs<DictionaryAttr>("prospective") : DictionaryAttr();
          if (!prospective || !prospective.get("external_pop")) continue;
          auto owner = row.getAs<DictionaryAttr>("owner");
          auto path = owner ? owner.getAs<ArrayAttr>("instance_path") : ArrayAttr();
          if (!path) return failure();
          Builder b(module.getContext());
          auto id = b.getDictionaryAttr({b.getNamedAttr("owner", owner), b.getNamedAttr("declaration", row.get("declaration"))});
          ModuleOp current = entry.module;
          for (Attribute occurrence : path) {
            auto instance = childFor(current, occurrence);
            if (failed(instance)) return failure();
            current = dyn_cast_or_null<ModuleOp>(analysis.resolveCallee(*instance));
            if (!current) return failure();
          }
          auto inventory = current->getAttrOfType<DictionaryAttr>("ac.queue_wiring");
          auto queues = inventory ? inventory.getAs<ArrayAttr>("queues") : ArrayAttr();
          std::optional<bool> unbound;
          if (queues) for (Attribute rawQueue : queues) {
            auto queue = dyn_cast<DictionaryAttr>(rawQueue);
            if (!queue || queue.get("declaration") != row.get("declaration")) continue;
            auto pop = queue.getAs<BoolAttr>("unbound_pop");
            if (!pop || unbound) return failure();
            unbound = pop.getValue();
          }
          if (!unbound) return failure();
          if (*unbound && failed(demand(entry.module, id, false, false))) return failure();
          if (failed(along(entry.module, id, false, true))) return failure();
        }
      }
    bool changed;
    uint64_t round = 0;
    do {
      if (++round > definitions.size() + 1)
        return module->emitOpError() << "cyclic Queue incoming demand hierarchy";
      changed = false;
      for (auto &entry : definitions)
        for (auto instance : entry.module.getBody().front().getOps<InstanceOp>()) {
          auto child = dyn_cast_or_null<ModuleOp>(analysis.resolveCallee(instance));
          auto *nested = child ? lookup(child) : nullptr;
          if (!nested) continue;
          if (child == entry.module) return instance->emitOpError() << "cyclic Queue incoming hierarchy";
          // Copy because a recursive forged hierarchy may target this vector.
          if (failed(pay(nested->demands.first.size() * sizeof(IncomingDemand)))) return failure();
          SmallVector<IncomingDemand> routes(nested->demands.first);
          for (const auto &[id, push] : routes) {
            if (push) continue;
            auto owner = id.getAs<DictionaryAttr>("owner");
            auto path = owner.getAs<ArrayAttr>("instance_path");
            if (path.size() >= 128) return failure();
            SmallVector<Attribute> prefixed{instance.getOccurrence()};
            if (failed(pay((path.size() + 1) * sizeof(Attribute)))) return failure();
            llvm::append_range(prefixed, path);
            Builder b(module.getContext());
            auto relative = queueID(b.getArrayAttr(prefixed), id.get("declaration"));
            if (failed(relative)) return failure();
            auto before = entry.demands.first.size();
            if (failed(demand(entry.module, *relative, false, false))) return failure();
            changed |= before != entry.demands.first.size();
          }
        }
    } while (changed);
    auto found = definitionIndex.find(module);
    if (found == definitionIndex.end()) return failure();
    size_t index = found->second;
    // Publish only a completed package closure. The inventory remains fixed
    // while every module independently checks its actual routing and unions.
    demandInventory.definitions = std::move(definitions);
    demandInventory.definitionIndex = std::move(definitionIndex);
    demandInventory.complete = true;
    return &demandInventory.definitions[index].demands;
  }
  LogicalResult prospectiveRoutes(DictionaryAttr wiring, const ModulePortLayout &ports) {
    // Derive required upward routes from every actual downstream-pop resource
    // request, translating the physical path at each lexical instance edge.
    auto demands = routeDemands();
    if (failed(demands)) return failure();
    const auto &requiredSummaries = (*demands)->second;
    if ((*demands)->first.size() != incomingPins.size())
      return module->emitOpError() << "Queue incoming routes differ from complete actual demand closure";
    for (auto [expected, actual] : llvm::zip((*demands)->first, incomingPins))
      if (expected.first != actual.id || expected.second != actual.push)
        return module->emitOpError() << "Queue incoming route order or physical identity differs from actual demand closure";
    auto summaries = wiring.getAs<ArrayAttr>("pop_summaries");
    if (requiredSummaries.size() != (summaries ? summaries.size() : 0))
      return module->emitOpError() << "Queue subtree summaries differ from actual ancestor demand inventory";
    uint64_t validityOutputs = llvm::count_if(ports.outputValidity, [](int64_t i) { return i >= 0; });
    validityOutputs += ports.routedOutputValidity.size();
    if (module.getFunctionType().getNumResults() < validityOutputs + requiredSummaries.size()) return failure();
    uint64_t outputBase = module.getFunctionType().getNumResults() - validityOutputs - requiredSummaries.size();
    if (outputBase < ports.sourceOutputCount) return failure();
    for (auto [payload, companion] : ports.routedOutputValidity)
      if (payload < int64_t(ports.sourceOutputCount) || payload >= int64_t(outputBase))
        return module->emitOpError() << "Queue summary outputs cannot be relabeled as routed payload availability";
    auto yielded = cast<YieldOp>(module.getBody().front().getTerminator());
    if (summaries) for (auto [i, raw] : llvm::enumerate(summaries)) {
      auto row = dyn_cast<DictionaryAttr>(raw);
      auto output = ordinal(row, "output", module);
      auto id = row ? row.getAs<DictionaryAttr>("queue") : DictionaryAttr();
      if (failed(exact(row, {"queue", "output"}, module)) || failed(output) ||
          *output != outputBase + i || id != requiredSummaries[i] ||
          *output >= yielded.getNumOperands())
        return module->emitOpError() << "Queue summary requires its exact private output suffix";
      auto target = resolve(id);
      auto child = subtree(id);
      auto type = analysis.resolveType(yielded.getOperand(*output).getType(), bindings, module);
      auto width = succeeded(type) ? analysis.getPackedWidth(*type, bindings, module) : FailureOr<uint64_t>(failure());
      if (failed(target) || failed(child) || failed(type) || !isa<BitsType>(*type) || failed(width) || *width != 1)
        return module->emitOpError() << "Queue summary requires a private Bits1 output";
      SmallVector<Value> values;
      for (const auto &pin : localPins) if (!pin.push && pin.id == id) values.push_back(pin.path);
      if (*child) values.push_back(*child);
      if (failed(unionOf(yielded.getOperand(*output), values)) ||
          failed(allow(yielded, *output, true, RouteEnable)) ||
          failed(pay(2 * sizeof(OpOperand *)))) return failure();
      // A raw-bound public input or an empty union is not itself private.
      // Record the authenticated summary edge without tainting its SSA origin.
      verifiedUses.insert(&yielded->getOpOperand(*output));
    }
    struct External { RuleOp rule; DictionaryAttr id; uint64_t capture; };
    SmallVector<External> expected;
    for (auto rule : module.getBody().front().getOps<RuleOp>()) {
      auto sampled = rule->getAttrOfType<DictionaryAttr>("ac.sampled_admission");
      auto resources = sampled ? sampled.getAs<ArrayAttr>("resources") : ArrayAttr();
      if (!resources) continue;
      for (Attribute raw : resources) {
        if (failed(step(rule))) return failure();
        auto row = dyn_cast<DictionaryAttr>(raw);
        auto prospective = row ? row.getAs<DictionaryAttr>("prospective") : DictionaryAttr();
        if (!prospective || !prospective.get("external_pop")) continue;
        auto capture = ordinal(prospective, "external_pop", rule);
        Builder b(module.getContext());
        auto id = b.getDictionaryAttr({b.getNamedAttr("owner", row.get("owner")), b.getNamedAttr("declaration", row.get("declaration"))});
        if (failed(capture) || *capture >= rule.getNumOperands()) return failure();
        auto clock = ordinal(sampled, "clock_capture", rule), reset = ordinal(sampled, "reset_capture", rule);
        if (failed(clock) || failed(reset) || *capture == *clock || *capture == *reset)
          return rule->emitOpError() << "external-pop capture cannot reuse a domain capture";
        auto prior = llvm::find_if(expected, [&](const External &item) { return item.rule == rule && item.id == id; });
        if (prior != expected.end()) {
          if (prior->capture != *capture) return rule->emitOpError() << "Queue external pop requires one dedicated capture per actual owner";
          continue;
        }
        if (llvm::any_of(expected, [&](const External &item) { return item.rule == rule && item.capture == *capture; }))
          return rule->emitOpError() << "different Queue owners cannot share an external-pop capture";
        if (!expected.empty() && expected.back().rule == rule && *capture != expected.back().capture + 1)
          return rule->emitOpError() << "external-pop captures require a contiguous dedicated suffix before availability";
        if (failed(pay(sizeof(External)))) return failure();
        expected.push_back({rule, id, *capture});
      }
    }
    auto external = wiring.getAs<ArrayAttr>("external_pops");
    if (expected.size() != (external ? external.size() : 0))
      return module->emitOpError() << "external Queue routes differ from complete downstream-pop resource inventory";
    if (external) for (auto [i, raw] : llvm::enumerate(external)) {
      auto row = dyn_cast<DictionaryAttr>(raw);
      auto capture = ordinal(row, "capture", module);
      const auto &item = expected[i];
      if (failed(exact(row, {"registration", "queue", "capture"}, module)) || failed(capture) ||
          row.get("registration") != item.rule->getAttrOfType<DictionaryAttr>("occurrence") || row.get("queue") != item.id || *capture != item.capture)
        return module->emitOpError() << "external Queue route differs from actual registration/owner/capture";
      auto target = resolve(item.id);
      auto child = subtree(item.id);
      auto config = succeeded(target)
                        ? analysis.resolveQueue(target->queue, target->bindings)
                        : FailureOr<ResolvedQueueConfig>(failure());
      if (failed(target) || failed(child) || failed(config) ||
          config->readyPolicy != QueueReadyPolicy::DownstreamPop)
        return failure();
      SmallVector<Value> values;
      for (const auto &pin : incomingPins) if (!pin.push && pin.id == item.id) values.push_back(pin.path);
      if (values.size() != unsigned(target->unboundPop))
        return module->emitOpError() << "external Queue pop requires its exact authored/unbound ancestor inventory";
      for (const auto &pin : localPins)
        if (!pin.push && pin.id == item.id && pin.rule != item.rule) values.push_back(pin.path);
      if (*child) values.push_back(*child);
      Value actual = item.rule->getOperand(item.capture);
      if (failed(unionOf(actual, values)) || failed(allow(item.rule, item.capture, true, RouteEnable))) return failure();
    }
    return success();
  }
