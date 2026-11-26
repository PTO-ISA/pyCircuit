// Reset normalization remains a public graph transformation. Its checker peels
// only the independently reconstructed mask and then compares the entire graph.
SmallVector<unsigned> resetMaskOutputs(ac::RuleOp rule) {
  auto row = rule->getAttrOfType<DictionaryAttr>("ac.sampled_admission");
  SmallVector<unsigned> outputs;
  auto add = [&](unsigned index) {
    if (!llvm::is_contained(outputs, index)) outputs.push_back(index);
  };
  add(row.getAs<IntegerAttr>("fire_result").getInt());
  auto owner = cast<ac::ModuleOp>(rule->getParentOp());
  auto wiring = owner->getAttrOfType<DictionaryAttr>("ac.transaction_wiring");
  if (wiring) for (Attribute raw : wiring.getAs<ArrayAttr>("writers")) {
    auto writer = cast<DictionaryAttr>(raw);
    if (writer.get("registration") != rule->getAttr("occurrence")) continue;
    unsigned ordinary = writer.getAs<IntegerAttr>("ordinary_results").getInt();
    for (auto [index, candidate] : llvm::enumerate(writer.getAs<ArrayAttr>("candidates"))) {
      (void)candidate;
      add(ordinary + 2 * index + 1);
    }
  }
  unsigned next = row.getAs<IntegerAttr>("fire_result").getInt() + 1;
  if (auto observations = row.getAs<ArrayAttr>("observations"))
    for (Attribute raw : observations) {
      add(next);
      next += 1 + cast<DictionaryAttr>(raw).getAs<IntegerAttr>("value_count").getInt();
    }
  unsigned controls = row.getAs<ArrayAttr>("controls").size();
  next += 2 * controls;
  for (Attribute raw : row.getAs<ArrayAttr>("body_checks")) {
    (void)raw;
    add(next + 1); next += 2;
  }
  if (auto proposals = row.getAs<ArrayAttr>("queue_proposals"))
    for (Attribute raw : proposals) {
      auto proposal = cast<DictionaryAttr>(raw);
      add(next + 1);
      next += proposal.getAs<StringAttr>("kind").getValue() == "push" ? 3 : 2;
    }
  if (auto proposals = row.getAs<ArrayAttr>("memory_proposals"))
    for (Attribute raw : proposals) {
      auto proposal = cast<DictionaryAttr>(raw);
      add(next + 1);
      next += 2 + proposal.getAs<ArrayAttr>("arguments").size();
    }
  llvm::sort(outputs);
  return outputs;
}

bool resetMaskCandidateOutput(ac::RuleOp rule, unsigned index) {
  auto owner = cast<ac::ModuleOp>(rule->getParentOp());
  auto wiring = owner->getAttrOfType<DictionaryAttr>("ac.transaction_wiring");
  if (!wiring) return false;
  for (Attribute raw : wiring.getAs<ArrayAttr>("writers")) {
    auto writer = cast<DictionaryAttr>(raw);
    if (writer.get("registration") != rule->getAttr("occurrence")) continue;
    unsigned ordinary = writer.getAs<IntegerAttr>("ordinary_results").getInt();
    return index >= ordinary && index < ordinary + 2 * writer.getAs<ArrayAttr>("candidates").size() &&
        (index - ordinary) % 2 == 1;
  }
  return false;
}

bool hasResetMask(ac::RuleOp rule) {
  auto row = rule->getAttrOfType<DictionaryAttr>("ac.sampled_admission");
  auto masked = row ? row.getAs<BoolAttr>("reset_masked") : BoolAttr();
  return masked && masked.getValue();
}

void addResetMask(ac::RuleOp rule) {
  auto row = rule->getAttrOfType<DictionaryAttr>("ac.sampled_admission");
  auto resets = row ? row.getAs<ArrayAttr>("resets") : ArrayAttr();
  if (!resets || resets.empty() || hasResetMask(rule)) return;
  auto &block = rule.getBody().front();
  auto yield = cast<ac::YieldOp>(block.back());
  OpBuilder builder(yield);
  unsigned clock = row.getAs<IntegerAttr>("clock_capture").getInt();
  Type bit = block.getArgument(clock).getType();
  auto create = [&](StringRef name, ValueRange operands, ArrayRef<NamedAttribute> attrs) {
    OperationState op(rule.getLoc(), name);
    op.addOperands(operands); op.addTypes(bit); op.addAttributes(attrs);
    return builder.create(op)->getResult(0);
  };
  Value mask;
  for (Attribute raw : resets) {
    auto reset = cast<DictionaryAttr>(raw);
    Value input = block.getArgument(reset.getAs<IntegerAttr>("capture").getInt());
    bool high = reset.getAs<StringAttr>("polarity").getValue() == "high";
    Value activeLevel = create(ac::BitsConstantOp::getOperationName(), {}, {
        builder.getNamedAttr("value", bitsLiteralAttribute(bit, rule->getAttrOfType<DictionaryAttr>("occurrence"), rule.getContext(), high))});
    Value active = create(ac::BitsCompareOp::getOperationName(), {input, activeLevel}, {
        builder.getNamedAttr("predicate", builder.getStringAttr("case_eq"))});
    Value inactive = create(ac::BitsUnaryOp::getOperationName(), {active}, {
        builder.getNamedAttr("opcode", builder.getStringAttr("not"))});
    mask = mask ? create(ac::BitsBinaryOp::getOperationName(), {mask, inactive}, {
        builder.getNamedAttr("opcode", builder.getStringAttr("and"))}) : inactive;
  }
  unsigned fireIndex = row.getAs<IntegerAttr>("fire_result").getInt();
  Value fire = create(ac::BitsBinaryOp::getOperationName(), {yield->getOperand(fireIndex), mask}, {
      builder.getNamedAttr("opcode", builder.getStringAttr("and"))});
  for (unsigned index : resetMaskOutputs(rule)) {
    if (index == fireIndex) { yield->setOperand(index, fire); continue; }
    Value old = yield->getOperand(index);
    Value gated;
    if (resetMaskCandidateOutput(rule, index)) {
      auto enabled = cast<ac::BitsBinaryOp>(old.getDefiningOp());
      gated = create(ac::BitsBinaryOp::getOperationName(), {fire, enabled.getRhs()}, {
          builder.getNamedAttr("opcode", builder.getStringAttr("and"))});
    } else gated = create(ac::BitsBinaryOp::getOperationName(), {old, mask}, {
        builder.getNamedAttr("opcode", builder.getStringAttr("and"))});
    yield->setOperand(index, gated);
  }
  NamedAttrList metadata(row);
  metadata.set("reset_masked", builder.getBoolAttr(true));
  rule->setAttr("ac.sampled_admission", metadata.getDictionary(rule.getContext()));
}

LogicalResult peelResetMask(ac::RuleOp source, ac::RuleOp target) {
  auto row = source->getAttrOfType<DictionaryAttr>("ac.sampled_admission");
  auto resets = row ? row.getAs<ArrayAttr>("resets") : ArrayAttr();
  if (!resets || resets.empty() || hasResetMask(source)) return success();
  auto reject = [&] { return target.emitOpError("transaction reset-mask correspondence differs"); };
  if (!hasResetMask(target)) return reject();
  auto &block = target.getBody().front();
  auto yield = cast<ac::YieldOp>(block.back());
  unsigned clock = row.getAs<IntegerAttr>("clock_capture").getInt();
  Type bit = block.getArgument(clock).getType();
  SmallVector<Operation *> inserted;
  Value mask;
  auto shape = [&](Operation *op, StringRef name, unsigned operands, StringRef key, StringRef value) {
    return op && op->getBlock() == &block && op->getName().getStringRef() == name &&
        op->getNumOperands() == operands && op->getNumResults() == 1 && op->getResult(0).getType() == bit &&
        !op->getNumRegions() && !op->getNumSuccessors() && op->getLoc() == source.getLoc() &&
        op->getAttrs().size() == 1 && op->getAttrOfType<StringAttr>(key) &&
        op->getAttrOfType<StringAttr>(key).getValue() == value;
  };
  unsigned fireIndex = row.getAs<IntegerAttr>("fire_result").getInt();
  Value maskedFire = yield->getOperand(fireIndex);
  auto *fireGate = maskedFire.getDefiningOp();
  if (!shape(fireGate, ac::BitsBinaryOp::getOperationName(), 2, "opcode", "and")) return reject();
  mask = fireGate->getOperand(1);
  IRMapping oldValues;
  auto &sourceBlock = source.getBody().front();
  for (auto [a, b] : llvm::zip(sourceBlock.getArguments(), block.getArguments())) oldValues.map(a, b);
  auto next = block.begin();
  for (Operation &operation : sourceBlock.without_terminator()) {
    if (next == block.end() || operation.getNumResults() != next->getNumResults()) return reject();
    for (auto [a, b] : llvm::zip(operation.getResults(), next->getResults())) oldValues.map(a, b);
    ++next;
  }
  auto sourceYield = cast<ac::YieldOp>(sourceBlock.back());
  for (unsigned index : resetMaskOutputs(source)) {
    auto *gate = yield->getOperand(index).getDefiningOp();
    if (!shape(gate, ac::BitsBinaryOp::getOperationName(), 2, "opcode", "and")) return reject();
    inserted.push_back(gate);
    if (resetMaskCandidateOutput(source, index)) {
      auto old = sourceYield->getOperand(index).getDefiningOp<ac::BitsBinaryOp>();
      if (!old || old.getOpcode() != "and" || gate->getOperand(0) != maskedFire ||
          gate->getOperand(1) != oldValues.lookupOrNull(old.getRhs())) return reject();
      Value restored = oldValues.lookupOrNull(sourceYield->getOperand(index));
      if (!restored) return reject();
      yield->setOperand(index, restored);
    } else {
      if (gate->getOperand(1) != mask) return reject();
      yield->setOperand(index, gate->getOperand(0));
    }
  }
  SmallVector<Value> inactive(resets.size());
  Value cursor = mask;
  for (unsigned i = resets.size(); i > 1; --i) {
    auto *join = cursor.getDefiningOp();
    if (!shape(join, ac::BitsBinaryOp::getOperationName(), 2, "opcode", "and")) return reject();
    inactive[i - 1] = join->getOperand(1); cursor = join->getOperand(0); inserted.push_back(join);
  }
  inactive[0] = cursor;
  for (auto [raw, value] : llvm::zip(resets, inactive)) {
    auto reset = cast<DictionaryAttr>(raw);
    auto *invert = value.getDefiningOp();
    if (!shape(invert, ac::BitsUnaryOp::getOperationName(), 1, "opcode", "not")) return reject();
    auto *active = invert->getOperand(0).getDefiningOp();
    if (!shape(active, ac::BitsCompareOp::getOperationName(), 2, "predicate", "case_eq") ||
        active->getOperand(0) != block.getArgument(reset.getAs<IntegerAttr>("capture").getInt())) return reject();
    auto constant = active->getOperand(1).getDefiningOp<ac::BitsConstantOp>();
    bool high = reset.getAs<StringAttr>("polarity").getValue() == "high";
    if (!constant || constant->getBlock() != &block || constant->getNumOperands() ||
        constant->getNumResults() != 1 || constant->getResult(0).getType() != bit ||
        constant->getAttrs().size() != 1 || constant.getLoc() != source.getLoc() ||
        constant.getValue() != bitsLiteralAttribute(bit, source->getAttrOfType<DictionaryAttr>("occurrence"), source.getContext(), high)) return reject();
    inserted.append({invert, active, constant.getOperation()});
  }
  llvm::DenseSet<Operation *> owned;
  for (Operation *operation : inserted)
    if (!owned.insert(operation).second) return reject();
  SmallVector<Operation *> ready;
  for (Operation *operation : inserted) if (operation->use_empty()) ready.push_back(operation);
  while (!ready.empty()) {
    Operation *operation = ready.pop_back_val();
    if (!owned.erase(operation)) continue;
    SmallVector<Operation *> dependencies;
    for (Value operand : operation->getOperands())
      if (auto *definition = operand.getDefiningOp(); definition && owned.contains(definition))
        dependencies.push_back(definition);
    operation->erase();
    for (Operation *definition : dependencies)
      if (definition->use_empty()) ready.push_back(definition);
  }
  if (!owned.empty()) return reject();
  NamedAttrList metadata(target->getAttrOfType<DictionaryAttr>("ac.sampled_admission"));
  metadata.erase("reset_masked");
  target->setAttr("ac.sampled_admission", metadata.getDictionary(target.getContext()));
  return success();
}

LogicalResult exactResetGraph(Operation *source, Operation *target, IRMapping &mapping,
                              SmallVectorImpl<std::pair<Operation *, Operation *>> &pairs,
                              SourceCheckBudget &budget) {
  if (!charge(budget, source, 16 + 4 * source->getNumResults() + 2 * source->getNumOperands())) return failure();
  if (source->getName() != target->getName() || source->getAttrs() != target->getAttrs() ||
      source->getPropertiesAsAttribute() != target->getPropertiesAsAttribute() || source->getLoc() != target->getLoc() ||
      source->getResultTypes() != target->getResultTypes() || source->getNumOperands() != target->getNumOperands() ||
      source->getNumRegions() != target->getNumRegions() || source->getNumSuccessors() != target->getNumSuccessors()) return failure();
  mapping.map(source, target); pairs.push_back({source, target});
  for (auto [a, b] : llvm::zip(source->getResults(), target->getResults())) mapping.map(a, b);
  for (auto [a, b] : llvm::zip(source->getRegions(), target->getRegions())) {
    if (a.getBlocks().size() != b.getBlocks().size()) return failure();
    for (auto [left, right] : llvm::zip(a, b)) {
      if (!charge(budget, source, 4 + 4 * left.getNumArguments())) return failure();
      if (!llvm::equal(left.getArgumentTypes(), right.getArgumentTypes()) || left.getOperations().size() != right.getOperations().size()) return failure();
      mapping.map(&left, &right);
      for (auto [x, y] : llvm::zip(left.getArguments(), right.getArguments())) {
        if (x.getLoc() != y.getLoc()) return failure(); mapping.map(x, y);
      }
      for (auto [x, y] : llvm::zip(left, right)) if (failed(exactResetGraph(&x, &y, mapping, pairs, budget))) return failure();
    }
  }
  return success();
}

FailureOr<OwningOpRef<mlir::ModuleOp>> materializeTransactionResetMasksImpl(
    mlir::ModuleOp routed, SourceCheckBudget &budget) {
  if (failed(ac::verifyRoutedPackage(routed)) || failed(prepayClone(routed, budget))) return failure();
  auto paid = routed.walk([&](ac::RuleOp rule) {
    auto row = rule->getAttrOfType<DictionaryAttr>("ac.sampled_admission");
    auto resets = row ? row.getAs<ArrayAttr>("resets") : ArrayAttr();
    if (resets && !resets.empty() && !hasResetMask(rule) &&
        !charge(budget, rule, SourceCheckBudget::add(
          SourceCheckBudget::multiply(resets.size(), 64),
          SourceCheckBudget::multiply(resetMaskOutputs(rule).size(), 32))))
      return WalkResult::interrupt();
    return WalkResult::advance();
  });
  if (paid.wasInterrupted()) return failure();
  OwningOpRef<mlir::ModuleOp> candidate = cast<mlir::ModuleOp>(routed->clone());
  auto contract = routed->getAttrOfType<IntegerAttr>("ac.routed_contract");
  if (contract && contract.getInt() == 7)
    candidate->walk([&](ac::RuleOp rule) { addResetMask(rule); });
  return std::move(candidate);
}
LogicalResult verifyTransactionResetMaskMaterializationImpl(
    mlir::ModuleOp routed, mlir::ModuleOp candidate, SourceCheckBudget &budget) {
  if (failed(ac::verifyRoutedPackage(routed)) || failed(ac::verifyRoutedPackage(candidate)) ||
      failed(prepayClone(candidate, budget))) return failure();
  OwningOpRef<mlir::ModuleOp> peeled = cast<mlir::ModuleOp>(candidate->clone());
  SmallVector<ac::RuleOp> sources, targets;
  routed.walk([&](ac::RuleOp rule) { sources.push_back(rule); });
  peeled->walk([&](ac::RuleOp rule) { targets.push_back(rule); });
  if (sources.size() != targets.size()) return routed.emitError("reset normalization Rule inventory differs");
  for (auto [source, target] : llvm::zip(sources, targets))
    if (failed(peelResetMask(source, target))) return failure();
  IRMapping mapping;
  SmallVector<std::pair<Operation *, Operation *>> pairs;
  if (failed(exactResetGraph(routed, *peeled, mapping, pairs, budget))) return routed.emitError("reset normalization changed unrelated structure");
  for (auto [source, target] : pairs) {
    for (auto [a, b] : llvm::zip(source->getOperands(), target->getOperands()))
      if (mapping.lookupOrNull(a) != b) return routed.emitError("reset normalization changed unrelated SSA");
    for (auto [a, b] : llvm::zip(source->getSuccessors(), target->getSuccessors()))
      if (mapping.lookupOrNull(a) != b) return routed.emitError("reset normalization changed unrelated successor");
  }
  return success();
}
