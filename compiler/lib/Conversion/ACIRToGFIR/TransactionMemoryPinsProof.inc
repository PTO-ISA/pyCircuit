  LogicalResult memoryPinsProof() {
    using PinKey = detail::QueueRoutingKey;
    struct ExpectedPin {
      ac::ModuleOp module;
      SmallVector<ac::InstanceOp> path;
      ac::InstanceOp memory;
      unsigned operand;
      SmallVector<std::pair<Value, Value>, 0> local;
    };
    SmallVector<ExpectedPin, 0> expected;
    llvm::DenseMap<Operation *, llvm::DenseMap<PinKey, unsigned>> index;
    auto key = [&](ArrayRef<ac::InstanceOp> path, ac::InstanceOp memory, unsigned operand) -> PinKey {
      return {{routePath(prepared.original->getContext(), path), memory.getOutputs()[0]}, operand};
    };
    auto demand = [&](ac::ModuleOp module, ArrayRef<ac::InstanceOp> path,
                      ac::InstanceOp memory, unsigned operand) -> FailureOr<unsigned> {
      auto identity = key(path, memory, operand);
      auto found = index[module].find(identity);
      if (found != index[module].end()) return found->second;
      if (!charge(budget, memory, 32 + path.size())) return failure();
      unsigned id = expected.size();
      index[module].try_emplace(identity, id);
      expected.push_back({module, SmallVector<ac::InstanceOp>(path), memory, operand, {}});
      return id;
    };
    // Derive private route inventory from original source accesses, including
    // actual lexical edges and physical pin roles, rather than candidate rows.
    llvm::DenseMap<Operation *, SmallVector<PinKey, 0>> routes;
    for (auto registration : prepared.original->getRegion(0).front().getOps<ac::ModuleOp>()) {
      const auto &routing = prepared.definitions[prepared.definitionIndex.lookup(registration)];
      for (uint64_t occurrenceIndex : routing.transactionIndices) {
        auto transactionIndex = originalTransactionIndex(occurrenceIndex);
        if (failed(transactionIndex)) return failure();
        const auto &transaction = prepared.transactions[*transactionIndex];
        const auto &plan = *transaction.plan;
        if (plan.routingModule != registration) return reject("Memory transaction routing owner differs");
        uint64_t proposalOrdinal = 0;
        for (auto [accessId, access] : llvm::enumerate(plan.memoryAccesses)) {
          if (access.kind == ac::TransactionMemoryAccessKind::Result) continue;
          if (proposalOrdinal >= transaction.memoryProposals.size()) return failure();
          const auto &proposal = transaction.memoryProposals[proposalOrdinal++];
          if (proposal.access != accessId) return reject("original Memory proposal source order differs");
          // Original source replay independently validates the reuse witness.
          // A covered Read contributes no additional physical select or OR.
          if (proposal.physicalReuse) continue;
          const auto &owner = plan.memoryOwners[access.owner];
          auto path = anchorPath(plan, owner.anchor);
          auto operands = memoryOperands(access, owner.layout);
          unsigned argument = 0;
          auto rule = candidate.transactions[*transactionIndex].rule;
          for (unsigned operand : operands) {
            auto local = demand(plan.routingModule, path, owner.allocation, operand);
            if (failed(local)) return failure();
            bool enable = access.kind == ac::TransactionMemoryAccessKind::Write && operand == owner.layout.writeEnableOperand;
            enable |= access.kind == ac::TransactionMemoryAccessKind::Request && operand == owner.layout.readEnableOperands[access.port];
            Value data = enable ? Value() : rule->getResult(proposal.argumentResults[argument++]);
            expected[*local].local.push_back({rule->getResult(proposal.pathResult), data});
          }
          for (unsigned offset = 0; offset < path.size(); ++offset) {
            auto child = dyn_cast_or_null<ac::ModuleOp>(hardware.resolveCallee(path[offset]));
            if (!child) return failure();
            auto suffix = ArrayRef<ac::InstanceOp>(path).drop_front(offset + 1);
            for (unsigned operand : operands) {
              auto identity = key(suffix, owner.allocation, operand);
              auto &inventory = routes[child];
              if (!llvm::is_contained(inventory, identity)) inventory.push_back(identity);
            }
          }
        }
        if (proposalOrdinal != transaction.memoryProposals.size()) return reject("original Memory proposal inventory differs");
      }
    }
    for (const auto &definition : prepared.definitions) {
      const auto &inventory = routes[definition.module];
      if (inventory.size() != definition.memoryIncoming.size()) return reject("original Memory private route inventory differs");
      for (auto [ordinal, route] : llvm::enumerate(definition.memoryIncoming)) {
        auto identity = key(route.path, route.allocation, route.operand);
        if (identity != inventory[ordinal] || definition.memoryIncomingIndex.lookup(identity) != ordinal)
          return reject("original Memory private route identity/order differs");
      }
      for (auto original : definition.module->getRegion(0).front().getOps<ac::InstanceOp>()) {
        auto child = dyn_cast_or_null<ac::ModuleOp>(hardware.resolveCallee(original));
        if (child) {
          const auto &callee = prepared.definitions[prepared.definitionIndex.lookup(child)];
          for (const auto &route : callee.memoryIncoming) {
            SmallVector<ac::InstanceOp> path{original}; llvm::append_range(path, route.path);
            if (failed(demand(definition.module, path, route.allocation, route.operand))) return failure();
          }
        } else if (original->hasAttr("ac.source_memory")) {
          for (unsigned operand = 2; operand < original.getNumOperands(); ++operand)
            if (failed(demand(definition.module, {}, original, operand))) return failure();
        }
      }
    }
    if (expected.size() != candidate.memoryPins.size()) return reject("original Memory pin merge inventory differs");
    llvm::DenseSet<std::pair<Operation *, PinKey>> seen;
    for (const auto &actual : candidate.memoryPins) {
      auto foundModule = index.find(actual.module);
      if (foundModule == index.end()) return reject("original Memory pin module differs");
      auto found = foundModule->second.find(actual.key);
      if (found == foundModule->second.end() || !seen.insert({actual.module, actual.key}).second)
        return reject("original Memory pin identity differs or repeats");
      const auto &pin = expected[found->second];
      const auto &definition = prepared.definitions[prepared.definitionIndex.lookup(pin.module)];
      auto module = cast<ac::ModuleOp>(candidate.packageMap.lookup(pin.module.operator->()));
      auto memory = pin.memory;
      Location location = pin.path.empty() ? memory.getLoc() : pin.path.front()->getLoc();
      ac::HardwareBindings bindings; bindings.owner = pin.module;
      for (auto edge : pin.path) {
        auto child = hardware.bindInstance(edge, bindings);
        if (failed(child)) return failure();
        bindings = *child;
      }
      auto type = hardware.resolveType(memory.getInputs()[pin.operand].getType(), bindings, module);
      auto bit = hardware.resolveType(memory.getInputs()[0].getType(), bindings, module);
      if (failed(type) || failed(bit)) return failure();
      SmallVector<std::pair<Value, Value>, 0> contributors;
      auto incoming = definition.memoryIncomingIndex.find(actual.key);
      if (incoming != definition.memoryIncomingIndex.end()) {
        uint64_t base = definition.sourceInputs + 2 * definition.downward.size() + queueInputCount(definition) + 2 * incoming->second;
        contributors.push_back({module.getBody().front().getArgument(base), module.getBody().front().getArgument(base + 1)});
      }
      llvm::append_range(contributors, pin.local);
      if (actual.selections.size() != contributors.size())
        return reject("original Memory typed pin selection inventory differs");
      Value active = actual.active;
      Builder b(module.getContext());
      for (auto [path, payload] : llvm::reverse(contributors)) {
        (void)payload;
        auto joined = active.getDefiningOp<ac::BitsBinaryOp>();
        if (!joined || joined.getRhs() != path ||
            failed(generatedProof(joined, ac::BitsBinaryOp::getOperationName(),
                {joined.getLhs(), path}, {*bit}, b.getDictionaryAttr({b.getNamedAttr("opcode", b.getStringAttr("or"))}), location, &module.getBody().front())))
          return reject("original Memory pin proposal path union differs");
        active = joined.getLhs();
      }
      if (failed(literalProof(active, *bit, memory.getOccurrence(), location, false, &module.getBody().front()))) return failure();
      Value data = actual.idle;
      if (isa<ac::BitsType>(*type)) {
        if (failed(literalProof(data, *type, memory.getOccurrence(), location, false, &module.getBody().front()))) return failure();
      } else {
        auto idle = routed(pin.module, pin.path, memory.getInputs()[pin.operand]);
        if (failed(idle) || data != *idle)
          return reject("original Memory nominal idle carrier differs");
      }
      // Bindings are local to this physical pin. A cache keyed by their
      // address must not survive into the next pin's reconstructed bindings.
      BranchTypeAdmissionCache pinTypes;
      Operation *last = nullptr;
      for (auto [ordinal, contribution] : llvm::enumerate(contributors)) {
        auto [path, payload] = contribution;
        const auto &selection = actual.selections[ordinal];
        if (!payload) {
          auto selected = selection.value ? selection.value.getDefiningOp<ac::BitsSelectOp>() : ac::BitsSelectOp();
          if (!selected || !operations.contains(selected.operator->()) || !isa<ac::BitsType>(*type))
            return reject("original Memory issue enable carrier differs");
          payload = selected.getTrueValue();
          if (failed(literalProof(payload, *type, memory.getOccurrence(), location, true, &module.getBody().front()))) return failure();
        }
        if (failed(localBranchProof(memory, *type, bindings, path, payload, data,
                                    selection, &module.getBody().front(), last, &pinTypes))) return failure();
        data = selection.value;
      }
      if (data != actual.data) return reject("original Memory typed pin merge result differs");
      if (pin.path.empty()) {
        auto allocation = cast<ac::InstanceOp>(candidate.packageMap.lookup(memory.operator->()));
        if (allocation.getInputs()[pin.operand] != actual.data) return reject("original Memory physical pin differs");
      } else {
        auto allocation = cast<ac::InstanceOp>(candidate.packageMap.lookup(pin.path.front().operator->()));
        auto child = dyn_cast<ac::ModuleOp>(hardware.resolveCallee(pin.path.front()));
        const auto &callee = prepared.definitions[prepared.definitionIndex.lookup(child)];
        auto route = callee.memoryIncomingIndex.find(key(ArrayRef<ac::InstanceOp>(pin.path).drop_front(), memory, pin.operand));
        if (route == callee.memoryIncomingIndex.end()) return failure();
        uint64_t base = callee.sourceInputs + 2 * callee.downward.size() + queueInputCount(callee) + 2 * route->second;
        if (allocation.getInputs()[base] != actual.active || allocation.getInputs()[base + 1] != actual.data)
          return reject("original Memory hierarchical pin pair differs");
      }
    }
    return success();
  }
