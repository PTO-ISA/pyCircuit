  FailureOr<detail::MaterializedSelectedWrite>
  selectedWrite(const ac::StateWriteIntent &write,
                 const detail::TransactionWriteDomain &domain,
                 Value previous, Value rhs, OpBuilder &builder) {
    detail::MaterializedSelectedWrite result;
    auto site = write.operation;
    auto loc = site->getLoc();
    auto make = [&](OpBuilder &at, StringRef name, ValueRange operands,
                    TypeRange types, ArrayRef<NamedAttribute> attributes,
                    unsigned regions = 0) -> FailureOr<Operation *> {
      if (!charge(budget, site, 32 + 4 * operands.size() + 2 * types.size() +
                                  2 * attributes.size()) ||
          !prepayAppend(budget, site, result.generated)) return failure();
      OperationState state(loc, name);
      state.addOperands(operands); state.addTypes(types); state.addAttributes(attributes);
      for (unsigned i = 0; i < regions; ++i) state.addRegion();
      Operation *created = at.create(state);
      result.generated.push_back(created);
      return created;
    };
    auto raw = rawStatementPath(site, write.authoredPath,
        plan.versions[write.version].validity.path, builder, &result.generated);
    if (failed(raw)) return failure();
    result.rawPath = *raw;
    result.mask = result.rawPath;
    result.data = rhs;
    if (write.selectors.empty()) return result;
    ac::HardwareAnalysis hardware(plan.transaction->getParentOfType<mlir::ModuleOp>());
    auto one = intentLiteral(prefix.getType(), true, loc, builder);
    if (failed(one) || !prepayAppend(budget, site, result.generated)) return failure();
    result.generated.push_back(one->getDefiningOp());
    auto merge = [&](OpBuilder &at, Value base, Value guard, Value value,
                     ArrayAttr path) -> FailureOr<Value> {
      auto merged = make(at, ac::ValueMergeOp::getOperationName(),
          {base, guard, value}, {base.getType(), guard.getType()},
          {at.getNamedAttr("paths", at.getArrayAttr({path})),
           at.getNamedAttr("operandSegmentSizes", at.getDenseI32ArrayAttr({1, 1, 1}))});
      if (failed(merged)) return failure();
      return (*merged)->getResult(0);
    };
    if (!domain.table) {
      auto updated = merge(builder, previous, *one, rhs, domain.suffix);
      if (failed(updated)) return failure();
      result.data = *updated;
      return result;
    }
    Value table = previous;
    for (Attribute raw : domain.prefix) {
      auto field = cast<StringAttr>(raw);
      auto declaration = hardware.lookupStruct(cast<ac::StructType>(table.getType()));
      Type fieldType;
      for (Attribute rawField : declaration.getFields()) {
        if (!charge(budget, site)) return failure();
        auto item = cast<DictionaryAttr>(rawField);
        if (item.getAs<StringAttr>("name") == field) {
          auto resolved = hardware.resolveType(item.getAs<TypeAttr>("type").getValue(),
                                                plan.frames[write.frame].bindings, site);
          if (failed(resolved)) return failure();
          fieldType = *resolved;
          break;
        }
      }
      if (!fieldType) return site->emitOpError() << "selected write lost its owner field";
      auto projected = make(builder, ac::StructGetOp::getOperationName(), {table}, {fieldType},
                            {builder.getNamedAttr("field", field)});
      if (failed(projected)) return failure();
      table = (*projected)->getResult(0);
    }
    auto index = bound(*domain.index);
    if (failed(index)) return failure();
    SmallVector<Value> tables{table};
    for (Value value : {*index, rhs, result.rawPath}) {
      Type type = ac::TableType::get(builder.getContext(), domain.table.getShape(), value.getType());
      auto splat = make(builder, ac::TableSplatOp::getOperationName(), {value}, {type},
                        {builder.getNamedAttr("shape", domain.table.getShape())});
      if (failed(splat)) return failure();
      tables.push_back((*splat)->getResult(0));
    }
    auto mapped = make(builder, ac::TableMapOp::getOperationName(), tables,
        {domain.table, domain.maskType},
        {builder.getNamedAttr("shape", domain.table.getShape()),
         builder.getNamedAttr("operandSegmentSizes", builder.getDenseI32ArrayAttr({4, 0}))}, 1);
    auto count = hardware.getTableSize(domain.table, plan.frames[write.frame].bindings, site);
    if (failed(mapped) || failed(count) || !*count ||
        !charge(budget, site, 16 + SourceCheckBudget::bytes(sizeof(Block)))) return failure();
    auto *body = new Block;
    (*mapped)->getRegion(0).push_back(body);
    unsigned ordinalWidth = std::max(1u, llvm::Log2_64_Ceil(*count));
    Type ordinalType = branchCarrierType(ordinalWidth, site);
    Value ordinal = body->addArgument(ordinalType, loc);
    Value old = body->addArgument(domain.table.getElementType(), loc);
    Value actualIndex = body->addArgument(index->getType(), loc);
    Value replacement = body->addArgument(rhs.getType(), loc);
    Value path = body->addArgument(result.rawPath.getType(), loc);
    auto nested = OpBuilder::atBlockEnd(body);
    auto width = hardware.getPackedWidth(actualIndex.getType(), plan.frames[write.frame].bindings, site);
    if (failed(width)) return failure();
    if (*width == ordinalWidth && !ac::areEquivalentHardwareTypes(actualIndex.getType(), ordinalType)) {
      auto adapter = make(nested, ac::BitsExtractOp::getOperationName(), {actualIndex}, {ordinalType},
          {nested.getNamedAttr("low", bitsLiteralAttribute(ordinalType, site->getAttrOfType<DictionaryAttr>("occurrence"),
                                                           builder.getContext(), false))});
      if (failed(adapter)) return failure();
      actualIndex = (*adapter)->getResult(0);
    } else if (*width < ordinalWidth) {
      auto widened = make(nested, ac::BitsResizeOp::getOperationName(), {actualIndex}, {ordinalType},
                          {nested.getNamedAttr("mode", nested.getStringAttr("zext"))});
      if (failed(widened)) return failure();
      actualIndex = (*widened)->getResult(0);
    } else if (*width > ordinalWidth) {
      auto widened = make(nested, ac::BitsResizeOp::getOperationName(), {ordinal}, {actualIndex.getType()},
                          {nested.getNamedAttr("mode", nested.getStringAttr("zext"))});
      if (failed(widened)) return failure();
      ordinal = (*widened)->getResult(0);
    }
    auto compared = make(nested, ac::BitsCompareOp::getOperationName(), {ordinal, actualIndex}, {prefix.getType()},
                         {nested.getNamedAttr("predicate", nested.getStringAttr("eq"))});
    if (failed(compared)) return failure();
    Value equal = (*compared)->getResult(0);
    auto value = merge(nested, old, equal, replacement, domain.suffix);
    auto mask = make(nested, ac::BitsBinaryOp::getOperationName(), {path, equal}, {path.getType()},
                     {nested.getNamedAttr("opcode", nested.getStringAttr("and"))});
    if (failed(value) || failed(mask)) return failure();
    auto yielded = make(nested, ac::YieldOp::getOperationName(), {*value, (*mask)->getResult(0)}, {}, {});
    if (failed(yielded)) return failure();
    result.data = (*mapped)->getResult(0);
    result.mask = (*mapped)->getResult(1);
    if (!domain.prefix.empty()) {
      auto updated = merge(builder, previous, *one, result.data, domain.prefix);
      if (failed(updated)) return failure();
      result.data = *updated;
    }
    return result;
  }
