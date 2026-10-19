  struct MemorySelection {
    HardwareIdentityEndpoint condition, payload, fallback;
  };
  FailureOr<std::optional<MemorySelection>> memorySelection(
      const HardwareIdentityEndpoint &selected, bool locatorOnly = false) {
    auto leaf = selected;
    SmallVector<Attribute, 0> path;
    for (unsigned depth = 0; depth <= 128; ++depth) {
      if (failed(step(module))) return failure();
      if (auto structure = dyn_cast<StructType>(leaf.resolvedType)) {
        auto create = leaf.value.getDefiningOp<StructCreateOp>();
        auto declaration = analysis.lookupStruct(structure);
        if (!create || !declaration || declaration.getFields().empty() || create.getValues().empty())
          return std::optional<MemorySelection>();
        if (failed(pay(sizeof(Attribute)))) return failure();
        path.push_back(cast<DictionaryAttr>(declaration.getFields()[0]).getAs<StringAttr>("name"));
        auto next = normal(create.getValues()[0], leaf.scope);
        if (failed(next)) return failure();
        leaf = *next; continue;
      }
      if (isa<TableType>(leaf.resolvedType)) {
        auto create = leaf.value.getDefiningOp<TableCreateOp>();
        if (!create || create.getInputs().empty()) return std::optional<MemorySelection>();
        if (failed(pay(sizeof(Attribute)))) return failure();
        path.push_back(Builder(module.getContext()).getI64IntegerAttr(0));
        auto next = normal(create.getInputs()[0], leaf.scope);
        if (failed(next)) return failure();
        leaf = *next; continue;
      }
      bool enumeration = isa<EnumType>(leaf.resolvedType);
      if (enumeration) {
        auto converted = leaf.value.getDefiningOp<EnumFromBitsOp>();
        if (!converted) return std::optional<MemorySelection>();
        auto next = normal(converted.getInput(), leaf.scope);
        if (failed(next)) return failure();
        leaf = *next;
      }
      auto select = leaf.value.getDefiningOp<BitsSelectOp>();
      if (!select) return std::optional<MemorySelection>();
      auto condition = normal(select.getCondition(), leaf.scope);
      Value payload = select.getTrueValue();
      if (enumeration) {
        auto converted = payload.getDefiningOp<EnumToBitsOp>();
        if (!converted) return failure();
        payload = converted.getInput();
      }
      if (locatorOnly) {
        auto projected = [&](Value source) -> FailureOr<std::optional<HardwareIdentityEndpoint>> {
          auto value = normal(source, leaf.scope);
          if (failed(value)) return failure();
          for (Attribute item : llvm::reverse(path)) {
            if (auto field = dyn_cast<StringAttr>(item)) {
              auto get = value->value.getDefiningOp<StructGetOp>();
              if (!get || get.getFieldAttr() != field) return std::optional<HardwareIdentityEndpoint>();
              value = normal(get.getValue(), value->scope);
            } else {
              auto get = value->value.getDefiningOp<TableGetOp>();
              if (!get) return std::optional<HardwareIdentityEndpoint>();
              value = normal(get.getInput(), value->scope);
            }
            if (failed(value)) return failure();
          }
          return std::optional<HardwareIdentityEndpoint>(*value);
        };
        Value fallback = select.getFalseValue();
        if (enumeration) {
          auto converted = fallback.getDefiningOp<EnumToBitsOp>();
          if (!converted) return std::optional<MemorySelection>();
          fallback = converted.getInput();
        }
        auto root = projected(payload), prior = projected(fallback);
        if (failed(condition) || failed(root) || failed(prior)) return failure();
        if (!*root || !*prior) return std::optional<MemorySelection>();
        return std::optional<MemorySelection>(MemorySelection{*condition, **root, **prior});
      }
      auto root = projectionRoot(payload, leaf.scope, path);
      if (failed(condition) || failed(root)) return failure();
      // Reuse the existing complete typed reconstruction proof, including all
      // other fields, Enum carriers, exact predicate and shared fallback owner.
      auto fallback = peelSelection(selected, *condition, *root);
      if (failed(fallback)) return failure();
      return std::optional<MemorySelection>(MemorySelection{*condition, *root, *fallback});
    }
    return module->emitOpError() << "Memory typed pin selection exceeds depth bound";
  }
  FailureOr<bool> memoryIdle(const HardwareIdentityEndpoint &endpoint,
                             unsigned depth = 0) {
    if (depth > 128 || failed(step(module))) return failure();
    if (isa<BitsType>(endpoint.resolvedType)) return literal(endpoint.value, endpoint.scope, false);
    if (auto enumeration = endpoint.value.getDefiningOp<EnumCreateOp>()) {
      auto bound = scopeBindings(endpoint.scope);
      if (failed(bound)) return failure();
      return succeeded(analysis.verifyResolvedOperation(enumeration, *bound));
    }
    auto *operation = endpoint.value.getDefiningOp();
    if (!isa_and_nonnull<StructCreateOp, TableCreateOp, TableSplatOp>(operation)) return false;
    auto bound = scopeBindings(endpoint.scope);
    if (failed(bound) || failed(analysis.verifyResolvedOperation(operation, *bound))) return failure();
    for (Value operand : operation->getOperands()) {
      auto child = normal(operand, endpoint.scope);
      if (failed(child)) return failure();
      auto idle = memoryIdle(*child, depth + 1);
      if (failed(idle) || !*idle) return idle;
    }
    return true;
  }
