  if (writeDomains) {
    auto wiring = module->getAttrOfType<DictionaryAttr>("ac.transaction_wiring");
    auto writers = wiring ? wiring.getAs<ArrayAttr>("writers") : ArrayAttr();
    ArrayAttr candidates;
    if (writers) for (Attribute raw : writers) {
      auto writer = dyn_cast<DictionaryAttr>(raw);
      if (!writer || !identical(writer.get("registration"), rule.getOccurrence()))
        continue;
      if (candidates)
        return rule->emitOpError() << "write domains have ambiguous transaction writer";
      candidates = writer.getAs<ArrayAttr>("candidates");
    }
    if (!candidates)
      return rule->emitOpError() << "write domains require their transaction candidate inventory";
    if (failed(charge(budget, rule, writeDomains.size(), sizeof(SampledWriteDomainView))))
      return failure();
    for (auto [ordinal, raw] : llvm::enumerate(writeDomains)) {
      auto domain = dyn_cast<DictionaryAttr>(raw);
      if (failed(exact(domain, {"candidate", "selectors", "mask"}, rule,
                       "sampled write domain"))) return failure();
      auto candidateIndex = index(domain, "candidate", rule);
      auto maskIndex = index(domain, "mask", rule);
      auto selectors = domain.getAs<ArrayAttr>("selectors");
      if (failed(candidateIndex) || failed(maskIndex) || !selectors ||
          uint64_t(*candidateIndex) >= candidates.size() ||
          uint64_t(*maskIndex) != queueResult + ordinal ||
          uint64_t(*maskIndex) >= yielded->getNumOperands())
        return rule->emitOpError() << "write domain candidate or mask is outside its exact suffix";
      auto candidate = dyn_cast<DictionaryAttr>(candidates[*candidateIndex]);
      auto state = candidate ? candidate.getAs<DictionaryAttr>("state") : DictionaryAttr();
      auto target = resolveTransactionState(module, state, analysis, bindings, budget);
      if (failed(target)) return failure();
      Type selected = target->payloadType;
      TableType table;
      SmallVector<Attribute> prefix, suffix;
      for (Attribute rawSelector : selectors) {
        if (failed(budget.debit(1, rule))) return failure();
        auto selector = dyn_cast<DictionaryAttr>(rawSelector);
        if (!selector || selector.size() != 1)
          return rule->emitOpError() << "write domain selector requires exactly field or index";
        if (auto field = selector.getAs<StringAttr>("field")) {
          auto record = dyn_cast<StructType>(selected);
          auto declaration = record ? analysis.lookupStruct(record) : StructOp();
          if (!declaration)
            return rule->emitOpError() << "write domain field requires its nominal Struct";
          Type member;
          for (Attribute rawField : declaration.getFields()) {
            if (failed(budget.debit(1, rule))) return failure();
            auto item = cast<DictionaryAttr>(rawField);
            if (item.getAs<StringAttr>("name") == field) {
              auto type = analysis.resolveType(item.getAs<TypeAttr>("type").getValue(),
                                                target->ownerBindings, rule);
              if (failed(type)) return failure();
              member = *type;
              break;
            }
          }
          if (!member) return rule->emitOpError() << "write domain selects an unknown Struct field";
          (table ? suffix : prefix).push_back(field);
          selected = member;
          continue;
        }
        auto axis = selector.getAs<IntegerAttr>("index");
        auto indexed = dyn_cast<TableType>(selected);
        if (!axis || !axis.getType().isSignlessInteger(64) || axis.getInt() != 0 ||
            table || !indexed || indexed.getShape().size() != 1)
          return rule->emitOpError() << "write domain requires one sequential index into a fixed one-dimensional Table";
        table = indexed;
        selected = indexed.getElementType();
      }
      if (!table) {
        suffix = std::move(prefix);
        prefix.clear();
      }
      Value mask = yielded->getOperand(*maskIndex);
      if (!table) {
        if (failed(bitOne(mask, analysis, bindings, rule, budget))) return failure();
      } else {
        auto resolved = analysis.resolveType(mask.getType(), bindings, rule);
        auto maskTable = succeeded(resolved) ? dyn_cast<TableType>(*resolved) : TableType();
        if (!maskTable || !isa<BitsType>(maskTable.getElementType()))
          return rule->emitOpError() << "indexed write domain mask requires a Table of Bits1";
        auto width = analysis.getPackedWidth(maskTable.getElementType(), bindings, rule);
        if (failed(width) || *width != 1)
          return rule->emitOpError() << "write domain mask requires exact Bits1 elements";
        if (analysis.isStaticEvaluable(table.getShape(), target->ownerBindings) &&
            analysis.isStaticEvaluable(maskTable.getShape(), bindings)) {
          auto shape = analysis.getTableShape(table, target->ownerBindings, rule);
          auto maskShape = analysis.getTableShape(maskTable, bindings, rule);
          if (failed(shape) || failed(maskShape) || *shape != *maskShape)
            return rule->emitOpError() << "write domain mask shape differs from selected state Table";
        } else {
          // Catalog schemas retain the already substituted logical shapes;
          // every actual scope re-enters the concrete branch above.
          auto shaped = TableType::get(rule.getContext(), table.getShape(), maskTable.getElementType());
          if (!areEquivalentHardwareTypes(shaped, maskTable))
            return rule->emitOpError() << "write domain mask shape differs from selected state Table";
        }
      }
      result.writeDomains.push_back({uint64_t(*candidateIndex), uint64_t(*maskIndex),
          selectors, mask, state, target->payloadType, table,
          ArrayAttr::get(rule.getContext(), prefix), ArrayAttr::get(rule.getContext(), suffix)});
    }
  }
