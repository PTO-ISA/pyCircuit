FailureOr<bool> unorderedOwnerGroup(const PreparedTransactionLowering &prepared,
                                    const TransactionPreparation &transaction,
                                    ac::StateOwnerId owner, SourceCheckBudget &budget) {
  const auto &plan = *transaction.plan;
  const auto &physical = plan.owners[owner];
  auto target = routingKey(anchorPath(plan, physical.anchor), physical.declaration->getResult(0));
  for (const auto &peer : prepared.transactions) {
    if (!charge(budget, plan.transaction)) return failure();
    if (peer.plan == &plan || peer.plan->routingModule != plan.routingModule) continue;
    for (const auto &output : peer.outputs) {
      if (!charge(budget, plan.transaction)) return failure();
      const auto &other = peer.plan->owners[output.binding.owner];
      if (routingKey(anchorPath(*peer.plan, other.anchor), other.declaration->getResult(0)) != target) continue;
      return true;
    }
  }
  return false;
}

struct DomainCommitUse { ArrayRef<Attribute> path; Value mask; };

FailureOr<MaterializedBranchResult> materializeDomainCommit(
    Operation *source, DictionaryAttr occurrence, Type payloadType,
    const ac::HardwareBindings &bindings, Value previous, Value current,
    Value enabled, ArrayRef<DomainCommitUse> domains,
    const ac::HardwareAnalysis &hardware, OpBuilder &builder,
    SourceCheckBudget &budget) {
  MaterializedBranchResult result;
  using Use = DomainCommitUse;
  auto make = [&](OpBuilder &at, StringRef name, ValueRange operands,
                  TypeRange types, ArrayRef<NamedAttribute> attributes,
                  unsigned regions = 0) -> FailureOr<Operation *> {
    if (!charge(budget, source, 32 + 4 * operands.size() + 2 * types.size() + 2 * attributes.size()) ||
        !prepayAppend(budget, source, result.generated)) return failure();
    OperationState state(source->getLoc(), name);
    state.addOperands(operands); state.addTypes(types); state.addAttributes(attributes);
    for (unsigned i = 0; i < regions; ++i) state.addRegion();
    Operation *operation = at.create(state); result.generated.push_back(operation); return operation;
  };
  SmallVector<Use, 0> uses;
  for (const auto &domain : domains) {
    if (!prepayAppend(budget, source, uses)) return failure();
    uses.push_back(domain);
  }
  if (uses.empty()) {
    if (!charge(budget, source, 16) || !prepayAppend(budget, source, result.generated)) return failure();
    Value one = bitsLiteral(enabled.getType(), occurrence, source->getLoc(), true, builder);
    result.generated.push_back(one.getDefiningOp());
    if (!prepayAppend(budget, source, uses)) return failure();
    uses.push_back({{}, one});
  }
  auto binary = [&](OpBuilder &at, Value first, Value second, StringRef opcode) -> FailureOr<Value> {
    auto operation = make(at, ac::BitsBinaryOp::getOperationName(), {first, second}, {first.getType()},
                          {at.getNamedAttr("opcode", at.getStringAttr(opcode))});
    if (failed(operation)) return failure(); return (*operation)->getResult(0);
  };
  auto project = [&](OpBuilder &at, Value value, StringAttr field, Type type) -> FailureOr<Value> {
    auto operation = make(at, ac::StructGetOp::getOperationName(), {value}, {type},
                          {at.getNamedAttr("field", field)});
    if (failed(operation)) return failure(); return (*operation)->getResult(0);
  };
  std::function<FailureOr<Value>(Type, Value, Value, Value, ArrayRef<Use>, OpBuilder &, unsigned)> compose;
  compose = [&](Type type, Value base, Value candidateValue, Value enable,
                ArrayRef<Use> domains, OpBuilder &at, unsigned depth) -> FailureOr<Value> {
    if (depth > 128 || !charge(budget, source, 4)) return failure();
    if (domains.empty()) return base;
    // One undivided domain selects the aggregate directly. No Table ordinal
    // layout is needed in the shared declaration for an incoming whole value.
    if (isa<ac::TableType>(type) && domains.size() == 1 && domains.front().path.empty() &&
        isa<ac::BitsType>(domains.front().mask.getType())) {
      auto guard = binary(at, enable, domains.front().mask, "and");
      if (failed(guard)) return failure();
      auto merged = make(at, ac::ValueMergeOp::getOperationName(), {base, *guard, candidateValue},
          {type, guard->getType()}, {at.getNamedAttr("paths", at.getArrayAttr({at.getArrayAttr({})})),
           at.getNamedAttr("operandSegmentSizes", at.getDenseI32ArrayAttr({1, 1, 1}))});
      if (failed(merged)) return failure();
      return (*merged)->getResult(0);
    }
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
        auto old = project(at, base, name, *resolved); if (failed(old)) return failure();
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
          auto next = project(at, candidateValue, name, *resolved); if (failed(next)) return failure();
          auto chosen = compose(*resolved, *old, *next, enable, selected, at, depth + 1);
          if (failed(chosen)) return failure(); value = *chosen;
        }
        if (!prepayAppend(budget, source, fields)) return failure(); fields.push_back(value);
      }
      auto rebuilt = make(at, ac::StructCreateOp::getOperationName(), fields, {type}, {});
      if (failed(rebuilt)) return failure(); return (*rebuilt)->getResult(0);
    }
    if (auto table = dyn_cast<ac::TableType>(type)) {
      SmallVector<Value> inputs{base, candidateValue};
      auto splat = [&](Value value) -> FailureOr<Value> {
        Type shaped = ac::TableType::get(at.getContext(), table.getShape(), value.getType());
        auto operation = make(at, ac::TableSplatOp::getOperationName(), {value}, {shaped},
                              {at.getNamedAttr("shape", table.getShape())});
        if (failed(operation)) return failure(); return (*operation)->getResult(0);
      };
      auto shapedEnable = splat(enable); if (failed(shapedEnable)) return failure();
      inputs.push_back(*shapedEnable);
      for (const auto &domain : domains) {
        if (!charge(budget, source)) return failure();
        Value mask = domain.mask;
        if (isa<ac::BitsType>(mask.getType())) {
          auto shaped = splat(mask); if (failed(shaped)) return failure(); mask = *shaped;
        } else if (!isa<ac::TableType>(mask.getType())) return failure();
        if (!prepayAppend(budget, source, inputs)) return failure(); inputs.push_back(mask);
      }
      auto mapped = make(at, ac::TableMapOp::getOperationName(), inputs, {table},
          {at.getNamedAttr("shape", table.getShape()),
           at.getNamedAttr("operandSegmentSizes", at.getDenseI32ArrayAttr({int32_t(inputs.size()), 0}))}, 1);
      auto count = hardware.getTableSize(table, bindings, source);
      if (failed(mapped) || failed(count) || !*count ||
          !charge(budget, source, 16 + 2 * inputs.size() + SourceCheckBudget::bytes(sizeof(Block)))) return failure();
      auto *body = new Block; (*mapped)->getRegion(0).push_back(body);
      body->addArgument(branchCarrierType(std::max(1u, llvm::Log2_64_Ceil(*count)), source,
                                         cast<ac::BitsType>(enable.getType())), source->getLoc());
      Value old = body->addArgument(table.getElementType(), source->getLoc());
      Value next = body->addArgument(table.getElementType(), source->getLoc());
      Value active = body->addArgument(enable.getType(), source->getLoc());
      SmallVector<Use, 0> elements;
      for (auto [i, domain] : llvm::enumerate(domains)) {
        auto path = domain.path;
        if (!path.empty()) {
          auto selector = cast<DictionaryAttr>(path.front());
          if (!selector.getAs<IntegerAttr>("index")) return failure();
          path = path.drop_front();
        }
        Type element = cast<ac::TableType>(inputs[3 + i].getType()).getElementType();
        Value mask = body->addArgument(element, source->getLoc());
        if (!prepayAppend(budget, source, elements)) return failure(); elements.push_back({path, mask});
      }
      auto nested = OpBuilder::atBlockEnd(body);
      auto value = compose(table.getElementType(), old, next, active, elements, nested, depth + 1);
      if (failed(value)) return failure();
      if (failed(make(nested, ac::YieldOp::getOperationName(), {*value}, {}, {}))) return failure();
      return (*mapped)->getResult(0);
    }
    if (!isa<ac::BitsType, ac::EnumType>(type)) return failure();
    Value mask;
    for (const auto &domain : domains) {
      if (!domain.path.empty() || !isa<ac::BitsType>(domain.mask.getType())) return failure();
      if (!mask) mask = domain.mask;
      else { auto either = binary(at, mask, domain.mask, "or"); if (failed(either)) return failure(); mask = *either; }
    }
    auto guard = binary(at, enable, mask, "and"); if (failed(guard)) return failure();
    auto merged = make(at, ac::ValueMergeOp::getOperationName(), {base, *guard, candidateValue},
        {type, guard->getType()}, {at.getNamedAttr("paths", at.getArrayAttr({at.getArrayAttr({})})),
         at.getNamedAttr("operandSegmentSizes", at.getDenseI32ArrayAttr({1, 1, 1}))});
    if (failed(merged)) return failure(); return (*merged)->getResult(0);
  };
  auto value = compose(payloadType, previous, current, enabled, uses, builder, 0);
  if (failed(value)) return failure(); result.value = *value; return result;
}
