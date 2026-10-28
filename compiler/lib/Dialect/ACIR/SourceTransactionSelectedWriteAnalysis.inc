  FailureOr<Type> selectedWriteType(Type root, ArrayAttr selectors,
                                     ArrayRef<BoundValueId> indices,
                                     InvocationFrameId frame, Operation *site) {
    Type current = root;
    unsigned nextIndex = 0;
    for (Attribute raw : selectors) {
      if (!budget.charge(site, 4)) return failure();
      auto selector = cast<DictionaryAttr>(raw);
      if (auto field = selector.getAs<StringAttr>("field")) {
        auto structure = dyn_cast<StructType>(current);
        auto declaration = structure ? hardware.lookupStruct(structure) : StructOp();
        Type selected;
        if (declaration) for (Attribute rawField : declaration.getFields()) {
          if (!budget.charge(site)) return failure();
          auto item = cast<DictionaryAttr>(rawField);
          if (item.getAs<StringAttr>("name") == field) {
            auto resolved = hardware.resolveType(item.getAs<TypeAttr>("type").getValue(),
                                                  plan.instances[0].bindings, site);
            if (failed(resolved)) return failure();
            selected = *resolved;
            break;
          }
        }
        if (!selected) return site->emitOpError() << "selected write requires an actual owner field";
        current = selected;
      } else {
        auto table = dyn_cast<TableType>(current);
        auto ordinal = selector.getAs<IntegerAttr>("index");
        if (!table || !ordinal || ordinal.getInt() != nextIndex || nextIndex >= indices.size())
          return site->emitOpError() << "selected write requires sequential actual Table indices";
        auto indexType = payloadType(indices[nextIndex], site);
        auto indexWidth = failed(indexType) ? FailureOr<uint64_t>(failure())
            : hardware.getPackedWidth(*indexType, plan.instances[0].bindings, site);
        if (failed(indexType) || !isa<BitsType>(*indexType) || failed(indexWidth)) return failure();
        auto shape = hardware.getTableShape(table, plan.instances[0].bindings, site);
        if (failed(shape) || shape->empty()) return failure();
        current = table.getShape().size() == 1 ? table.getElementType()
            : Type(TableType::get(package.getContext(),
                ArrayAttr::get(package.getContext(), table.getShape().getValue().drop_front()),
                table.getElementType()));
        ++nextIndex;
      }
    }
    if (nextIndex != indices.size())
      return site->emitOpError() << "selected write requires exact bound index count";
    return current;
  }

  FailureOr<SmallVector<BoundValueId>> writeIndices(Operation *site,
                                                     ValueRange source,
                                                     InvocationFrameId frame) {
    SmallVector<BoundValueId> result;
    if (!budget.chargeStorage<BoundValueId>(site, source.size()) ||
        !budget.reserve(site, result, source.size())) return failure();
    for (Value index : source) {
      auto bound = useValue(index, plan.frames[frame].anchor, frame, site);
      if (failed(bound)) return failure();
      result.push_back(*bound);
    }
    return result;
  }

  FailureOr<bool> forwardedStateProposal(SourceStateWriteOp operation,
                                          InvocationFrameId frame) {
    auto data = dyn_cast<OpResult>(operation.getValue());
    auto call = data ? dyn_cast<SourceMethodCallOp>(data.getOwner()) : SourceMethodCallOp();
    if (!call) return false;
    for (const auto &binding : plan.calls) {
      if (!budget.charge(operation)) return failure();
      if (binding.operation != call || binding.caller != frame) continue;
      const auto &callee = plan.frames[binding.callee];
      SourceMethodView method{plan.instances[callee.anchor].definition, callee.method, {}};
      auto proposals = method.getFormalProposals();
      if (!proposals) return false;
      auto mappings = call->getAttrOfType<ArrayAttr>("ac.formal_owner_bindings");
      for (auto [ordinal, formal] : llvm::enumerate(proposals)) {
        if (!budget.charge(operation, 4)) return failure();
        unsigned base = method.getNumPublicResults() + 2 * ordinal;
        if (data.getResultNumber() != base || call->getResult(base + 1) != operation.getPath()) continue;
        if (mappings) for (Attribute raw : mappings) {
          if (!budget.charge(operation)) return failure();
          auto mapping = cast<DictionaryAttr>(raw);
          if (mapping.get("formal") == formal && mapping.get("state") == operation.getStateAttr())
            return true;
        }
        return operation.emitOpError() << "physical proposal forwarding lacks its exact owner binding";
      }
      return false;
    }
    return false;
  }
