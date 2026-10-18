  LogicalResult writeDomains(TransactionPreparation &prepared) {
    const auto &plan = *prepared.plan;
    if (llvm::none_of(plan.writes, [](const auto &write) {
          return !write.selectors.empty();
        })) return success();
    auto predicateType = hardware.resolveType(plan.values[plan.clockValue].value.getType(),
                                               plan.frames[plan.rootFrame].bindings,
                                               plan.transaction);
    if (failed(predicateType)) return failure();
    Builder b(plan.transaction->getContext());
    for (auto [id, write] : llvm::enumerate(plan.writes)) {
      if (!charge(budget, write.operation, 4)) return failure();
      if (write.forwardedProposal) continue;
      detail::TransactionWriteDomain domain;
      domain.write = id;
      domain.selectors = write.selectors;
      domain.maskType = *predicateType;
      Type current = plan.owners[write.owner].payloadType;
      SmallVector<Attribute> before, after;
      bool indexed = false;
      for (Attribute raw : write.selectors) {
        if (!charge(budget, write.operation, 4)) return failure();
        auto selector = cast<DictionaryAttr>(raw);
        if (auto field = selector.getAs<StringAttr>("field")) {
          auto structure = dyn_cast<ac::StructType>(current);
          auto declaration = structure ? hardware.lookupStruct(structure) : ac::StructOp();
          Type next;
          if (declaration) for (Attribute rawField : declaration.getFields()) {
            if (!charge(budget, write.operation)) return failure();
            auto item = cast<DictionaryAttr>(rawField);
            if (item.getAs<StringAttr>("name") == field) {
              auto resolved = hardware.resolveType(item.getAs<TypeAttr>("type").getValue(),
                                                    plan.frames[write.frame].bindings,
                                                    write.operation);
              if (failed(resolved)) return failure();
              next = *resolved;
              break;
            }
          }
          if (!next) return write.operation->emitOpError() << "partial write field layout differs from owner";
          (indexed ? after : before).push_back(field);
          current = next;
        } else {
          auto table = dyn_cast<ac::TableType>(current);
          if (indexed || !table || table.getShape().size() != 1 || write.indices.size() != 1)
            return write.operation->emitOpError()
                << "partial write composition requires at most one Table index with static field paths";
          domain.table = table;
          domain.index = write.indices.front();
          domain.maskType = ac::TableType::get(b.getContext(), table.getShape(), *predicateType);
          current = table.getElementType();
          indexed = true;
        }
      }
      domain.prefix = b.getArrayAttr(before);
      domain.suffix = b.getArrayAttr(after);
      if (!indexed) domain.suffix = domain.prefix;
      auto output = llvm::find_if(prepared.outputs, [&](const auto &candidate) {
        return candidate.binding.owner == write.owner;
      });
      if (output == prepared.outputs.end())
        return write.operation->emitOpError() << "partial write has no actual owner candidate";
      domain.candidate = output - prepared.outputs.begin();
      if (!prepayAppend(budget, write.operation, prepared.writeDomains)) return failure();
      prepared.writeDomains.push_back(std::move(domain));
    }
    return success();
  }
