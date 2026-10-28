  LogicalResult selectedWriteProof(const TransactionPreparation &preparation,
                                    const MaterializedTransaction &materialized,
                                    uint64_t id, Value previous, Value payload,
                                    llvm::function_ref<Value(ac::BoundValueId)> bound,
                                    Block *parent, Operation *&last) {
    const auto &plan = *preparation.plan;
    const auto &write = plan.writes[id];
    auto site = write.operation;
    const auto &owner = plan.owners[write.owner];
    auto declaration = owner.declaration;
    auto originalTable = dyn_cast<ac::TableType>(declaration.getQ().getType());
    if (originalTable && !write.indices.empty()) {
      auto original = dyn_cast<ac::SourceStateWriteOp>(site);
      if (original && (original.getIndices().size() != 1 ||
          !charge(budget, site, 32) ||
          failed(hardware.verifySourceIndex(original.getIndices().front(), originalTable,
              plan.instances[owner.anchor].bindings, site)))) return failure();
    }
    auto declared = llvm::find_if(preparation.writeDomains,
        [&](const auto &domain) { return domain.write == id; });
    if (declared == preparation.writeDomains.end()) return success();
    if (id >= materialized.selectedWrites.size()) return reject("original selected write inventory differs");
    const auto &record = materialized.selectedWrites[id];
    Builder b(site->getContext());
    Type current = plan.owners[write.owner].payloadType;
    ac::TableType table;
    SmallVector<Attribute> before, after;
    for (Attribute raw : write.selectors) {
      if (!charge(budget, site, 4)) return failure();
      auto selector = cast<DictionaryAttr>(raw);
      if (auto field = selector.getAs<StringAttr>("field")) {
        auto structure = dyn_cast<ac::StructType>(current);
        auto declaration = structure ? hardware.lookupStruct(structure) : ac::StructOp();
        Type next;
        if (declaration) for (Attribute rawField : declaration.getFields()) {
          if (!charge(budget, site)) return failure();
          auto item = cast<DictionaryAttr>(rawField);
          if (item.getAs<StringAttr>("name") == field) {
            auto resolved = hardware.resolveType(item.getAs<TypeAttr>("type").getValue(),
                                                  plan.instances[0].bindings, site);
            if (failed(resolved)) return failure();
            next = *resolved; break;
          }
        }
        if (!next) return reject("original selected owner field differs");
        (table ? after : before).push_back(field); current = next;
      } else {
        if (table || !isa<ac::TableType>(current) || write.indices.size() != 1)
          return reject("original selected Table layout differs");
        table = cast<ac::TableType>(current);
        if (table.getShape().size() != 1) return reject("original selected Table rank differs");
        current = table.getElementType();
      }
    }
    ArrayAttr prefix = b.getArrayAttr(before);
    ArrayAttr suffix = b.getArrayAttr(table ? after : before);
    Type bit = materialized.rule->getOperand(preparation.clockCapture).getType();
    Type maskType = table ? Type(ac::TableType::get(b.getContext(), table.getShape(), bit)) : bit;
    if (declared->selectors != write.selectors || declared->prefix != prefix ||
        declared->suffix != suffix || declared->table != table ||
        declared->index != (table ? std::optional<ac::BoundValueId>(write.indices.front()) : std::nullopt) ||
        declared->maskType != maskType || !payload ||
        !ac::areEquivalentHardwareTypes(current, payload.getType()))
      return reject("original selected write preparation differs");
    uint64_t cursor = 0;
    llvm::DenseMap<Block *, Operation *> preceding;
    preceding[parent] = last;
    auto take = [&](StringRef name, ArrayRef<Value> operands, ArrayRef<Type> types,
                    DictionaryAttr attributes, Block *block, unsigned regions = 0)
                    -> FailureOr<Operation *> {
      if (cursor == record.generated.size()) return reject("original selected write operations are missing");
      Operation *operation = record.generated[cursor++];
      if (!operation || !operations.contains(operation)) return reject("original selected write locator is not live");
      for (Value operand : operands)
        if (!values.contains(operand)) return reject("original selected write operand is not live");
      if (failed(generatedProof(operation, name, operands, types, attributes,
                                site->getLoc(), block, regions))) return failure();
      auto prior = preceding.lookup(block);
      if (prior && !prior->isBeforeInBlock(operation)) return reject("original selected write operation order differs");
      preceding[block] = operation;
      return operation;
    };
    auto initial = take(ac::BitsConstantOp::getOperationName(), {}, {bit},
        b.getDictionaryAttr({b.getNamedAttr("value", bitsLiteralAttribute(bit,
            plan.transaction->getAttrOfType<DictionaryAttr>("occurrence"), b.getContext(), true))}), parent);
    if (failed(initial)) return failure();
    Value rawPath = (*initial)->getResult(0);
    auto ancestry = plan.versions[write.version].validity.path;
    for (unsigned depth = 0; ancestry.branch; ++depth) {
      if (depth > 128 || !charge(budget, site)) return failure();
      const auto &branch = plan.branches[*ancestry.branch];
      Value selected = bound(branch.condition);
      if (!ancestry.whenTrue) {
        auto inverted = take(ac::BitsUnaryOp::getOperationName(), {selected}, {selected.getType()},
            b.getDictionaryAttr({b.getNamedAttr("opcode", b.getStringAttr("not"))}), parent);
        if (failed(inverted)) return failure(); selected = (*inverted)->getResult(0);
      }
      auto combined = take(ac::BitsBinaryOp::getOperationName(), {rawPath, selected}, {bit},
          b.getDictionaryAttr({b.getNamedAttr("opcode", b.getStringAttr("and"))}), parent);
      if (failed(combined)) return failure(); rawPath = (*combined)->getResult(0);
      ancestry = branch.entry.path;
    }
    Value authored = bound(write.authoredPath);
    auto combined = take(ac::BitsBinaryOp::getOperationName(), {rawPath, authored}, {bit},
        b.getDictionaryAttr({b.getNamedAttr("opcode", b.getStringAttr("and"))}), parent);
    if (failed(combined)) return failure(); rawPath = (*combined)->getResult(0);
    if (record.rawPath != rawPath) return reject("original selected raw statement path differs");
    Value expectedData = payload, expectedMask = rawPath;
    auto merge = [&](Value base, Value guard, Value value, ArrayAttr fields, Block *block) -> FailureOr<Value> {
      auto merged = take(ac::ValueMergeOp::getOperationName(), {base, guard, value},
          {base.getType(), guard.getType()}, b.getDictionaryAttr({
            b.getNamedAttr("paths", b.getArrayAttr({fields})),
            b.getNamedAttr("operandSegmentSizes", b.getDenseI32ArrayAttr({1, 1, 1}))}), block);
      if (failed(merged) || !(*merged)->getResult(1).use_empty()) return failure();
      return (*merged)->getResult(0);
    };
    if (!write.selectors.empty()) {
      auto one = take(ac::BitsConstantOp::getOperationName(), {}, {bit},
          b.getDictionaryAttr({b.getNamedAttr("value", bitsLiteralAttribute(bit,
              plan.transaction->getAttrOfType<DictionaryAttr>("occurrence"), b.getContext(), true))}), parent);
      if (failed(one)) return failure();
      Value selected = (*one)->getResult(0);
      if (!table) {
        auto data = merge(previous, selected, payload, suffix, parent);
        if (failed(data)) return failure();
        expectedData = *data;
      } else {
        Value base = previous;
        for (Attribute raw : prefix) {
          auto field = cast<StringAttr>(raw);
          auto declaration = hardware.lookupStruct(cast<ac::StructType>(base.getType()));
          Type fieldType;
          for (Attribute rawField : declaration.getFields()) {
            if (!charge(budget, site)) return failure();
            auto item = cast<DictionaryAttr>(rawField);
            if (item.getAs<StringAttr>("name") == field) {
              auto resolved = hardware.resolveType(item.getAs<TypeAttr>("type").getValue(),
                                                    plan.instances[0].bindings, site);
              if (failed(resolved)) return failure();
              fieldType = *resolved; break;
            }
          }
          auto projected = take(ac::StructGetOp::getOperationName(), {base}, {fieldType},
                                b.getDictionaryAttr({b.getNamedAttr("field", field)}), parent);
          if (failed(projected)) return failure();
          base = (*projected)->getResult(0);
        }
        Value index = bound(write.indices.front());
        SmallVector<Value> inputs{base};
        for (Value value : {index, payload, rawPath}) {
          Type type = ac::TableType::get(b.getContext(), table.getShape(), value.getType());
          auto splat = take(ac::TableSplatOp::getOperationName(), {value}, {type},
                             b.getDictionaryAttr({b.getNamedAttr("shape", table.getShape())}), parent);
          if (failed(splat)) return failure();
          inputs.push_back((*splat)->getResult(0));
        }
        auto map = take(ac::TableMapOp::getOperationName(), inputs, {table, maskType},
            b.getDictionaryAttr({b.getNamedAttr("shape", table.getShape()),
              b.getNamedAttr("operandSegmentSizes", b.getDenseI32ArrayAttr({4, 0}))}), parent, 1);
        auto count = hardware.getTableSize(table, plan.instances[0].bindings, site);
        if (failed(map) || failed(count) || !(*map)->getRegion(0).hasOneBlock()) return failure();
        auto &body = (*map)->getRegion(0).front();
        unsigned ordinalWidth = std::max(1u, llvm::Log2_64_Ceil(*count));
        Type ordinalType = branchCarrierType(ordinalWidth, site);
        SmallVector<Type> arguments{ordinalType, table.getElementType(), index.getType(), payload.getType(), rawPath.getType()};
        if (!llvm::equal(body.getArgumentTypes(), arguments)) return reject("original selected Table block bindings differ");
        Value ordinal = body.getArgument(0), actualIndex = body.getArgument(2);
        auto width = hardware.getPackedWidth(index.getType(), plan.instances[0].bindings, site);
        if (failed(width)) return failure();
        if (*width == ordinalWidth && !ac::areEquivalentHardwareTypes(actualIndex.getType(), ordinalType)) {
          auto adapted = take(ac::BitsExtractOp::getOperationName(), {actualIndex}, {ordinalType},
              b.getDictionaryAttr({b.getNamedAttr("low", bitsLiteralAttribute(ordinalType,
                    site->getAttrOfType<DictionaryAttr>("occurrence"), b.getContext(), false))}), &body);
          if (failed(adapted)) return failure();
          actualIndex = (*adapted)->getResult(0);
        } else if (*width < ordinalWidth) {
          auto resized = take(ac::BitsResizeOp::getOperationName(), {actualIndex}, {ordinalType},
              b.getDictionaryAttr({b.getNamedAttr("mode", b.getStringAttr("zext"))}), &body);
          if (failed(resized)) return failure();
          actualIndex = (*resized)->getResult(0);
        } else if (*width > ordinalWidth) {
          auto resized = take(ac::BitsResizeOp::getOperationName(), {ordinal}, {actualIndex.getType()},
              b.getDictionaryAttr({b.getNamedAttr("mode", b.getStringAttr("zext"))}), &body);
          if (failed(resized)) return failure();
          ordinal = (*resized)->getResult(0);
        }
        auto compared = take(ac::BitsCompareOp::getOperationName(), {ordinal, actualIndex}, {bit},
            b.getDictionaryAttr({b.getNamedAttr("predicate", b.getStringAttr("eq"))}), &body);
        if (failed(compared)) return failure();
        Value equal = (*compared)->getResult(0);
        auto data = merge(body.getArgument(1), equal, body.getArgument(3), suffix, &body);
        auto mask = take(ac::BitsBinaryOp::getOperationName(), {body.getArgument(4), equal}, {rawPath.getType()},
            b.getDictionaryAttr({b.getNamedAttr("opcode", b.getStringAttr("and"))}), &body);
        if (failed(data) || failed(mask)) return failure();
        if (failed(take(ac::YieldOp::getOperationName(), {*data, (*mask)->getResult(0)}, {}, b.getDictionaryAttr({}), &body))) return failure();
        expectedData = (*map)->getResult(0); expectedMask = (*map)->getResult(1);
        if (!prefix.empty()) {
          auto root = merge(previous, selected, expectedData, prefix, parent);
          if (failed(root)) return failure();
          expectedData = *root;
        }
      }
    }
    if (cursor != record.generated.size() || record.data != expectedData ||
        record.mask != expectedMask || record.mask.getType() != maskType)
      return reject("original selected write result/mask differs");
    last = preceding.lookup(parent);
    return success();
  }
