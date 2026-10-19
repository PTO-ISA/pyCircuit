  // Reconstruct Memory authority from actual instance occurrences and canonical
  // primitive layouts. No source marker or materializer-owned routing table is
  // trusted by this final-graph proof.
  struct MemoryEndpoint {
    HardwareIdentityScope scope;
    MemoryPrimitiveLayout layout;
  };
  FailureOr<MemoryEndpoint> physicalMemory(DictionaryAttr id, ModuleOp definition,
                                           HardwareIdentityScope at) {
    if (failed(exact(id, {"owner", "declaration"}, definition))) return failure();
    auto owner = id.getAs<DictionaryAttr>("owner");
    if (failed(verifyOwnerRef(owner, error())) ||
        failed(verifyOccurrence(id.getAs<DictionaryAttr>("declaration"), error()))) return failure();
    for (Attribute occurrence : owner.getAs<ArrayAttr>("instance_path")) {
      auto child = childFor(definition, occurrence);
      if (failed(child) || failed(pay(sizeof(InstanceOp)))) return failure();
      auto next = dyn_cast_or_null<ModuleOp>(analysis.resolveCallee(*child));
      if (!next || !analysis.getPrimitiveKind(next).empty() || !next.getBody().hasOneBlock())
        return definition->emitOpError() << "Memory owner path requires actual ordinary instances";
      at.path.push_back(*child); definition = next;
    }
    InstanceOp allocation;
    for (auto candidate : definition.getBody().front().getOps<InstanceOp>()) {
      if (failed(step(candidate))) return failure();
      if (!isMemoryInstance(candidate) || candidate.getOccurrence() != id.get("declaration")) continue;
      if (allocation) return definition->emitOpError() << "ambiguous physical Memory declaration";
      allocation = candidate;
    }
    if (!allocation) return definition->emitOpError() << "Memory metadata has no actual canonical allocation";
    auto bound = scopeBindings(at);
    auto layout = succeeded(bound) ? getMemoryPrimitiveLayout(allocation, *bound)
                                  : FailureOr<MemoryPrimitiveLayout>(failure());
    if (failed(layout)) return failure();
    return MemoryEndpoint{std::move(at), std::move(*layout)};
  }
  LogicalResult memoryResources(const SampledAdmissionView &view,
                                 const HardwareIdentityScope &at) {
    Builder b(module.getContext());
    for (const auto &resource : view.resources) {
      if (resource.kind.getValue() != "memory_live") continue;
      auto id = b.getDictionaryAttr({b.getNamedAttr("owner", resource.owner),
                                    b.getNamedAttr("declaration", resource.declaration)});
      auto target = physicalMemory(id, view.definition, at);
      if (failed(target) || !target->layout.isSynchronous())
        return view.rule->emitOpError() << "memory_live requires synchronous Memory";
      const auto &control = view.controls[resource.control];
      auto raw = normal(control.raw, at);
      auto select = succeeded(raw) ? raw->value.getDefiningOp<BitsSelectOp>() : BitsSelectOp();
      if (!select) return view.rule->emitOpError() << "memory_live requires its path-selected actual live output";
      auto predicate = normal(select.getTrueValue(), raw->scope);
      auto bypass = literal(select.getFalseValue(), raw->scope, true);
      if (failed(predicate) || failed(bypass) || !*bypass ||
          failed(resourcePath(view, control, select.getCondition(), at))) return failure();
      if (!resource.memoryPort || *resource.memoryPort >= target->layout.readPorts)
        return view.rule->emitOpError() << "memory_live requires its exact static response port";
      auto expected = normal(target->layout.allocation->getResult(target->layout.liveResult(*resource.memoryPort)), target->scope);
      if (failed(expected)) return failure();
      bool live = same(*predicate, *expected);
      auto clock = normal(view.clock, at), reset = normal(view.reset, at);
      auto memoryClock = normal(target->layout.allocation.getInputs()[0], target->scope),
           memoryReset = normal(target->layout.allocation.getInputs()[1], target->scope);
      if (!live || failed(clock) || failed(reset) || failed(memoryClock) || failed(memoryReset) ||
          !same(*clock, *memoryClock) || !same(*reset, *memoryReset))
        return view.rule->emitOpError() << "Memory live predicate or domain differs from actual allocation";
    }
    return success();
  }
#include "HardwareMemorySelectionProof.inc"
#include "HardwareMemoryClosureProof.inc"
#include "HardwareMemoryIncomingProof.inc"
  LogicalResult memoryWiring() {
    auto closure = memoryClosureFacts(module, bindings);
    if (failed(closure)) return failure();
    if (!closure->effect) return success();
    struct Proposal {
      const SampledAdmissionView *view;
      const SampledMemoryProposalView *proposal;
      HardwareIdentityScope scope;
      MemoryEndpoint target;
      HardwareIdentityEndpoint identity;
    };
    SmallVector<Proposal, 0> proposals;
    SmallVector<ModuleOp, 0> active;
    std::function<LogicalResult(ModuleOp, const HardwareBindings &, HardwareIdentityScope)> collect =
        [&](ModuleOp definition, const HardwareBindings &bound, HardwareIdentityScope at) -> LogicalResult {
      if (active.size() >= 128 || llvm::is_contained(active, definition))
        return definition->emitOpError() << "cyclic Memory contributor hierarchy";
      if (failed(pay(sizeof(ModuleOp)))) return failure();
      active.push_back(definition);
      for (auto rule : definition.getBody().front().getOps<RuleOp>()) {
        auto metadata = rule->getAttrOfType<DictionaryAttr>("ac.sampled_admission");
        if (!metadata) continue;
        auto resources = metadata.getAs<ArrayAttr>("resources");
        bool hasMemory = metadata.get("memory_proposals") || (resources && llvm::any_of(resources, [](Attribute raw) {
          auto row = dyn_cast<DictionaryAttr>(raw);
          auto kind = row ? row.getAs<StringAttr>("kind") : StringAttr();
          return kind && kind.getValue() == "memory_live";
        }));
        if (!hasMemory) continue;
        auto parsed = sampleFor(rule, bound);
        if (failed(parsed)) return failure();
        auto *view = *parsed;
        if (failed(memoryResources(*view, at))) return failure();
        for (const auto &proposal : view->memoryProposals) {
          auto target = physicalMemory(proposal.memory, definition, at);
          if (failed(target)) return failure();
          bool write = proposal.kind.getValue() == "write";
          bool request = proposal.kind.getValue() == "request";
          if (proposal.port >= target->layout.readPorts || (write && proposal.port != 0) ||
              (request != target->layout.isSynchronous() && !write))
            return rule->emitOpError() << "Memory proposal action/port differs from primitive layout";
          auto identity = normal(target->layout.allocation.getOutputs()[0], target->scope);
          auto clock = normal(view->clock, at), reset = normal(view->reset, at);
          auto memoryClock = normal(target->layout.allocation.getInputs()[0], target->scope),
               memoryReset = normal(target->layout.allocation.getInputs()[1], target->scope);
          if (failed(identity) || failed(clock) || failed(reset) || failed(memoryClock) || failed(memoryReset) ||
              !same(*clock, *memoryClock) || !same(*reset, *memoryReset))
            return rule->emitOpError() << "Memory proposal domain differs from physical allocation";
          if (failed(pay(sizeof(Proposal) + at.path.size() * sizeof(InstanceOp)))) return failure();
          proposals.push_back({view, &proposal, at, std::move(*target), *identity});
        }
      }
      for (auto instance : definition.getBody().front().getOps<InstanceOp>()) {
        auto child = dyn_cast_or_null<ModuleOp>(analysis.resolveCallee(instance));
        if (!child || !analysis.getPrimitiveKind(child).empty()) continue;
        auto childBound = analysis.bindInstance(instance, bound);
        if (failed(childBound)) return failure();
        auto childFacts = memoryClosureFacts(child, *childBound);
        if (failed(childFacts)) return failure();
        if (!childFacts->effect) continue;
        if (failed(pay(sizeof(HardwareIdentityScope) + (at.path.size() + 1) * sizeof(InstanceOp)))) return failure();
        auto nested = at; nested.path.push_back(instance);
        if (failed(collect(child, *childBound, std::move(nested)))) return failure();
      }
      active.pop_back(); return success();
    };
    if (failed(collect(module, bindings, scope))) return failure();
#include "HardwareMemoryGrantProof.inc"
#include "HardwareMemoryReadCoverage.inc"
    // Port identity is physical allocation + direction/port. Source order does
    // not manufacture additional ports, including across distinct Rules.
    for (size_t i = 0; i < proposals.size(); ++i) {
      auto &left = proposals[i];
      for (auto &right : MutableArrayRef(proposals).drop_front(i + 1)) {
        if (failed(step(module))) return failure();
        bool leftWrite = left.proposal->kind.getValue() == "write";
        bool rightWrite = right.proposal->kind.getValue() == "write";
        if (!same(left.identity, right.identity) || leftWrite != rightWrite ||
            (!leftWrite && left.proposal->port != right.proposal->port)) continue;
        if (left.proposal->kind.getValue() == "read" && right.proposal->kind.getValue() == "read") {
          auto equalAddress = sameAddress(left, right);
          if (failed(equalAddress)) return failure();
          if (*equalAddress) continue;
        }
        auto a = grantValue(left.proposal->rawPath, left.scope),
             b = grantValue(right.proposal->rawPath, right.scope);
        if (failed(a) || failed(b)) return failure();
        auto separated = grantProof.disjoint(*a, *b);
        if (failed(separated)) return failure();
        bool disjoint = *separated;
        if (!disjoint) return module->emitOpError() << "Memory port conflict: potentially simultaneous actual-instance proposals";
      }
    }
    auto abstractInputs = memoryIncomingCount();
    auto rootPorts = getModulePortLayout(module);
    if (failed(abstractInputs) || failed(rootPorts)) return failure();
    uint64_t validityInputs = llvm::count_if(rootPorts->inputValidity,
        [](int64_t ordinal) { return ordinal >= 0; });
    uint64_t abstractEnd = module.getFunctionType().getNumInputs() - validityInputs;
    uint64_t abstractStart = abstractEnd - *abstractInputs;
    // Reconstruct every driven functional pin. Normalization follows private
    // Module inputs through actual instances, so ancestor proposals and child
    // local proposals meet at the same physical pin without a routing locator.
    for (size_t owner = 0; owner < proposals.size(); ++owner) {
      auto &representative = proposals[owner];
      bool seen = false;
      for (size_t previous = 0; previous < owner; ++previous) {
        if (failed(step(module))) return failure();
        seen |= same(proposals[previous].identity, representative.identity);
      }
      if (seen) continue;
      const auto &layout = representative.target.layout;
      for (unsigned pin = 2; pin < layout.allocation->getNumOperands(); ++pin) {
        struct Contribution { const Proposal *proposal; std::optional<unsigned> argument; };
        SmallVector<Contribution, 0> expected;
        for (auto [proposalOrdinal, entry] : llvm::enumerate(proposals)) {
          if (failed(step(module))) return failure();
          if (!same(entry.identity, representative.identity)) continue;
          auto kind = entry.proposal->kind.getValue();
          if (kind == "read" && coveredReads[proposalOrdinal]) continue;
          std::optional<unsigned> argument;
          bool contributes = false;
          if (kind == "write") {
            contributes = pin == layout.writeEnableOperand;
            for (auto [ordinal, role] : llvm::enumerate(ArrayRef<unsigned>{layout.writeAddressOperand, layout.writeDataOperand, layout.writeMaskOperand}))
              if (pin == role) { contributes = true; argument = ordinal; }
          } else {
            contributes = kind == "request" && pin == layout.readEnableOperands[entry.proposal->port];
            if (pin == layout.readAddressOperands[entry.proposal->port]) { contributes = true; argument = 0; }
          }
          if (contributes) {
            if (failed(pay(sizeof(Contribution)))) return failure();
            expected.push_back({&entry, argument});
          }
        }
#include "HardwareMemoryPinProof.inc"
      }
    }
    return success();
  }
