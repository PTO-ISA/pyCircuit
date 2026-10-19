  FailureOr<uint64_t> memoryIncomingCount() {
    auto ports = getModulePortLayout(module);
    if (failed(ports)) return failure();
    uint64_t start = ports->sourceInputCount;
    if (auto wiring = module->getAttrOfType<DictionaryAttr>("ac.transaction_wiring")) {
      auto incoming = wiring.getAs<ArrayAttr>("incoming");
      if (!incoming) return failure();
      start = SourceCheckBudget::add(start, SourceCheckBudget::multiply(2, incoming.size()));
    }
    if (auto wiring = module->getAttrOfType<DictionaryAttr>("ac.queue_wiring")) {
      auto incoming = wiring.getAs<ArrayAttr>("incoming");
      if (!incoming) return failure();
      for (Attribute raw : incoming) {
        if (failed(step(module))) return failure();
        auto row = dyn_cast<DictionaryAttr>(raw);
        auto kind = row ? row.getAs<StringAttr>("kind") : StringAttr();
        if (!kind || (kind.getValue() != "push" && kind.getValue() != "pop")) return failure();
        start = SourceCheckBudget::add(start, kind.getValue() == "push" ? 2 : 1);
      }
    }
    uint64_t validity = llvm::count_if(ports->inputValidity, [](int64_t ordinal) { return ordinal >= 0; });
    uint64_t inputs = module.getFunctionType().getNumInputs();
    if (SourceCheckBudget::add(start, validity) == inputs) return uint64_t(0);
    auto closure = memoryClosureFacts(module, bindings);
    if (failed(closure)) return failure();
    // A missing state/Queue row is diagnosed by its owning verifier. Without
    // an actual Memory allocation, unexplained ports cannot be Memory routes.
    if (!closure->allocation) return uint64_t(0);
    if (validity > inputs || start > inputs - validity || (inputs - validity - start) % 2)
      return uint64_t(0);
    uint64_t end = inputs - validity;
    llvm::DenseSet<uint64_t> used;
    SmallVector<ModuleOp, 0> active;
    std::function<LogicalResult(ModuleOp, HardwareIdentityScope)> visit =
        [&](ModuleOp definition, HardwareIdentityScope at) -> LogicalResult {
      if (active.size() >= 128 || llvm::is_contained(active, definition))
        return definition->emitOpError() << "cyclic Memory incoming route hierarchy";
      if (failed(pay(sizeof(ModuleOp)))) return failure();
      active.push_back(definition);
      for (auto instance : definition.getBody().front().getOps<InstanceOp>()) {
        if (failed(step(instance))) return failure();
        if (isMemoryInstance(instance)) {
          for (unsigned pin = 2; pin < instance.getNumOperands(); ++pin) {
            auto actual = normal(instance->getOperand(pin), at);
            if (failed(actual)) return failure();
            SmallVector<std::pair<HardwareIdentityEndpoint, unsigned>, 0> pending;
            if (failed(pay(sizeof(HardwareIdentityEndpoint) + sizeof(unsigned)))) return failure();
            pending.push_back({*actual, 0});
            while (!pending.empty()) {
              auto [value, depth] = pending.pop_back_val();
              if (depth > 128) return module->emitOpError() << "Memory incoming route exceeds depth bound";
              auto selected = memorySelection(value, true);
              if (failed(selected)) return failure();
              if (!*selected) continue;
              const auto &entry = **selected;
              auto path = dyn_cast<BlockArgument>(entry.condition.value), data = dyn_cast<BlockArgument>(entry.payload.value);
              if (path && path.getOwner()->getParentOp() == module && entry.condition.scope.path.empty() &&
                  path.getArgNumber() >= start && path.getArgNumber() < end) {
                uint64_t ordinal = path.getArgNumber();
                if (!data || data.getOwner() != path.getOwner() || !entry.payload.scope.path.empty() ||
                    data.getArgNumber() != ordinal + 1 || (ordinal - start) % 2 ||
                    !areEquivalentHardwareTypes(entry.payload.resolvedType, value.resolvedType))
                  return module->emitOpError() << "Memory incoming route mixes physical path/data pairs";
                auto fallback = peelSelection(value, entry.condition, entry.payload);
                if (failed(fallback)) return failure();
                auto width = analysis.getPackedWidth(entry.condition.resolvedType, bindings, module);
                if (failed(width) || !isa<BitsType>(entry.condition.resolvedType) || *width != 1 ||
                    failed(pay(sizeof(uint64_t) * 3)) || !used.insert(ordinal).second)
                  return module->emitOpError() << "Memory incoming pair must drive one physical pin exactly once";
              }
              if (failed(pay(2 * (sizeof(HardwareIdentityEndpoint) + sizeof(unsigned))))) return failure();
              pending.push_back({entry.payload, depth + 1});
              pending.push_back({entry.fallback, depth + 1});
            }
          }
          continue;
        }
        auto child = dyn_cast_or_null<ModuleOp>(analysis.resolveCallee(instance));
        if (!child || !analysis.getPrimitiveKind(child).empty()) continue;
        if (failed(pay(sizeof(HardwareIdentityScope) + (at.path.size() + 1) * sizeof(InstanceOp)))) return failure();
        auto nested = at; nested.path.push_back(instance);
        if (failed(visit(child, std::move(nested)))) return failure();
      }
      active.pop_back(); return success();
    };
    if (failed(visit(module, scope))) return failure();
    // An unrelated raw Memory is not authority for unexplained private ports.
    // Zero/partial coverage leaves those ports to the caller's exact suffix
    // check; returning zero cannot make its declared input inventory complete.
    for (uint64_t ordinal = start; ordinal < end; ordinal += 2) {
      if (failed(step(module))) return failure();
      if (!used.contains(ordinal)) return uint64_t(0);
    }
    auto internal = module->getAttrOfType<DictionaryAttr>("ac.internal_ports");
    auto defaults = internal ? internal.getAs<ArrayAttr>("root_inputs") : ArrayAttr();
    for (uint64_t ordinal = start; ordinal < end; ordinal += 2) {
      if (failed(step(module))) return failure();
      if (!defaults || ordinal + 1 - ports->sourceInputCount >= defaults.size())
        return module->emitOpError() << "Memory incoming route lacks its exact root defaults";
      for (unsigned offset = 0; offset < 2; ++offset) {
        auto value = dyn_cast<IntegerAttr>(defaults[ordinal + offset - ports->sourceInputCount]);
        if (!value || !value.getValue().isZero())
          return module->emitOpError() << "Memory incoming root defaults must be known zero";
      }
    }
    return end - start;
  }
