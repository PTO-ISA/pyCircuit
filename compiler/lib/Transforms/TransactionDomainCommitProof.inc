  LogicalResult domainCommitProof(Operation *source, DictionaryAttr occurrence,
                                   Type payloadType, const ac::HardwareBindings &bindings,
                                   Value current, Value enabled,
                                   ArrayRef<DomainCommitUse> domains,
                                   const MaterializedBranchResult &data,
                                   Value previous, Block *parent, Operation *&last) {
    if (!data.value || !values.contains(previous))
      return reject("original unordered domain commit locator differs");
    Builder b(source->getContext());
    using Use = DomainCommitUse;
    SmallVector<Use, 0> uses;
  for (const auto &domain : domains) {
    if (!prepayAppend(budget, source, uses)) return failure();
    uses.push_back(domain);
  }
    uint64_t cursor = 0;
    llvm::DenseMap<Block *, Operation *> previousOperation;
    previousOperation[parent] = last;
    auto take = [&](StringRef name, ArrayRef<Value> operands, ArrayRef<Type> types,
                    DictionaryAttr attributes, Block *block, unsigned regions = 0)
                    -> FailureOr<Operation *> {
      if (cursor == data.generated.size()) return reject("original unordered commit operations are missing");
      Operation *operation = data.generated[cursor++];
      if (!operation || !operations.contains(operation)) return reject("original unordered commit operation is not live");
      for (Value operand : operands)
        if (!values.contains(operand)) return reject("original unordered commit operand is not live");
      if (failed(generatedProof(operation, name, operands, types, attributes,
                                source->getLoc(), block, regions))) return failure();
      auto preceding = previousOperation.lookup(block);
      if (preceding && !preceding->isBeforeInBlock(operation)) return reject("original unordered commit operation order differs");
      previousOperation[block] = operation; return operation;
    };
    if (uses.empty()) {
      auto one = take(ac::BitsConstantOp::getOperationName(), {}, {enabled.getType()},
          b.getDictionaryAttr({b.getNamedAttr("value", bitsLiteralAttribute(enabled.getType(), occurrence, b.getContext(), true))}), parent);
      if (failed(one) || !prepayAppend(budget, source, uses)) return failure();
      uses.push_back({{}, (*one)->getResult(0)});
    }
    auto binary = [&](Value first, Value second, StringRef opcode, Block *block) -> FailureOr<Value> {
      auto operation = take(ac::BitsBinaryOp::getOperationName(), {first, second}, {first.getType()},
          b.getDictionaryAttr({b.getNamedAttr("opcode", b.getStringAttr(opcode))}), block);
      if (failed(operation)) return failure(); return (*operation)->getResult(0);
    };
    auto project = [&](Value value, StringAttr field, Type type, Block *block) -> FailureOr<Value> {
      auto operation = take(ac::StructGetOp::getOperationName(), {value}, {type},
                            b.getDictionaryAttr({b.getNamedAttr("field", field)}), block);
      if (failed(operation)) return failure(); return (*operation)->getResult(0);
    };
    std::function<FailureOr<Value>(Type, Value, Value, Value, ArrayRef<Use>, Block *, unsigned)> verify;
    verify = [&](Type type, Value base, Value candidateValue, Value enable,
                 ArrayRef<Use> domains, Block *block, unsigned depth) -> FailureOr<Value> {
      if (depth > 128 || !charge(budget, source, 4)) return failure();
      if (domains.empty()) return base;
      if (auto structure = dyn_cast<ac::StructType>(type)) {
        auto declaration = hardware.lookupStruct(structure);
        if (!declaration) return failure();
        SmallVector<Value> fields;
        for (Attribute raw : declaration.getFields()) {
          if (!charge(budget, source, 4)) return failure();
          auto field = cast<DictionaryAttr>(raw);
          auto name = field.getAs<StringAttr>("name");
          auto resolved = hardware.resolveType(field.getAs<TypeAttr>("type").getValue(),
                                                bindings, source);
          if (failed(resolved)) return failure();
          auto old = project(base, name, *resolved, block); if (failed(old)) return failure();
          SmallVector<Use, 0> selected;
          for (const auto &domain : domains) {
            if (!charge(budget, source)) return failure();
            if (domain.path.empty()) {
              if (!prepayAppend(budget, source, selected)) return failure(); selected.push_back(domain); continue;
            }
            auto selector = cast<DictionaryAttr>(domain.path.front());
            if (selector.getAs<StringAttr>("field") == name) {
              if (!prepayAppend(budget, source, selected)) return failure();
              selected.push_back({domain.path.drop_front(), domain.mask});
            }
          }
          Value value = *old;
          if (!selected.empty()) {
            auto next = project(candidateValue, name, *resolved, block); if (failed(next)) return failure();
            auto chosen = verify(*resolved, *old, *next, enable, selected, block, depth + 1);
            if (failed(chosen)) return failure(); value = *chosen;
          }
          if (!prepayAppend(budget, source, fields)) return failure(); fields.push_back(value);
        }
        auto create = take(ac::StructCreateOp::getOperationName(), fields, {type}, b.getDictionaryAttr({}), block);
        if (failed(create)) return failure(); return (*create)->getResult(0);
      }
      if (auto table = dyn_cast<ac::TableType>(type)) {
        SmallVector<Value> inputs{base, candidateValue};
        auto splat = [&](Value value) -> FailureOr<Value> {
          Type shaped = ac::TableType::get(b.getContext(), table.getShape(), value.getType());
          auto operation = take(ac::TableSplatOp::getOperationName(), {value}, {shaped},
              b.getDictionaryAttr({b.getNamedAttr("shape", table.getShape())}), block);
          if (failed(operation)) return failure(); return (*operation)->getResult(0);
        };
        auto shaped = splat(enable); if (failed(shaped)) return failure(); inputs.push_back(*shaped);
        for (const auto &domain : domains) {
          if (!charge(budget, source)) return failure();
          Value mask = domain.mask;
          if (isa<ac::BitsType>(mask.getType())) { auto row = splat(mask); if (failed(row)) return failure(); mask = *row; }
          else if (!isa<ac::TableType>(mask.getType())) return failure();
          if (!prepayAppend(budget, source, inputs)) return failure(); inputs.push_back(mask);
        }
        auto mapped = take(ac::TableMapOp::getOperationName(), inputs, {table},
            b.getDictionaryAttr({b.getNamedAttr("shape", table.getShape()),
              b.getNamedAttr("operandSegmentSizes", b.getDenseI32ArrayAttr({int32_t(inputs.size()), 0}))}), block, 1);
        auto count = hardware.getTableSize(table, bindings, source);
        if (failed(mapped) || failed(count) || !*count || !(*mapped)->getRegion(0).hasOneBlock()) return failure();
        auto &body = (*mapped)->getRegion(0).front();
        SmallVector<Type> arguments{branchCarrierType(std::max(1u, llvm::Log2_64_Ceil(*count)), source,
                                                     cast<ac::BitsType>(enable.getType())),
                                    table.getElementType(), table.getElementType(), enable.getType()};
        for (Value input : ArrayRef<Value>(inputs).drop_front(3))
          arguments.push_back(cast<ac::TableType>(input.getType()).getElementType());
        if (!llvm::equal(body.getArgumentTypes(), arguments)) return reject("original unordered Table bindings differ");
        SmallVector<Use, 0> elements;
        for (auto [i, domain] : llvm::enumerate(domains)) {
          auto path = domain.path;
          if (!path.empty()) {
            if (!cast<DictionaryAttr>(path.front()).getAs<IntegerAttr>("index")) return failure();
            path = path.drop_front();
          }
          if (!prepayAppend(budget, source, elements)) return failure();
          elements.push_back({path, body.getArgument(4 + i)});
        }
        auto value = verify(table.getElementType(), body.getArgument(1), body.getArgument(2),
                             body.getArgument(3), elements, &body, depth + 1);
        if (failed(value)) return failure();
        if (failed(take(ac::YieldOp::getOperationName(), {*value}, {}, b.getDictionaryAttr({}), &body))) return failure();
        return (*mapped)->getResult(0);
      }
      if (!isa<ac::BitsType, ac::EnumType>(type)) return failure();
      Value mask;
      for (const auto &domain : domains) {
        if (!domain.path.empty() || !isa<ac::BitsType>(domain.mask.getType())) return failure();
        if (!mask) mask = domain.mask;
        else { auto either = binary(mask, domain.mask, "or", block); if (failed(either)) return failure(); mask = *either; }
      }
      auto guard = binary(enable, mask, "and", block); if (failed(guard)) return failure();
      auto merge = take(ac::ValueMergeOp::getOperationName(), {base, *guard, candidateValue}, {type, guard->getType()},
          b.getDictionaryAttr({b.getNamedAttr("paths", b.getArrayAttr({b.getArrayAttr({})})),
            b.getNamedAttr("operandSegmentSizes", b.getDenseI32ArrayAttr({1, 1, 1}))}), block);
      if (failed(merge) || !(*merge)->getResult(1).use_empty()) return failure(); return (*merge)->getResult(0);
    };
    auto value = verify(payloadType, previous, current, enabled, uses, parent, 0);
    if (failed(value) || *value != data.value || cursor != data.generated.size())
      return reject("original unordered commit data/domain coverage differs");
    last = previousOperation.lookup(parent);
    return success();
  }

  LogicalResult domainCommitProof(const TransactionPreparation &transaction,
                                   uint64_t ordinal, const MaterializedTransaction &record,
                                   Value previous, Block *parent, Operation *&last) {
    const auto &plan = *transaction.plan;
    const auto &output = transaction.outputs[ordinal];
    const auto &owner = plan.owners[output.binding.owner];
    const auto &commit = record.commits[ordinal];
    if (!commit.domainMerge) return reject("original unordered domain merge is absent");
    SmallVector<DomainCommitUse, 0> domains;
    for (const auto &domain : transaction.writeDomains) {
      if (!charge(budget, plan.transaction)) return failure();
      if (domain.candidate != ordinal) continue;
      if (!prepayAppend(budget, plan.transaction, domains)) return failure();
      domains.push_back({domain.selectors.getValue(), record.rule->getResult(domain.result)});
    }
    auto source = plan.transaction;
    return domainCommitProof(source, source.getOccurrence(),
        owner.payloadType, plan.frames[plan.rootFrame].bindings,
        record.rule->getResult(output.dataResult), record.rule->getResult(output.enableResult),
        domains, commit.data, previous, parent, last);
  }
