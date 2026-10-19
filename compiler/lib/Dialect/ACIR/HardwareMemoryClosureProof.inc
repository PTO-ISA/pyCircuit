  FailureOr<MemoryClosureFacts> memoryClosureFacts(
      ModuleOp definition, const HardwareBindings &bound) {
    if (failed(step(definition))) return failure();
    auto found = demandInventory.memoryClosure.find(definition);
    if (found != demandInventory.memoryClosure.end())
      for (const auto &entry : found->second) {
        if (failed(step(definition, SourceCheckBudget::add(SourceCheckBudget::bindings(bound),
                                                         SourceCheckBudget::bindings(entry->bindings))))) return failure();
        if (!entry->complete)
          return definition->emitOpError() << "cyclic Memory effect definition closure";
        if (!sameHardwareBindings(bound, entry->bindings)) continue;
        return entry->facts;
      }
    if (!definition.getBody().hasOneBlock())
      return definition->emitOpError() << "Memory effect closure requires ordinary graph definitions";
    if (failed(pay(sizeof(MemoryClosureEntry) + sizeof(std::unique_ptr<MemoryClosureEntry>) +
          SourceCheckBudget::multiply(8, SourceCheckBudget::bindings(bound)) +
          (found == demandInventory.memoryClosure.end() ? sizeof(Operation *) + 64 : 0)))) return failure();
    auto owned = std::make_unique<MemoryClosureEntry>();
    owned->bindings = bound;
    auto *entry = owned.get();
    if (found == demandInventory.memoryClosure.end())
      found = demandInventory.memoryClosure.try_emplace(definition).first;
    found->second.push_back(std::move(owned));
    // Only definition/binding presence is memoized. Each required actual
    // occurrence still proves its live predicate, domain, conflicts and pins.
    for (Operation &operation : definition.getBody().front()) {
      if (failed(step(&operation))) return failure();
      if (auto rule = dyn_cast<RuleOp>(operation)) {
        auto metadata = rule->getAttrOfType<DictionaryAttr>("ac.sampled_admission");
        if (metadata) {
          entry->facts.effect |= bool(metadata.get("memory_proposals"));
          if (auto resources = metadata.getAs<ArrayAttr>("resources"))
            for (Attribute raw : resources) {
              if (failed(step(rule))) return failure();
              auto row = dyn_cast<DictionaryAttr>(raw);
              auto kind = row ? row.getAs<StringAttr>("kind") : StringAttr();
              entry->facts.effect |= kind && kind.getValue() == "memory_live";
            }
        }
      }
      auto instance = dyn_cast<InstanceOp>(operation);
      if (!instance) continue;
      entry->facts.allocation |= isMemoryInstance(instance);
      auto child = dyn_cast_or_null<ModuleOp>(analysis.resolveCallee(instance));
      if (!child || !analysis.getPrimitiveKind(child).empty()) continue;
      if (failed(step(instance, SourceCheckBudget::add(32, SourceCheckBudget::bindings(bound))))) return failure();
      auto childBound = analysis.bindInstance(instance, bound);
      if (failed(childBound)) return failure();
      auto childFacts = memoryClosureFacts(child, *childBound);
      if (failed(childFacts)) return failure();
      entry->facts.allocation |= childFacts->allocation;
      entry->facts.effect |= childFacts->effect;
    }
    entry->complete = true;
    return entry->facts;
  }
